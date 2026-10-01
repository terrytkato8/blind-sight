#include "GameFramework/BSGameMode.h"
#include "GameFramework/BSGameState.h"
#include "GameFramework/BSPlayerState.h"
#include "GameFramework/BSPlayerController.h"
#include "Characters/BSCharacterBase.h"
#include "Characters/BSHunterCharacter.h"
#include "Characters/BSHiderCharacter.h"
#include "Abilities/BSAttributeSet.h"
#include "Gameplay/BSThrowableSpawnPoint.h"
#include "Sound/BSSoundEventSubsystem.h"
#include "Core/BSGameSettings.h"
#include "Backend/BSServerBackend.h"
#include "Backend/BSBackendTypes.h"
#include "BlindSight.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"

ABSGameMode::ABSGameMode()
{
	GameStateClass = ABSGameState::StaticClass();
	PlayerStateClass = ABSPlayerState::StaticClass();
	PlayerControllerClass = ABSPlayerController::StaticClass();
	HunterPawnClass = ABSHunterCharacter::StaticClass();
	HiderPawnClass = ABSHiderCharacter::StaticClass();
	DefaultPawnClass = nullptr;	// nobody spawns until a round starts
	bStartPlayersAsSpectators = true;
	ModeDisplayName = NSLOCTEXT("BlindSight", "ModeBase", "Blind Sight");
}

void ABSGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (UBSSoundEventSubsystem* Sub = GetWorld()->GetSubsystem<UBSSoundEventSubsystem>())
	{
		Sub->SetSoundProfile(SoundProfileOverride);
	}
	if (ABSGameState* GS = GetBSGameState()) GS->SetModeDisplayName(ModeDisplayName);
}

ABSGameState* ABSGameMode::GetBSGameState() const { return GetGameState<ABSGameState>(); }

// ---------------- Backend / match lifecycle ----------------
void ABSGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Matchmaking passes ?MatchId=... when it allocates this container to a match.
	MatchId = UGameplayStatics::ParseOption(Options, TEXT("MatchId"));
	if (MatchId.IsEmpty()) MatchId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);

	ReportMatchState(TEXT("lobby"));
	UE_LOG(LogBlindSight, Log, TEXT("Match %s starting on map %s"), *MatchId, *MapName);
}

FString ABSGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	const FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	// The client sends its backend account id as a join option; the dedicated server then
	// asks the backend whether that player was actually assigned to this match.
	if (ABSPlayerState* PS = NewPlayerController ? NewPlayerController->GetPlayerState<ABSPlayerState>() : nullptr)
	{
		PS->SetBackendPlayerId(UGameplayStatics::ParseOption(Options, TEXT("PlayerId")));
	}
	return Result;
}

void ABSGameMode::ReportMatchState(const FString& State)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBSServerBackend* Backend = GI->GetSubsystem<UBSServerBackend>())
		{
			Backend->SetMatchState(MatchId, State, GetNumPlayers());
		}
	}
}

void ABSGameMode::RecordTelemetry(const FString& EventName, const FString& PlayerId, const TMap<FString, float>& Numbers)
{
	UGameInstance* GI = GetGameInstance();
	UBSServerBackend* Backend = GI ? GI->GetSubsystem<UBSServerBackend>() : nullptr;
	if (!Backend) return;

	FBSTelemetryEvent Ev;
	Ev.EventName = EventName;
	Ev.MatchId = MatchId;
	Ev.PlayerId = PlayerId;
	Ev.RoundTime = GetWorld()->GetTimeSeconds();
	Ev.Numbers = Numbers;
	Ev.Strings.Add(TEXT("mode"), ModeDisplayName.ToString());
	Backend->RecordEvent(Ev);
}

