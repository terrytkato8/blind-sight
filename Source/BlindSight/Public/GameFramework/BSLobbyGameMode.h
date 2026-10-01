#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BSLobbyGameMode.generated.h"

class ABSGameMode;

/** Pre-match lobby (GDD §5 step 1). Host picks map + mode, then everyone travels. */
UCLASS()
class BLINDSIGHT_API ABSLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABSLobbyGameMode();

	/** Map name (e.g. "L_Slice01") + mode class -> seamless ServerTravel with ?game=... */
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Lobby")
	void StartMatch(const FString& MapName, TSubclassOf<ABSGameMode> ModeClass);
};
