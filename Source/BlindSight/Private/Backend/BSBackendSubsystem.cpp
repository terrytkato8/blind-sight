#include "Backend/BSBackendSubsystem.h"
#include "Core/BSGameSettings.h"
#include "BlindSight.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

static TSharedPtr<FJsonObject> ParseJson(const FString& Body)
{
	TSharedPtr<FJsonObject> Obj;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	return FJsonSerializer::Deserialize(Reader, Obj) ? Obj : nullptr;
}

static FString ToJsonString(const TSharedRef<FJsonObject>& Obj)
{
	FString Out;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Obj, Writer);
	return Out;
}

static void ReadProfile(const TSharedPtr<FJsonObject>& J, FBSPlayerProfile& Out)
{
	if (!J.IsValid()) return;
	Out.PlayerId          = J->GetStringField(TEXT("playerId"));
	Out.DisplayName       = J->GetStringField(TEXT("displayName"));
	Out.Level             = J->GetIntegerField(TEXT("level"));
	Out.Experience        = J->GetIntegerField(TEXT("experience"));
	Out.Currency          = J->GetIntegerField(TEXT("currency"));
	Out.MatchesPlayed     = J->GetIntegerField(TEXT("matchesPlayed"));
	Out.RoundsAsHunter    = J->GetIntegerField(TEXT("roundsAsHunter"));
	Out.TotalCatches      = J->GetIntegerField(TEXT("totalCatches"));
	Out.TotalSurvivals    = J->GetIntegerField(TEXT("totalSurvivals"));
	J->TryGetStringField(TEXT("equippedHunterSkin"), Out.EquippedHunterSkin);
	J->TryGetStringField(TEXT("equippedHiderSkin"), Out.EquippedHiderSkin);
	Out.UnlockedCosmetics.Reset();
	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
	if (J->TryGetArrayField(TEXT("unlockedCosmetics"), Arr))
	{
		for (const TSharedPtr<FJsonValue>& V : *Arr) Out.UnlockedCosmetics.Add(V->AsString());
	}
	Out.bValid = true;
}

void UBSBackendSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BaseUrl = GetDefault<UBSGameSettings>()->BackendBaseUrl;
	BaseUrl.RemoveFromEnd(TEXT("/"));
	UE_LOG(LogBlindSight, Log, TEXT("Backend base URL: %s"), *BaseUrl);
}

void UBSBackendSubsystem::Deinitialize()
{
	if (UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		W->GetTimerManager().ClearTimer(PollTimer);
	}
	Super::Deinitialize();
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> UBSBackendSubsystem::MakeRequest(const FString& Verb, const FString& Path, bool bAuthed) const
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(BaseUrl + Path);
	Req->SetVerb(Verb);
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (bAuthed && !PlayerToken.IsEmpty())
	{
		Req->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + PlayerToken);
	}
	Req->SetTimeout(10.f);
	return Req;
}

void UBSBackendSubsystem::Login(const FString& PlatformIdToken, const FString& PlatformType)
{
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("platform"), PlatformType);
	Body->SetStringField(TEXT("idToken"), PlatformIdToken);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = MakeRequest(TEXT("POST"), TEXT("/auth/login"), false);
	Req->SetContentAsString(ToJsonString(Body));
	Req->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{
			if (!bOk || !Res.IsValid() || Res->GetResponseCode() != 200)
			{
				UE_LOG(LogBlindSight, Warning, TEXT("Backend login failed (%d)"), Res.IsValid() ? Res->GetResponseCode() : -1);
				OnLoginComplete.Broadcast(false);
				return;
			}
			const TSharedPtr<FJsonObject> J = ParseJson(Res->GetContentAsString());
			if (!J.IsValid()) { OnLoginComplete.Broadcast(false); return; }
			PlayerToken = J->GetStringField(TEXT("token"));
			const TSharedPtr<FJsonObject>* P = nullptr;
			if (J->TryGetObjectField(TEXT("profile"), P)) ReadProfile(*P, CachedProfile);
			OnLoginComplete.Broadcast(true);
			OnProfileUpdated.Broadcast(CachedProfile);
		});
	Req->ProcessRequest();
}

void UBSBackendSubsystem::FetchProfile()
{
	if (!IsLoggedIn()) return;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = MakeRequest(TEXT("GET"), TEXT("/profile/me"));
	Req->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{
			if (bOk && Res.IsValid() && Res->GetResponseCode() == 200)
			{
				ReadProfile(ParseJson(Res->GetContentAsString()), CachedProfile);
				OnProfileUpdated.Broadcast(CachedProfile);
			}
		});
	Req->ProcessRequest();
}

void UBSBackendSubsystem::EquipCosmetic(const FString& Slot, const FString& CosmeticId)
{
	if (!IsLoggedIn()) return;
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("slot"), Slot);
	Body->SetStringField(TEXT("cosmeticId"), CosmeticId);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = MakeRequest(TEXT("POST"), TEXT("/profile/equip"));
	Req->SetContentAsString(ToJsonString(Body));
	Req->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{
			if (bOk && Res.IsValid() && Res->GetResponseCode() == 200)
			{
				ReadProfile(ParseJson(Res->GetContentAsString()), CachedProfile);
				OnProfileUpdated.Broadcast(CachedProfile);
			}
		});
	Req->ProcessRequest();
}