void ABSGameMode::SubmitResultsToBackend(const TArray<FBSRoundScoreEntry>& Results)
{
	UGameInstance* GI = GetGameInstance();
	UBSServerBackend* Backend = GI ? GI->GetSubsystem<UBSServerBackend>() : nullptr;
	if (!Backend) return;

	TArray<ABSPlayerState*> Players; GatherPlayers(Players);
	TArray<FBSMatchResultEntry> Entries;

	for (ABSPlayerState* PS : Players)
	{
		if (PS->GetBackendPlayerId().IsEmpty()) continue;	// offline / unauthenticated player

		FBSMatchResultEntry E;
		E.PlayerId        = PS->GetBackendPlayerId();
		E.Role            = (PS->GetBSRole() == EBSRole::Hunter) ? TEXT("hunter") : TEXT("hider");
		E.Placement       = PS->GetPlacement();
		E.Catches         = PS->GetCatchesThisRound();
		E.bSurvived       = !PS->IsEliminated();
		E.SurvivalSeconds = PS->GetSurvivalSeconds();
		E.ShotsFired      = PS->GetShotsFiredThisRound();
		E.ThrowablesUsed  = PS->GetThrowablesUsedThisRound();

		// Points come from the authoritative scoring pass, not from the PlayerState total.
		for (const FBSRoundScoreEntry& R : Results)
		{
			if (R.PlayerName == PS->GetPlayerName()) { E.Points = R.PointsAwarded; break; }
		}
		Entries.Add(E);
	}

	if (Entries.Num() > 0)
	{
		Backend->SubmitRoundResults(MatchId, RoundNumber, ModeDisplayName.ToString(), Entries);
	}
}

// ---------------- Players ----------------
void ABSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (ABSPlayerState* PS = NewPlayer->GetPlayerState<ABSPlayerState>())
	{
		PS->SetRotationIndex(NextRotationIndex++);
		PS->SetBSRole(EBSRole::None);
	}
	if (Phase == EBSRoundPhase::Lobby && bAutoStartWhenReady) TryAutoStart();
}

void ABSGameMode::Logout(AController* Exiting)
{
	if (ABSPlayerState* PS = Exiting ? Exiting->GetPlayerState<ABSPlayerState>() : nullptr)
	{
		if (Phase == EBSRoundPhase::InProgress || Phase == EBSRoundPhase::HeadStart)
		{
			if (PS == CurrentHunterPS)
			{
				EndRound(EBSRoundEndReason::Aborted);	// no Hunter, no hunt
			}
			else if (PS->GetBSRole() == EBSRole::Hider && !PS->IsEliminated())
			{
				HandleHiderEliminated(PS, nullptr);		// leaving counts as caught for round bookkeeping
			}
		}
	}
	Super::Logout(Exiting);
}

void ABSGameMode::TryAutoStart()
{
	const int32 Min = GetDefault<UBSGameSettings>()->MinPlayersToStart;
	if (GetNumPlayers() >= Min && !GetWorldTimerManager().IsTimerActive(AutoStartTimer))
	{
		GetWorldTimerManager().SetTimer(AutoStartTimer, this, &ABSGameMode::StartRound, AutoStartDelaySeconds, false);
	}
}

void ABSGameMode::GatherPlayers(TArray<ABSPlayerState*>& Out) const
{
	Out.Reset();
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (ABSPlayerState* BPS = Cast<ABSPlayerState>(PS)) Out.Add(BPS);
	}
	Out.Sort([](const ABSPlayerState& A, const ABSPlayerState& B) { return A.GetRotationIndex() < B.GetRotationIndex(); });
}

ABSPlayerState* ABSGameMode::PickHunter(const TArray<ABSPlayerState*>& Players, ABSPlayerState*& OutNext) const
{
	// Sequential rotation (GDD §4.3, MVP recommendation): "you're up next" is always knowable.
	if (Players.Num() == 0) { OutNext = nullptr; return nullptr; }
	const int32 Idx = HunterRotationCursor % Players.Num();
	OutNext = Players[(Idx + 1) % Players.Num()];
	return Players[Idx];
}

void ABSGameMode::AdvanceRotation() { ++HunterRotationCursor; }

UClass* ABSGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	const ABSPlayerState* PS = InController ? InController->GetPlayerState<ABSPlayerState>() : nullptr;
	if (!PS) return nullptr;
	switch (PS->GetBSRole())
	{
	case EBSRole::Hunter: return HunterPawnClass;
	case EBSRole::Hider:  return HiderPawnClass;
	default:              return nullptr;
	}
}

AActor* ABSGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Tag PlayerStarts "Hunter" or "Hider" in the level; untagged starts are used as fallback for either.
	const ABSPlayerState* PS = Player ? Player->GetPlayerState<ABSPlayerState>() : nullptr;
	const FName Want = (PS && PS->GetBSRole() == EBSRole::Hunter) ? FName("Hunter") : FName("Hider");

	TArray<APlayerStart*> Preferred, Fallback;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerStartTag == Want) Preferred.Add(*It);
		else if (It->PlayerStartTag.IsNone()) Fallback.Add(*It);
	}
	if (Preferred.Num()) return Preferred[FMath::RandRange(0, Preferred.Num() - 1)];
	if (Fallback.Num())  return Fallback[FMath::RandRange(0, Fallback.Num() - 1)];
	return Super::ChoosePlayerStart_Implementation(Player);
}

