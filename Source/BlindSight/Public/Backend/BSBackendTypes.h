#pragma once

#include "CoreMinimal.h"
#include "Core/BSTypes.h"
#include "BSBackendTypes.generated.h"

/** A player's persistent profile, owned by the backend. The game never writes these locally. */
USTRUCT(BlueprintType)
struct FBSPlayerProfile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString PlayerId;
	UPROPERTY(BlueprintReadOnly) FString DisplayName;
	UPROPERTY(BlueprintReadOnly) int32 Level = 1;
	UPROPERTY(BlueprintReadOnly) int32 Experience = 0;
	/** Soft currency earned from round payouts (GDD 7.1 / 7.2). */
	UPROPERTY(BlueprintReadOnly) int32 Currency = 0;
	UPROPERTY(BlueprintReadOnly) int32 MatchesPlayed = 0;
	UPROPERTY(BlueprintReadOnly) int32 RoundsAsHunter = 0;
	UPROPERTY(BlueprintReadOnly) int32 TotalCatches = 0;
	UPROPERTY(BlueprintReadOnly) int32 TotalSurvivals = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FString> UnlockedCosmetics;
	UPROPERTY(BlueprintReadOnly) FString EquippedHunterSkin;
	UPROPERTY(BlueprintReadOnly) FString EquippedHiderSkin;
	UPROPERTY(BlueprintReadOnly) bool bValid = false;
};

/** One player's contribution to a finished round, posted by the dedicated server. */
USTRUCT()
struct FBSMatchResultEntry
{
	GENERATED_BODY()

	UPROPERTY() FString PlayerId;
	UPROPERTY() FString Role;
	UPROPERTY() int32 Points = 0;
	UPROPERTY() int32 Placement = 0;
	UPROPERTY() int32 Catches = 0;
	UPROPERTY() bool bSurvived = false;
	UPROPERTY() float SurvivalSeconds = 0.f;
	UPROPERTY() int32 ShotsFired = 0;
	UPROPERTY() int32 ThrowablesUsed = 0;
	UPROPERTY() int32 DistanceTravelledMeters = 0;
};

/** A single telemetry event. Flushed in batches so a busy round does not spam HTTP. */
USTRUCT()
struct FBSTelemetryEvent
{
	GENERATED_BODY()

	UPROPERTY() FString EventName;
	UPROPERTY() FString MatchId;
	UPROPERTY() FString PlayerId;
	UPROPERTY() double RoundTime = 0.0;
	/** Free-form numeric payload — distances, intensities, counts. */
	UPROPERTY() TMap<FString, float> Numbers;
	UPROPERTY() TMap<FString, FString> Strings;
};

/** What matchmaking hands back once a server has been allocated. */
USTRUCT(BlueprintType)
struct FBSMatchAssignment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString MatchId;
	UPROPERTY(BlueprintReadOnly) FString ServerAddress;	// "ip:port" — passed to ClientTravel
	UPROPERTY(BlueprintReadOnly) FString Region;
	UPROPERTY(BlueprintReadOnly) FString ModeName;
	UPROPERTY(BlueprintReadOnly) FString MapName;
	UPROPERTY(BlueprintReadOnly) bool bReady = false;
};

UENUM(BlueprintType)
enum class EBSTicketState : uint8
{
	None,
	Queued,
	Matched,
	Failed,
	Cancelled,
};