void UBSBackendSubsystem::PurchaseCosmetic(const FString& CosmeticId)
{
	if (!IsLoggedIn()) return;
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("cosmeticId"), CosmeticId);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = MakeRequest(TEXT("POST"), TEXT("/shop/purchase"));
	Req->SetContentAsString(ToJsonString(Body));
	Req->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{
			// 402 = insufficient funds, 409 = already owned. Backend is authoritative on both.
			if (bOk && Res.IsValid() && Res->GetResponseCode() == 200)
			{
				ReadProfile(ParseJson(Res->GetContentAsString()), CachedProfile);
				OnProfileUpdated.Broadcast(CachedProfile);
			}
			else
			{
				UE_LOG(LogBlindSight, Warning, TEXT("Purchase rejected (%d)"), Res.IsValid() ? Res->GetResponseCode() : -1);
			}
		});
	Req->ProcessRequest();
}

// ---------------- Matchmaking ----------------
void UBSBackendSubsystem::EnterQueue(const FString& ModeName, const FString& Region, const FString& PartyId)
{
	if (!IsLoggedIn()) { SetTicketState(EBSTicketState::Failed); return; }

	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("mode"), ModeName);
	Body->SetStringField(TEXT("region"), Region);
	if (!PartyId.IsEmpty()) Body->SetStringField(TEXT("partyId"), PartyId);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = MakeRequest(TEXT("POST"), TEXT("/matchmaking/ticket"));
	Req->SetContentAsString(ToJsonString(Body));
	Req->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{
			const TSharedPtr<FJsonObject> J = (bOk && Res.IsValid()) ? ParseJson(Res->GetContentAsString()) : nullptr;
			if (!J.IsValid() || Res->GetResponseCode() != 200) { SetTicketState(EBSTicketState::Failed); return; }
			TicketId = J->GetStringField(TEXT("ticketId"));
			SetTicketState(EBSTicketState::Queued);

			if (UWorld* W = GetGameInstance()->GetWorld())
			{
				W->GetTimerManager().SetTimer(PollTimer, FTimerDelegate::CreateUObject(this, &UBSBackendSubsystem::PollTicket), 2.f, true);
			}
		});
	Req->ProcessRequest();
}

void UBSBackendSubsystem::PollTicket()
{
	if (TicketId.IsEmpty()) return;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = MakeRequest(TEXT("GET"), TEXT("/matchmaking/ticket/") + TicketId);
	Req->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr, FHttpResponsePtr Res, bool bOk)
		{
			const TSharedPtr<FJsonObject> J = (bOk && Res.IsValid()) ? ParseJson(Res->GetContentAsString()) : nullptr;
			if (!J.IsValid()) return;

			const FString State = J->GetStringField(TEXT("state"));
			if (State == TEXT("matched"))
			{
				PendingAssignment.MatchId       = J->GetStringField(TEXT("matchId"));
				PendingAssignment.ServerAddress = J->GetStringField(TEXT("serverAddress"));
				J->TryGetStringField(TEXT("region"), PendingAssignment.Region);
				J->TryGetStringField(TEXT("mode"), PendingAssignment.ModeName);
				J->TryGetStringField(TEXT("map"), PendingAssignment.MapName);
				PendingAssignment.bReady = true;

				if (UWorld* W = GetGameInstance()->GetWorld()) W->GetTimerManager().ClearTimer(PollTimer);
				SetTicketState(EBSTicketState::Matched);
				OnMatchFound.Broadcast(PendingAssignment);
			}
			else if (State == TEXT("failed") || State == TEXT("expired"))
			{
				if (UWorld* W = GetGameInstance()->GetWorld()) W->GetTimerManager().ClearTimer(PollTimer);
				SetTicketState(EBSTicketState::Failed);
			}
		});
	Req->ProcessRequest();
}

void UBSBackendSubsystem::LeaveQueue()
{
	if (UWorld* W = GetGameInstance()->GetWorld()) W->GetTimerManager().ClearTimer(PollTimer);
	if (!TicketId.IsEmpty())
	{
		MakeRequest(TEXT("DELETE"), TEXT("/matchmaking/ticket/") + TicketId)->ProcessRequest();
		TicketId.Empty();
	}
	SetTicketState(EBSTicketState::Cancelled);
}

void UBSBackendSubsystem::JoinAssignedMatch()
{
	if (!PendingAssignment.bReady || PendingAssignment.ServerAddress.IsEmpty()) return;
	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
	{
		// The match id rides along as a URL option so the server can tie this connection to the ticket.
		const FString Url = FString::Printf(TEXT("%s?MatchId=%s"), *PendingAssignment.ServerAddress, *PendingAssignment.MatchId);
		PC->ClientTravel(Url, TRAVEL_Absolute);
	}
}

void UBSBackendSubsystem::SetTicketState(EBSTicketState NewState)
{
	if (TicketState == NewState) return;
	TicketState = NewState;
	OnTicketStateChanged.Broadcast(TicketState);
}
