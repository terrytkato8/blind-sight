#include "Sound/BSSoundEventSubsystem.h"
#include "Sound/BSSoundProfile.h"
#include "GameFramework/BSPlayerController.h"
#include "GameFramework/BSPlayerState.h"
#include "GameFramework/BSGameState.h"
#include "Core/BSGameSettings.h"
#include "BlindSight.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

void UBSSoundEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Default profile from project settings; GameMode may override at round start.
	if (const UBSGameSettings* Settings = GetDefault<UBSGameSettings>())
	{
		if (!Settings->DefaultSoundProfile.IsNull())
		{
			SoundProfile = Settings->DefaultSoundProfile.LoadSynchronous();
		}
	}
	if (!SoundProfile)
	{
		SoundProfile = NewObject<UBSSoundProfile>(this, TEXT("TransientSoundProfile"));
		UE_LOG(LogBlindSight, Warning, TEXT("No DefaultSoundProfile set in Project Settings > Blind Sight; using code defaults."));
	}
}

bool UBSSoundEventSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UBSSoundEventSubsystem::SetSoundProfile(UBSSoundProfile* InProfile)
{
	if (InProfile) SoundProfile = InProfile;
}

void UBSSoundEventSubsystem::EmitSound(UObject* WorldContextObject, EBSSoundEventType Type, FVector Origin, AActor* Instigator, float IntensityScale)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World) return;
	UBSSoundEventSubsystem* Sub = World->GetSubsystem<UBSSoundEventSubsystem>();
	if (!Sub || !Sub->SoundProfile) return;

	const FBSSoundEventDefinition& Def = Sub->SoundProfile->GetDefinition(Type);
	FBSSoundEvent Ev;
	Ev.Type = Type;
	Ev.Origin = Origin;
	Ev.Radius = Def.Radius;
	Ev.Intensity = FMath::Clamp(Def.Intensity * IntensityScale, 0.f, 1.f);
	Ev.PingLifetime = Def.PingLifetime;
	Ev.Instigator = Instigator;
	Sub->EmitSoundEvent(Ev);
}

void UBSSoundEventSubsystem::EmitSoundEvent(const FBSSoundEvent& Event)
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client) return;	// server-authoritative
	if (Event.Radius <= 0.f || Event.Intensity <= 0.f) return;

	OnSoundEmitted.Broadcast(Event);

	// 1) Audio for everyone (Hiders hear the world too; the gunshot alerting Hiders is a GDD feature).
	if (ABSGameState* GS = World->GetGameState<ABSGameState>())
	{
		GS->MulticastPlaySoundEvent(Event.Type, Event.Origin, Event.Intensity);
	}

	// 2) Perception pings for Hunters only.
	const float ServerTime = World->GetTimeSeconds();
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ABSPlayerController* PC = Cast<ABSPlayerController>(It->Get());
		if (!PC) continue;
		ABSPlayerState* PS = PC->GetPlayerState<ABSPlayerState>();
		if (!PS || PS->GetBSRole() != EBSRole::Hunter || PS->IsEliminated()) continue;
		APawn* Pawn = PC->GetPawn();
		if (!Pawn) continue;

		FVector ListenerLoc = Pawn->GetActorLocation() + FVector(0.f, 0.f, 60.f);	// ear height

		bool bOccluded = false;
		const float Perceived = ComputePerceivedIntensity(Event, ListenerLoc, bOccluded);
		if (Perceived < SoundProfile->MinPerceivedIntensity) continue;

		FBSSoundPing Ping;
		Ping.Type = Event.Type;
		Ping.Location = Event.Origin;
		Ping.PerceivedIntensity = Perceived;
		Ping.Lifetime = Event.PingLifetime;
		Ping.bOccluded = bOccluded;
		Ping.ServerTime = ServerTime;
		PC->ClientReceiveSoundPing(Ping);
	}
}

float UBSSoundEventSubsystem::ComputePerceivedIntensity(const FBSSoundEvent& Event, const FVector& ListenerLocation, bool& bOutOccluded) const
{
	const float Dist = FVector::Dist(Event.Origin, ListenerLocation);
	if (Dist >= Event.Radius) { bOutOccluded = false; return 0.f; }

	const float Norm = 1.f - (Dist / Event.Radius);
	float Perceived = Event.Intensity * FMath::Pow(Norm, SoundProfile->FalloffExponent);

	const int32 Surfaces = CountBlockingSurfaces(Event.Origin, ListenerLocation, Event.Instigator.Get(), nullptr);
	bOutOccluded = Surfaces > 0;
	if (Surfaces > 0)
	{
		Perceived *= SoundProfile->OcclusionFactor;
		for (int32 i = 1; i < Surfaces; ++i) Perceived *= SoundProfile->PerExtraSurfaceFactor;
	}
	return FMath::Clamp(Perceived, 0.f, 1.f);
}

int32 UBSSoundEventSubsystem::CountBlockingSurfaces(const FVector& From, const FVector& To, AActor* IgnoreA, AActor* IgnoreB) const
{
	// Approximates "thick wall vs thin wall" by counting distinct blocking hits along the line.
	// Level designers can also add Audio Volumes / custom occlusion channels later (GDD §8, §10).
	UWorld* World = GetWorld();
	if (!World) return 0;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BSSoundOcclusion), false);
	Params.bIgnoreTouches = true;
	if (IgnoreA) Params.AddIgnoredActor(IgnoreA);
	if (IgnoreB) Params.AddIgnoredActor(IgnoreB);
	// Never let pawns occlude sound.
	for (TActorIterator<APawn> It(World); It; ++It) Params.AddIgnoredActor(*It);

	int32 Count = 0;
	FVector Start = From;
	const FVector Dir = (To - From).GetSafeNormal();
	for (int32 Step = 0; Step < 4; ++Step)	// cap at 4 surfaces
	{
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, Start, To, ECC_Visibility, Params)) break;
		++Count;
		Start = Hit.ImpactPoint + Dir * 10.f;
		if (FVector::DistSquared(Start, To) < 100.f) break;
	}
	return Count;
}
