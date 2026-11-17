#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BSLobbyGameState.generated.h"

/** One map the lobby can send the match to. Filled in on BP_GM_Lobby. */
USTRUCT(BlueprintType)
struct FBSLobbyMapOption
{
	GENERATED_BODY()

	/** Name shown on the lobby info screen and vote spots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby") FText DisplayName;

	/** Map to travel to. Full path is safest, e.g. /Game/Maps/L_Slice02/L_Slice02_Blockout */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby") FString MapName;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBSOnLobbyStateChanged);

/** Replicated lobby state every client needs for the info screen, ready pads and map vote. Written only by ABSLobbyGameMode. */
UCLASS()
class BLINDSIGHT_API ABSLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Blind Sight|Lobby") const TArray<FBSLobbyMapOption>& GetMapOptions() const { return MapOptions; }
	/** Votes per map, same order as GetMapOptions. */
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Lobby") const TArray<int32>& GetMapVotes() const { return MapVotes; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Lobby") int32 GetSelectedMapIndex() const { return SelectedMapIndex; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Lobby") FText GetSelectedMapDisplayName() const;
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Lobby") int32 GetLobbyPlayerCount() const { return LobbyPlayerCount; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Lobby") int32 GetReadyCount() const { return ReadyCount; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Lobby") int32 GetMinPlayersToStart() const { return MinPlayersToStart; }
	/** True when the host's Start Match will be accepted. */
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Lobby") bool CanStartMatch() const { return bCanStartMatch; }

	/** Fires on every machine whenever any value above changes. Bind UI to this instead of polling. */
	UPROPERTY(BlueprintAssignable, Category = "Blind Sight|Lobby") FBSOnLobbyStateChanged OnLobbyStateChanged;

	// Server only (called by ABSLobbyGameMode)
	void SetMapOptions(const TArray<FBSLobbyMapOption>& InOptions);
	void SetLobbySnapshot(int32 InPlayerCount, int32 InReadyCount, const TArray<int32>& InVotes, int32 InSelectedMapIndex, bool bInCanStart, int32 InMinPlayers);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_LobbyState) TArray<FBSLobbyMapOption> MapOptions;
	UPROPERTY(ReplicatedUsing = OnRep_LobbyState) TArray<int32> MapVotes;
	UPROPERTY(ReplicatedUsing = OnRep_LobbyState) int32 SelectedMapIndex = 0;
	UPROPERTY(ReplicatedUsing = OnRep_LobbyState) int32 LobbyPlayerCount = 0;
	UPROPERTY(ReplicatedUsing = OnRep_LobbyState) int32 ReadyCount = 0;
	UPROPERTY(ReplicatedUsing = OnRep_LobbyState) int32 MinPlayersToStart = 2;
	UPROPERTY(ReplicatedUsing = OnRep_LobbyState) bool bCanStartMatch = false;

	UFUNCTION() void OnRep_LobbyState();
};