// ---------------- Round flow ----------------
void ABSGameMode::StartRound()
{
	if (Phase == EBSRoundPhase::HeadStart || Phase == EBSRoundPhase::InProgress) return;
	GetWorldTimerManager().ClearTimer(AutoStartTimer);
	GetWorldTimerManager().ClearTimer(RoundOverTimer);

	TArray<ABSPlayerState*> Players;
	GatherPlayers(Players);
	if (Players.Num() < 2)
	{
		UE_LOG(LogBlindSight, Warning, TEXT("StartRound: need at least 2 players (have %d)."), Players.Num());
		return;
	}

	++RoundNumber;
	ABSPlayerState* NextPS = nullptr;
	CurrentHunterPS = PickHunter(Players, NextPS);
	CurrentHunterPS->IncrementHunterTurns();

	TotalHidersThisRound = Players.Num() - 1;
	RemainingHiders = TotalHidersThisRound;

	for (ABSPlayerState* PS : Players)
	{
		PS->ResetForRound();
		PS->SetBSRole(PS == CurrentHunterPS ? EBSRole::Hunter : EBSRole::Hider);
	}

	ABSGameState* GS = GetBSGameState();
	GS->SetRoundNumber(RoundNumber);
	GS->SetHunter(CurrentHunterPS, NextPS);
	GS->SetHiderCounts(RemainingHiders, TotalHidersThisRound);
	GS->SetRoundTimer(0.f);

	for (TActorIterator<ABSThrowableSpawnPoint> It(GetWorld()); It; ++It) It->ResetStock();

	RespawnAll(Players);
	for (ABSPlayerState* PS : Players) InitRoundAttributes(PS, TotalHidersThisRound);

	// Hunter is frozen (blind + still) while Hiders scatter (GDD §5 step 2).
	if (ABSCharacterBase* HunterChar = Cast<ABSCharacterBase>(CurrentHunterPS->GetPawn())) HunterChar->SetHeadStartFrozen(true);

	Phase = EBSRoundPhase::HeadStart;
	GS->SetPhase(Phase);
	ReportMatchState(TEXT("in_progress"));
	RecordTelemetry(TEXT("round_start"), CurrentHunterPS->GetBackendPlayerId(),
		{ {TEXT("round"), float(RoundNumber)}, {TEXT("hiders"), float(TotalHidersThisRound)},
		  {TEXT("hunterAmmo"), float(GetHunterAmmo(TotalHidersThisRound))} });
	GetWorldTimerManager().SetTimer(HeadStartTimer, this, &ABSGameMode::BeginHunt, HeadStartSeconds, false);
	UE_LOG(LogBlindSight, Log, TEXT("Round %d started. Hunter=%s Hiders=%d"), RoundNumber, *CurrentHunterPS->GetPlayerName(), TotalHidersThisRound);
}

void ABSGameMode::RespawnAll(const TArray<ABSPlayerState*>& Players)
{
	for (ABSPlayerState* PS : Players)
	{
		AController* C = PS->GetOwningController();
		if (!C) continue;
		if (APawn* Old = C->GetPawn()) { C->UnPossess(); Old->Destroy(); }
		RestartPlayer(C);
		if (ABSPlayerController* PC = Cast<ABSPlayerController>(C)) PC->ClientOnRoleAssigned(PS->GetBSRole());
	}
}

void ABSGameMode::InitRoundAttributes(ABSPlayerState* PS, int32 NumHiders)
{
	UBSAttributeSet* Attr = PS->GetAttributeSet();
	if (!Attr) return;
	if (PS->GetBSRole() == EBSRole::Hunter)
	{
		const float Ammo = float(GetHunterAmmo(NumHiders));
		Attr->SetMaxAmmo(Ammo); Attr->SetAmmo(Ammo);
		Attr->SetMaxThrowables(0.f); Attr->SetThrowables(0.f);
	}
	else
	{
		Attr->SetMaxAmmo(0.f); Attr->SetAmmo(0.f);
		Attr->SetMaxThrowables(MaxThrowablesCarried);
		Attr->SetThrowables(FMath::Clamp(float(StartingThrowables), 0.f, MaxThrowablesCarried));
	}
}

