#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/BSLobbyGameState.h"
#include "BSLobbyGameMode.generated.h"

class ABSGameMode;
class ABSPlayerState;

/**
 * Pre-match lobby (GDD §5 step 1). Players ready up and vote on a map; the host starts the match and everyone travels.
 * All lobby rules live here on the server; clients only see the result through ABSLobbyGameState.
 */
UCLASS()
class BLINDSIGHT_API ABSLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABSLobbyGameMode();

	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** Maps players can vote for. The first entry is used until someone votes. Fill in on BP_GM_Lobby. */
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Lobby") TArray<FBSLobbyMapOption> MapOptions;

	/** Game mode the match map runs (e.g. BP_GM_Blackout). */
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Lobby") TSubclassOf<ABSGameMode> MatchModeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Lobby", meta = (ClampMin = "1")) int32 MinPlayersToStart = 2;

	/** When true, Start Match is refused until every player has readied up. Turn off for quick solo testing. */
	UPROPERTY(EditDefaultsOnly, Category = "Blind Sight|Lobby") bool bRequireAllReady = true;

	// Called from ABSPlayerController server RPCs.
	void SetPlayerReady(ABSPlayerState* PS, bool bReady);
	void SetPlayerMapVote(ABSPlayerState* PS, int32 MapIndex);
	/** Host only. Travels to the winning map if the lobby is ready. Returns false if refused. */
	bool RequestStartMatch(APlayerController* Requester);

	/** Map name (e.g. "L_Slice01") + mode class -> seamless ServerTravel with ?game=... */
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Lobby")
	void StartMatch(const FString& MapName, TSubclassOf<ABSGameMode> ModeClass);

protected:
	/** Recounts players, ready flags and votes, then publishes the result to the game state. */
	void RefreshLobbyState(const APlayerState* Ignore = nullptr);

	int32 SelectedMapIndex = 0;
	bool bCanStartMatch = false;
};
