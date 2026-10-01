#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BSGameSettings.generated.h"

class UBSSoundProfile;

/** Project Settings > Game > Blind Sight. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Blind Sight"))
class BLINDSIGHT_API UBSGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Sound event catalogue used by the sound subsystem unless a GameMode overrides it. */
	UPROPERTY(Config, EditAnywhere, Category = "Sound")
	TSoftObjectPtr<UBSSoundProfile> DefaultSoundProfile;

	/** Lobby sweet spot (GDD §3). */
	UPROPERTY(Config, EditAnywhere, Category = "Lobby", meta = (ClampMin = "2", ClampMax = "8"))
	int32 MinPlayersToStart = 4;

	UPROPERTY(Config, EditAnywhere, Category = "Lobby", meta = (ClampMin = "2", ClampMax = "8"))
	int32 MaxPlayers = 8;

	/** Base URL of the Blind Sight backend service, e.g. https://api.blindsight.game */
	UPROPERTY(Config, EditAnywhere, Category = "Backend")
	FString BackendBaseUrl = TEXT("http://127.0.0.1:8080");

	/** Rounds played per match before the dedicated server tears down and returns to the pool. */
	UPROPERTY(Config, EditAnywhere, Category = "Backend", meta = (ClampMin = "1", ClampMax = "20"))
	int32 RoundsPerMatch = 6;

	/** Dedicated server: disconnect players the backend does not recognise as assigned to this match. */
	UPROPERTY(Config, EditAnywhere, Category = "Backend")
	bool bEnforcePlayerValidation = true;
};