void ABSGameMode::BeginHunt()
{
	if (Phase != EBSRoundPhase::HeadStart) return;
	if (ABSCharacterBase* HunterChar = Cast<ABSCharacterBase>(CurrentHunterPS ? CurrentHunterPS->GetPawn() : nullptr)) HunterChar->SetHeadStartFrozen(false);

	Phase = EBSRoundPhase::InProgress;
	ABSGameState* GS = GetBSGameState();
	GS->SetPhase(Phase);

	const float Duration = GetRoundDurationSeconds();
	if (Duration > 0.f)
	{
		GS->SetRoundTimer(GS->GetServerWorldTimeSeconds() + Duration);
		GetWorldTimerManager().SetTimer(RoundTimer, this, &ABSGameMode::OnRoundTimerExpired, Duration, false);
	}
}

void ABSGameMode::OnRoundTimerExpired()
{
	if (Phase == EBSRoundPhase::InProgress) EndRound(EBSRoundEndReason::TimerExpired);
}

void ABSGameMode::HandleHiderEliminated(ABSPlayerState* HiderPS, AActor* Eliminator)
{
	if (!HiderPS || (Phase != EBSRoundPhase::InProgress && Phase != EBSRoundPhase::HeadStart)) return;

	// Placement: first caught = last place (TotalHiders), last standing = 1.
	HiderPS->SetPlacement(RemainingHiders);
	HiderPS->SetEliminated(true);
	HiderPS->SetSurvivalSeconds(GetWorld()->GetTimeSeconds());
	RecordTelemetry(TEXT("hider_eliminated"), HiderPS->GetBackendPlayerId(),
		{ {TEXT("placement"), float(HiderPS->GetPlacement())},
		  {TEXT("remaining"), float(RemainingHiders - 1)},
		  {TEXT("survivalSeconds"), HiderPS->GetSurvivalSeconds()} });
	RemainingHiders = FMath::Max(0, RemainingHiders - 1);
	if (CurrentHunterPS && Eliminator) CurrentHunterPS->AddCatch();
	GetBSGameState()->SetRemainingHiders(RemainingHiders);

	if (ABSPlayerController* PC = Cast<ABSPlayerController>(HiderPS->GetOwningController()))
	{
		PC->ClientBeginSpectate(SpectateDelaySeconds);
	}
	CheckRoundEndAfterElimination();
}

void ABSGameMode::EndRound(EBSRoundEndReason Reason)
{
	if (Phase == EBSRoundPhase::RoundOver || Phase == EBSRoundPhase::Lobby) return;
	GetWorldTimerManager().ClearTimer(HeadStartTimer);
	GetWorldTimerManager().ClearTimer(RoundTimer);

	Phase = EBSRoundPhase::RoundOver;
	ABSGameState* GS = GetBSGameState();
	GS->SetRoundTimer(0.f);

	TArray<FBSRoundScoreEntry> Results = ResolveScoring(Reason);
	GS->SetLastRoundResults(Results);

	RecordTelemetry(TEXT("round_end"), CurrentHunterPS ? CurrentHunterPS->GetBackendPlayerId() : FString(),
		{ {TEXT("round"), float(RoundNumber)}, {TEXT("reason"), float(int32(Reason))},
		  {TEXT("survivors"), float(RemainingHiders)}, {TEXT("totalHiders"), float(TotalHidersThisRound)},
		  {TEXT("durationSeconds"), float(GetWorld()->GetTimeSeconds())} });
	SubmitResultsToBackend(Results);
	GS->SetPhase(Phase);

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ABSPlayerController* PC = Cast<ABSPlayerController>(It->Get())) PC->ClientShowResults();
	}

	AdvanceRotation();

	// A dedicated server hosts a fixed number of rounds, then returns itself to the pool.
	const int32 RoundsPerMatch = GetDefault<UBSGameSettings>()->RoundsPerMatch;
	if (RoundNumber >= RoundsPerMatch)
	{
		ReportMatchState(TEXT("finished"));
		if (UBSServerBackend* B = GetGameInstance() ? GetGameInstance()->GetSubsystem<UBSServerBackend>() : nullptr)
		{
			B->FlushTelemetry();
		}
		UE_LOG(LogBlindSight, Log, TEXT("Match %s complete after %d rounds."), *MatchId, RoundNumber);
		return;
	}

	if (bAutoContinueRounds)
	{
		GetWorldTimerManager().SetTimer(RoundOverTimer, this, &ABSGameMode::StartRound, RoundOverSeconds, false);
	}
	UE_LOG(LogBlindSight, Log, TEXT("Round %d ended (%d). Remaining hiders=%d"), RoundNumber, int32(Reason), RemainingHiders);
}
