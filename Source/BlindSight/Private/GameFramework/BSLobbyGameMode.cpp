#include "GameFramework/BSLobbyGameMode.h"
#include "GameFramework/BSLobbyGameState.h"
#include "GameFramework/BSGameMode.h"
#include "GameFramework/BSPlayerState.h"
#include "GameFramework/BSPlayerController.h"
#include "BlindSight.h"

ABSLobbyGameMode::ABSLobbyGameMode()
{
	DefaultPawnClass = nullptr;	// set to BP_LobbyCharacter on BP_GM_Lobby
	bStartPlayersAsSpectators = false;
	bUseSeamlessTravel = true;
	GameStateClass = ABSLobbyGameState::StaticClass();
	PlayerStateClass = ABSPlayerState::StaticClass();
	PlayerControllerClass = ABSPlayerController::StaticClass();
}

void ABSLobbyGameMode::InitGameState()
{
	Super::InitGameState();
	if (ABSLobbyGameState* GS = GetGameState<ABSLobbyGameState>()) GS->SetMapOptions(MapOptions);
	RefreshLobbyState();
}

void ABSLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	RefreshLobbyState();
}

void ABSLobbyGameMode::Logout(AController* Exiting)
{
	// The leaving player's PlayerState is still in the PlayerArray at this point, so skip it explicitly.
	RefreshLobbyState(Exiting ? Exiting->GetPlayerState<APlayerState>() : nullptr);
	Super::Logout(Exiting);
}

void ABSLobbyGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);
	RefreshLobbyState();
}

void ABSLobbyGameMode::SetPlayerReady(ABSPlayerState* PS, bool bReady)
{
	if (!PS) return;
	PS->SetLobbyReady(bReady);
	RefreshLobbyState();
}

void ABSLobbyGameMode::SetPlayerMapVote(ABSPlayerState* PS, int32 MapIndex)
{
	if (!PS) return;
	// -1 clears the vote; anything else must be a real option. Never trust the index a client sends.
	if (MapIndex != INDEX_NONE && !MapOptions.IsValidIndex(MapIndex)) return;
	PS->SetLobbyMapVote(MapIndex);
	RefreshLobbyState();
}

void ABSLobbyGameMode::RefreshLobbyState(const APlayerState* Ignore)
{
	ABSLobbyGameState* GS = GetGameState<ABSLobbyGameState>();
	if (!GS) return;

	int32 PlayerCount = 0;
	int32 ReadyCount = 0;
	TArray<int32> Votes;
	Votes.Init(0, MapOptions.Num());

	for (APlayerState* Raw : GS->PlayerArray)
	{
		const ABSPlayerState* PS = Cast<ABSPlayerState>(Raw);
		if (!PS || PS == Ignore || PS->IsOnlyASpectator() || PS->IsInactive()) continue;
		++PlayerCount;
		if (PS->IsLobbyReady()) ++ReadyCount;
		if (Votes.IsValidIndex(PS->GetLobbyMapVote())) ++Votes[PS->GetLobbyMapVote()];
	}

	// Most votes wins. On a tie the current selection stays if it is one of the leaders; otherwise the first leader wins.
	int32 BestVotes = 0;
	for (const int32 V : Votes) BestVotes = FMath::Max(BestVotes, V);
	if (BestVotes > 0 && !(Votes.IsValidIndex(SelectedMapIndex) && Votes[SelectedMapIndex] == BestVotes))
	{
		SelectedMapIndex = Votes.IndexOfByKey(BestVotes);
	}
	if (!MapOptions.IsValidIndex(SelectedMapIndex)) SelectedMapIndex = 0;

	bCanStartMatch = MapOptions.IsValidIndex(SelectedMapIndex)
		&& MatchModeClass != nullptr
		&& PlayerCount >= MinPlayersToStart
		&& (!bRequireAllReady || ReadyCount == PlayerCount);

	GS->SetLobbySnapshot(PlayerCount, ReadyCount, Votes, SelectedMapIndex, bCanStartMatch, MinPlayersToStart);
}

bool ABSLobbyGameMode::RequestStartMatch(APlayerController* Requester)
{
	// Host = the listen server's own player. (Dedicated servers will need an explicit host later.)
	if (!Requester || !Requester->IsLocalController())
	{
		UE_LOG(LogBlindSight, Warning, TEXT("RequestStartMatch refused: %s is not the host."), *GetNameSafe(Requester));
		return false;
	}
	RefreshLobbyState();
	if (!bCanStartMatch)
	{
		UE_LOG(LogBlindSight, Warning, TEXT("RequestStartMatch refused: lobby not ready (check players, ready flags, MapOptions and MatchModeClass)."));
		return false;
	}
	StartMatch(MapOptions[SelectedMapIndex].MapName, MatchModeClass);
	return true;
}

void ABSLobbyGameMode::StartMatch(const FString& MapName, TSubclassOf<ABSGameMode> ModeClass)
{
	if (!ModeClass) { UE_LOG(LogBlindSight, Warning, TEXT("StartMatch: no mode class")); return; }
	const FString URL = FString::Printf(TEXT("%s?game=%s?listen"), *MapName, *ModeClass->GetPathName());
	GetWorld()->ServerTravel(URL, false);
}
