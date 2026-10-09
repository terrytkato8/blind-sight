#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/BSTypes.h"
#include "BSGameMode.generated.h"

class ABSPlayerState;
class ABSCharacterBase;
class ABSGameState;
class UBSSoundProfile;

/**
 * Base round-based GameMode (GDD §5, §4.3). Rounds run in-map without travel:
 *   Lobby -> [StartRound] -> HeadStart -> InProgress -> RoundOver -> (rotate Hunter) -> HeadStart ...
 * Subclasses (Blackout / Manhunt) define the clock, ammo budget, end condition and scoring.
 */
UCLASS(Abstract)
class BLINDSIGHT_API ABSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABSGameMode();

	// ---- Config (override per BP child) ----
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Classes") TSubclassOf<ABSCharacterBase> HunterPawnClass;
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Classes") TSubclassOf<ABSCharacterBase> HiderPawnClass;
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Sound")   TObjectPtr<UBSSoundProfile> SoundProfileOverride;

	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Round") float HeadStartSeconds = 15.f;
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Round") float RoundOverSeconds = 12.f;
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Round") float SpectateDelaySeconds = 6.f;
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Round") float MaxThrowablesCarried = 2.f;
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Round") int32 StartingThrowables = 0;
	/** Auto-start the first round when MinPlayers have joined; otherwise call StartRound() from lobby UI / console. */
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Round") bool bAutoStartWhenReady = true;
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Round") bool bAutoContinueRounds = true;
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Round") float AutoStartDelaySeconds = 8.f;
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Round") FText ModeDisplayName;

	// ---- Round control ----
	UFUNCTION(BlueprintCallable, Category = "Blind Sight") virtual void StartRound();
	UFUNCTION(BlueprintCallable, Category = "Blind Sight") virtual void EndRound(EBSRoundEndReason Reason);

	/** Called by ABSHiderCharacter on the server when a Hider is hit. */
	virtual void HandleHiderEliminated(ABSPlayerState* HiderPS, AActor* Eliminator);

	// ---- Mode contract ----
	/** Round length in seconds; 0 = no clock. */
	virtual float GetRoundDurationSeconds() const { return 0.f; }
	/** Hunter ammo budget for this round given the Hider count. */
	virtual int32 GetHunterAmmo(int32 NumHiders) const { return 3; }
	/** Decide whether the round should end after an elimination. */
	virtual void CheckRoundEndAfterElimination() {}
	/** Produce per-player results and award points. */
	virtual TArray<FBSRoundScoreEntry> ResolveScoring(EBSRoundEndReason Reason) { return {}; }

	/** Telemetry convenience — safe to call from anywhere on the server. */
	void RecordTelemetry(const FString& EventName, const FString& PlayerId, const TMap<FString, float>& Numbers);

	UFUNCTION(BlueprintPure, Category = "Blind Sight") const FString& GetMatchId() const { return MatchId; }

	// ---- AGameModeBase ----
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override { return false; }
	
	//PlayerStart tag for differentiating between Hunter and Hider JRB 10.8.26
	UPROPERTY() TSet<TObjectPtr<AActor>> UsedStartsThisRound;

protected:
	virtual void BeginPlay() override;

	ABSGameState* GetBSGameState() const;
	void GatherPlayers(TArray<ABSPlayerState*>& Out) const;
	ABSPlayerState* PickHunter(const TArray<ABSPlayerState*>& Players, ABSPlayerState*& OutNext) const;
	void RespawnAll(const TArray<ABSPlayerState*>& Players);
	void InitRoundAttributes(ABSPlayerState* PS, int32 NumHiders);
	void BeginHunt();
	void OnRoundTimerExpired();
	void TryAutoStart();
	void AdvanceRotation();

	void ReportMatchState(const FString& State);
	void SubmitResultsToBackend(const TArray<FBSRoundScoreEntry>& Results);

	FString MatchId;
	int32 RoundNumber = 0;
	int32 HunterRotationCursor = 0;
	int32 NextRotationIndex = 0;
	int32 TotalHidersThisRound = 0;
	int32 RemainingHiders = 0;
	EBSRoundPhase Phase = EBSRoundPhase::Lobby;

	UPROPERTY() TObjectPtr<ABSPlayerState> CurrentHunterPS;

	FTimerHandle HeadStartTimer, RoundTimer, RoundOverTimer, AutoStartTimer;
};
