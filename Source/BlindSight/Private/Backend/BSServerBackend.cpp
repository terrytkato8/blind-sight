#include "Backend/BSServerBackend.h"
#include "Core/BSGameSettings.h"
#include "BlindSight.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/GameInstance.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

static FString SerializeJson(const TSharedRef<FJsonObject>& Obj)
{
	FString Out;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Obj, Writer);
	return Out;
}

bool UBSServerBackend::ShouldCreateSubsystem(UObject* Outer) const
{
	// Never create this on a shipping client — it would mean the server key is in the build.
#if UE_SERVER
	return true;
#elif WITH_EDITOR
	return true;	// listen-server development
#else
	return false;
#endif
}

void UBSServerBackend::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UBSGameSettings* Settings = GetDefault<UBSGameSettings>();
	BaseUrl = Settings->BackendBaseUrl;
	BaseUrl.RemoveFromEnd(TEXT("/"));

	// The key comes from the environment, never from a config file that could be packaged.
	ServerKey = FPlatformMisc::GetEnvironmentVariable(TEXT("BS_SERVER_KEY"));
	Region    = FPlatformMisc::GetEnvironmentVariable(TEXT("BS_REGION"));
	if (Region.IsEmpty()) Region = TEXT("local");

	// Allocation platforms (Agones, GameLift, Edgegap, Hathora) inject the public address.
	AdvertisedAddress = FPlatformMisc::GetEnvironmentVariable(TEXT("BS_ADVERTISE_ADDR"));
	FParse::Value(FCommandLine::Get(), TEXT("BSAdvertise="), AdvertisedAddress);

	if (!IsBackendConfigured())
	{
		UE_LOG(LogBlindSight, Warning, TEXT("Server backend not configured (BS_SERVER_KEY unset). Running standalone."));
		return;
	}

	RegisterServer();

	if (UWorld* W = GetGameInstance()->GetWorld())
	{
		W->GetTimerManager().SetTimer(HeartbeatTimer, FTimerDelegate::CreateUObject(this, &UBSServerBackend::Heartbeat), 15.f, true);
		W->GetTimerManager().SetTimer(FlushTimer, FTimerDelegate::CreateUObject(this, &UBSServerBackend::FlushTelemetry), 20.f, true);
	}
}

void UBSServerBackend::Deinitialize()
{
	FlushTelemetry();
	if (UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		W->GetTimerManager().ClearTimer(HeartbeatTimer);
		W->GetTimerManager().ClearTimer(FlushTimer);
	}
	Super::Deinitialize();
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> UBSServerBackend::MakeRequest(const FString& Verb, const FString& Path) const
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(BaseUrl + Path);
	Req->SetVerb(Verb);
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Req->SetHeader(TEXT("X-Server-Key"), ServerKey);
	Req->SetTimeout(15.f);
	return Req;
}

void UBSServerBackend::RegisterServer()
{
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("region"), Region);
	Body->SetStringField(TEXT("address"), AdvertisedAddress);
	Body->SetStringField(TEXT("build"), FApp::GetBuildVersion());

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = MakeRequest(TEXT("POST"), TEXT("/server/register"));
	Req->SetContentAsString(SerializeJson(Body));
	Req->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{
			if (!bOk || !Res.IsValid() || Res->GetResponseCode() != 200)
			{
				UE_LOG(LogBlindSight, Error, TEXT("Server registration failed — this container will never be allocated."));
				return;
			}
			TSharedPtr<FJsonObject> J;
			const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Res->GetContentAsString());
			if (FJsonSerializer::Deserialize(R, J) && J.IsValid())
			{
				ServerId = J->GetStringField(TEXT("serverId"));
				UE_LOG(LogBlindSight, Log, TEXT("Registered with backend as %s (%s)"), *ServerId, *Region);
			}
		});
	Req->ProcessRequest();
}

void UBSServerBackend::Heartbeat()
{
	if (ServerId.IsEmpty()) { RegisterServer(); return; }
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("serverId"), ServerId);
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = MakeRequest(TEXT("POST"), TEXT("/server/heartbeat"));
	Req->SetContentAsString(SerializeJson(Body));
	Req->ProcessRequest();
}

void UBSServerBackend::SetMatchState(const FString& InMatchId, const FString& State, int32 PlayerCount)
{
	if (!IsBackendConfigured()) return;
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("serverId"), ServerId);
	Body->SetStringField(TEXT("matchId"), InMatchId);
	Body->SetStringField(TEXT("state"), State);		// "lobby" | "in_progress" | "finished"
	Body->SetNumberField(TEXT("playerCount"), PlayerCount);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = MakeRequest(TEXT("POST"), TEXT("/server/match-state"));
	Req->SetContentAsString(SerializeJson(Body));
	Req->ProcessRequest();
}

