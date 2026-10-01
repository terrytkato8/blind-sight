#include "GameFramework/BSGameState.h"
#include "Sound/BSSoundEventSubsystem.h"
#include "Sound/BSSoundProfile.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

void ABSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABSGameState, Phase);
	DOREPLIFETIME(ABSGameState, RoundEndServerTime);
	DOREPLIFETIME(ABSGameState, RemainingHiders);
	DOREPLIFETIME(ABSGameState, TotalHidersThisRound);
	DOREPLIFETIME(ABSGameState, RoundNumber);
	DOREPLIFETIME(ABSGameState, HunterPlayerState);
	DOREPLIFETIME(ABSGameState, NextHunterPlayerState);
	DOREPLIFETIME(ABSGameState, ModeDisplayName);
	DOREPLIFETIME(ABSGameState, LastRoundResults);
}

float ABSGameState::GetRoundTimeRemaining() const
{
	if (RoundEndServerTime <= 0.f) return 0.f;
	return FMath::Max(0.f, RoundEndServerTime - GetServerWorldTimeSeconds());
}

void ABSGameState::SetPhase(EBSRoundPhase NewPhase)
{
	Phase = NewPhase;
	OnRep_Phase();
}

void ABSGameState::OnRep_Phase()            { OnPhaseChanged.Broadcast(Phase); }
void ABSGameState::OnRep_RemainingHiders()  { OnRemainingHidersChanged.Broadcast(RemainingHiders); }
void ABSGameState::OnRep_LastRoundResults() { OnRoundResults.Broadcast(); }

void ABSGameState::MulticastPlaySoundEvent_Implementation(EBSSoundEventType Type, FVector Location, float Intensity)
{
	if (IsRunningDedicatedServer()) return;
	UBSSoundEventSubsystem* Sub = GetWorld()->GetSubsystem<UBSSoundEventSubsystem>();
	UBSSoundProfile* Profile = Sub ? Sub->GetSoundProfile() : nullptr;
	if (!Profile) return;

	const FBSSoundEventDefinition& Def = Profile->GetDefinition(Type);
	USoundBase* Sound = Def.Sound.IsNull() ? nullptr : Def.Sound.LoadSynchronous();
	if (!Sound) return;

	// MetaSound receives Intensity + Radius so loudness/attenuation are driven by gameplay values (GDD §10).
	if (UAudioComponent* AC = UGameplayStatics::SpawnSoundAtLocation(this, Sound, Location, FRotator::ZeroRotator, 1.f, 1.f, 0.f, nullptr, nullptr, true))
	{
		AC->SetFloatParameter(TEXT("Intensity"), Intensity);
		AC->SetFloatParameter(TEXT("Radius"), Def.Radius);
	}
}
