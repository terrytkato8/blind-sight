#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/BSTypes.h"
#include "BSSoundProfile.generated.h"

/**
 * Tunable catalogue of every sound event (GDD §6.1 table, §10).
 * Create one Data Asset from this (DA_SoundProfile) and assign it in Project Settings > Blind Sight,
 * or on the World Subsystem via the GameMode. Values here are the GDD's relative-loudness baseline.
 */
UCLASS(BlueprintType)
class BLINDSIGHT_API UBSSoundProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UBSSoundProfile();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events")
	TMap<EBSSoundEventType, FBSSoundEventDefinition> Events;

	/** Multiplier applied to perceived intensity when the origin->listener line is blocked by geometry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Propagation", meta = (ClampMin = "0", ClampMax = "1"))
	float OcclusionFactor = 0.35f;

	/** Extra multiplier per additional blocking surface beyond the first (thick walls / floors). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Propagation", meta = (ClampMin = "0", ClampMax = "1"))
	float PerExtraSurfaceFactor = 0.6f;

	/** Pings weaker than this are never sent to the Hunter (bandwidth + noise floor). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Propagation", meta = (ClampMin = "0", ClampMax = "1"))
	float MinPerceivedIntensity = 0.04f;

	/** Falloff curve exponent. 1 = linear, 2 = quadratic-ish (quieter at range). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Propagation", meta = (ClampMin = "0.1", ClampMax = "4"))
	float FalloffExponent = 1.25f;

	const FBSSoundEventDefinition& GetDefinition(EBSSoundEventType Type) const;

	UFUNCTION(BlueprintPure, Category = "Sound", DisplayName = "Get Definition")
	FBSSoundEventDefinition GetDefinitionCopy(EBSSoundEventType Type) const { return GetDefinition(Type); }
};
