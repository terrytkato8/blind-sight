#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BSTypes.generated.h"

/** Which side of the hunt a player is on this round. */
UENUM(BlueprintType)
enum class EBSRole : uint8
{
	None		UMETA(DisplayName = "None / Lobby"),
	Hunter		UMETA(DisplayName = "Hunter"),
	Hider		UMETA(DisplayName = "Hider"),
};

/** Round state machine (GDD §5). */
UENUM(BlueprintType)
enum class EBSRoundPhase : uint8
{
	Lobby,			// waiting for players / mode select
	HeadStart,		// hiders scatter, hunter frozen
	InProgress,		// the hunt
	RoundOver,		// results screen, then rotation
};

/** Why a round ended. Drives scoring (GDD §7). */
UENUM(BlueprintType)
enum class EBSRoundEndReason : uint8
{
	TimerExpired,		// Blackout only
	AllHidersCaught,	// Blackout: hunter takes the pot
	LastHiderStanding,	// Manhunt
	Aborted,
};

/** Hider movement / noise tiers (GDD §4.2, §6.1). */
UENUM(BlueprintType)
enum class EBSNoiseTier : uint8
{
	Still,
	Crouch,
	Walk,
	Sprint,
};

/** Every audible event type the Hunter can perceive (GDD §6.1 table). */
UENUM(BlueprintType)
enum class EBSSoundEventType : uint8
{
	FootstepCrouch,
	FootstepWalk,
	FootstepSprint,
	ItemPickup,
	ThrowRelease,
	ThrowImpact,
	Door,
	Gunshot,
	HunterPulse,
	Custom,
};

/** Tunable definition of one sound event type (lives in UBSSoundProfile). */
USTRUCT(BlueprintType)
struct FBSSoundEventDefinition
{
	GENERATED_BODY()

	/** How far the sound travels (cm) before falling to zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float Radius = 1000.f;

	/** How strongly it registers to the Hunter at the origin, 0..1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1"))
	float Intensity = 0.5f;

	/** How long the Hunter's visual ping lingers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float PingLifetime = 1.5f;

	/** Audio asset played at the location on every client (MetaSound recommended; receives "Intensity" float param). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<USoundBase> Sound;

	/** Optional gameplay tag for Blueprint/analytics filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag;
};

/** Server-side authoritative sound event. */
USTRUCT(BlueprintType)
struct FBSSoundEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) EBSSoundEventType Type = EBSSoundEventType::Custom;
	UPROPERTY(BlueprintReadWrite) FVector Origin = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite) float Radius = 0.f;
	UPROPERTY(BlueprintReadWrite) float Intensity = 0.f;
	UPROPERTY(BlueprintReadWrite) float PingLifetime = 1.5f;
	UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AActor> Instigator;
};

/** What a Hunter client actually receives — already attenuated/occluded by the server. */
USTRUCT(BlueprintType)
struct FBSSoundPing
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) EBSSoundEventType Type = EBSSoundEventType::Custom;
	UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
	/** Perceived strength 0..1 after distance falloff and occlusion. */
	UPROPERTY(BlueprintReadOnly) float PerceivedIntensity = 0.f;
	UPROPERTY(BlueprintReadOnly) float Lifetime = 1.5f;
	UPROPERTY(BlueprintReadOnly) bool bOccluded = false;
	UPROPERTY(BlueprintReadOnly) float ServerTime = 0.f;
};

/** Per-round scoring output for one player. */
USTRUCT(BlueprintType)
struct FBSRoundScoreEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString PlayerName;
	UPROPERTY(BlueprintReadOnly) EBSRole Role = EBSRole::None;
	UPROPERTY(BlueprintReadOnly) int32 PointsAwarded = 0;
	UPROPERTY(BlueprintReadOnly) bool bSurvived = false;
	UPROPERTY(BlueprintReadOnly) int32 Placement = 0;	// Manhunt: 1 = Survivor
	UPROPERTY(BlueprintReadOnly) bool bIsSurvivorCallout = false;
};