void UBSServerBackend::ValidatePlayer(const FString& PlayerId, const FString& MatchId, TFunction<void(bool)> OnDone)
{
	if (!IsBackendConfigured()) { OnDone(true); return; }	// dev: allow everyone

	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("playerId"), PlayerId);
	Body->SetStringField(TEXT("matchId"), MatchId);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = MakeRequest(TEXT("POST"), TEXT("/server/validate-player"));
	Req->SetContentAsString(SerializeJson(Body));
	Req->OnProcessRequestComplete().BindLambda(
		[OnDone](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{
			OnDone(bOk && Res.IsValid() && Res->GetResponseCode() == 200);
		});
	Req->ProcessRequest();
}

void UBSServerBackend::SubmitRoundResults(const FString& MatchId, int32 RoundNumber, const FString& ModeName,
                                          const TArray<FBSMatchResultEntry>& Entries)
{
	if (!IsBackendConfigured()) return;

	TArray<TSharedPtr<FJsonValue>> Arr;
	for (const FBSMatchResultEntry& E : Entries)
	{
		const TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("playerId"), E.PlayerId);
		O->SetStringField(TEXT("role"), E.Role);
		O->SetNumberField(TEXT("points"), E.Points);
		O->SetNumberField(TEXT("placement"), E.Placement);
		O->SetNumberField(TEXT("catches"), E.Catches);
		O->SetBoolField(TEXT("survived"), E.bSurvived);
		O->SetNumberField(TEXT("survivalSeconds"), E.SurvivalSeconds);
		O->SetNumberField(TEXT("shotsFired"), E.ShotsFired);
		O->SetNumberField(TEXT("throwablesUsed"), E.ThrowablesUsed);
		O->SetNumberField(TEXT("distanceMeters"), E.DistanceTravelledMeters);
		Arr.Add(MakeShared<FJsonValueObject>(O));
	}

	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("matchId"), MatchId);
	Body->SetStringField(TEXT("serverId"), ServerId);
	Body->SetNumberField(TEXT("roundNumber"), RoundNumber);
	Body->SetStringField(TEXT("mode"), ModeName);
	Body->SetArrayField(TEXT("entries"), Arr);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = MakeRequest(TEXT("POST"), TEXT("/server/round-results"));
	Req->SetContentAsString(SerializeJson(Body));
	Req->OnProcessRequestComplete().BindLambda(
		[RoundNumber](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{
			if (!bOk || !Res.IsValid() || Res->GetResponseCode() != 200)
			{
				// Backend is down or rejected the payload. The round still played out correctly;
				// only persistence is lost. Logged loudly so it shows up in container logs.
				UE_LOG(LogBlindSight, Error, TEXT("Round %d results were NOT persisted (%d)"),
					RoundNumber, Res.IsValid() ? Res->GetResponseCode() : -1);
			}
		});
	Req->ProcessRequest();
}

void UBSServerBackend::RecordEvent(const FBSTelemetryEvent& Event)
{
	if (!IsBackendConfigured()) return;
	if (PendingEvents.Num() >= MaxBufferedEvents)
	{
		// Never let telemetry grow without bound if the backend is unreachable.
		PendingEvents.RemoveAt(0, PendingEvents.Num() / 2, EAllowShrinking::No);
	}
	PendingEvents.Add(Event);
}

void UBSServerBackend::FlushTelemetry()
{
	if (!IsBackendConfigured() || PendingEvents.Num() == 0) return;

	TArray<TSharedPtr<FJsonValue>> Arr;
	for (const FBSTelemetryEvent& E : PendingEvents)
	{
		const TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("event"), E.EventName);
		O->SetStringField(TEXT("matchId"), E.MatchId);
		O->SetStringField(TEXT("playerId"), E.PlayerId);
		O->SetNumberField(TEXT("roundTime"), E.RoundTime);

		const TSharedRef<FJsonObject> Nums = MakeShared<FJsonObject>();
		for (const TPair<FString, float>& KV : E.Numbers) Nums->SetNumberField(KV.Key, KV.Value);
		O->SetObjectField(TEXT("numbers"), Nums);

		const TSharedRef<FJsonObject> Strs = MakeShared<FJsonObject>();
		for (const TPair<FString, FString>& KV : E.Strings) Strs->SetStringField(KV.Key, KV.Value);
		O->SetObjectField(TEXT("strings"), Strs);

		Arr.Add(MakeShared<FJsonValueObject>(O));
	}

	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("serverId"), ServerId);
	Body->SetArrayField(TEXT("events"), Arr);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = MakeRequest(TEXT("POST"), TEXT("/server/telemetry"));
	Req->SetContentAsString(SerializeJson(Body));
	Req->ProcessRequest();

	PendingEvents.Reset();
}
