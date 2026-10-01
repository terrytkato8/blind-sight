#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/BSTypes.h"
#include "BSGameState.generated.h"

class ABSPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnPhaseChanged, EBSRoundPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnRemainingHidersChanged, int32, Remaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBSOnRoundResults);

/** Replicated round state every client needs for HUD/results (GDD §9). */
UCLASS()
class BLINDSIGHT_API ABSGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Blind Sight") EBSRoundPhase GetPhase() const { return Phase; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") int32 GetRemainingHiders() const { return RemainingHiders; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") int32 GetTotalHiders() const { return TotalHidersThisRound; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") int32 GetRoundNumber() const { return RoundNumber; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") ABSPlayerState* GetHunterPlayerState() const { return HunterPlayerState; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") ABSPlayerState* GetNextHunterPlayerState() const { return NextHunterPlayerState; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") FText GetModeDisplayName() const { return ModeDisplayName; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") bool HasRoundTimer() const { return RoundEndServerTime > 0.f; }
	/** Seconds left on the round clock (Blackout). 0 if no timer. */
	UFUNCTION(BlueprintPure, Category = "Blind Sight") float GetRoundTimeRemaining() const;
	UFUNCTION(BlueprintPure, Category = "Blind Sight") const TArray<FBSRoundScoreEntry>& GetLastRoundResults() const { return LastRoundResults; }

	// Server API (called by GameMode)
	void SetPhase(EBSRoundPhase NewPhase);
	void SetRoundTimer(float EndServerTime) { RoundEndServerTime = EndServerTime; }
	void SetHiderCounts(int32 Remaining, int32 Total) { RemainingHiders = Remaining; TotalHidersThisRound = Total; OnRep_RemainingHiders(); }
	void SetRemainingHiders(int32 Remaining) { RemainingHiders = Remaining; OnRep_RemainingHiders(); }
	void SetRoundNumber(int32 N) { RoundNumber = N; }
	void SetHunter(ABSPlayerState* PS, ABSPlayerState* NextPS) { HunterPlayerState = PS; NextHunterPlayerState = NextPS; }
	void SetModeDisplayName(const FText& T) { ModeDisplayName = T; }
	void SetLastRoundResults(const TArray<FBSRoundScoreEntry>& R) { LastRoundResults = R; OnRep_LastRoundResults(); }

	/** Plays the profiled audio for a sound event on every client (Hiders hear the world; Hunters get pings separately). */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlaySoundEvent(EBSSoundEventType Type, FVector Location, float Intensity);

	UPROPERTY(BlueprintAssignable) FBSOnPhaseChanged OnPhaseChanged;
	UPROPERTY(BlueprintAssignable) FBSOnRemainingHidersChanged OnRemainingHidersChanged;
	UPROPERTY(BlueprintAssignable) FBSOnRoundResults OnRoundResults;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Phase) EBSRoundPhase Phase = EBSRoundPhase::Lobby;
	UPROPERTY(Replicated) float RoundEndServerTime = 0.f;
	UPROPERTY(ReplicatedUsing = OnRep_RemainingHiders) int32 RemainingHiders = 0;
	UPROPERTY(Replicated) int32 TotalHidersThisRound = 0;
	UPROPERTY(Replicated) int32 RoundNumber = 0;
	UPROPERTY(Replicated) TObjectPtr<ABSPlayerState> HunterPlayerState;
	UPROPERTY(Replicated) TObjectPtr<ABSPlayerState> NextHunterPlayerState;
	UPROPERTY(Replicated) FText ModeDisplayName;
	UPROPERTY(ReplicatedUsing = OnRep_LastRoundResults) TArray<FBSRoundScoreEntry> LastRoundResults;

	UFUNCTION() void OnRep_Phase();
	UFUNCTION() void OnRep_RemainingHiders();
	UFUNCTION() void OnRep_LastRoundResults();
};
