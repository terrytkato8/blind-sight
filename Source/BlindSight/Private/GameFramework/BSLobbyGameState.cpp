#include "GameFramework/BSLobbyGameState.h"
#include "Net/UnrealNetwork.h"

void ABSLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABSLobbyGameState, MapOptions);
	DOREPLIFETIME(ABSLobbyGameState, MapVotes);
	DOREPLIFETIME(ABSLobbyGameState, SelectedMapIndex);
	DOREPLIFETIME(ABSLobbyGameState, LobbyPlayerCount);
	DOREPLIFETIME(ABSLobbyGameState, ReadyCount);
	DOREPLIFETIME(ABSLobbyGameState, MinPlayersToStart);
	DOREPLIFETIME(ABSLobbyGameState, bCanStartMatch);
}

FText ABSLobbyGameState::GetSelectedMapDisplayName() const
{
	return MapOptions.IsValidIndex(SelectedMapIndex) ? MapOptions[SelectedMapIndex].DisplayName : FText::GetEmpty();
}

void ABSLobbyGameState::SetMapOptions(const TArray<FBSLobbyMapOption>& InOptions)
{
	if (!HasAuthority()) return;
	MapOptions = InOptions;
	OnRep_LobbyState();	// listen server / local
}

void ABSLobbyGameState::SetLobbySnapshot(int32 InPlayerCount, int32 InReadyCount, const TArray<int32>& InVotes, int32 InSelectedMapIndex, bool bInCanStart, int32 InMinPlayers)
{
	if (!HasAuthority()) return;
	LobbyPlayerCount = InPlayerCount;
	ReadyCount = InReadyCount;
	MapVotes = InVotes;
	SelectedMapIndex = InSelectedMapIndex;
	bCanStartMatch = bInCanStart;
	MinPlayersToStart = InMinPlayers;
	OnRep_LobbyState();	// listen server / local
}

void ABSLobbyGameState::OnRep_LobbyState() { OnLobbyStateChanged.Broadcast(); }
