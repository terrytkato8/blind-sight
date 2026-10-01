#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/BSTypes.h"
#include "BSThrowable.generated.h"

class UStaticMeshComponent;

/**
 * Thrown prop (bottle, can, rock, noisemaker). Chaos physics for arcs (GDD §12);
 * emits one loud ThrowImpact sound event on first landing, then optional smaller re-bounces.
 * Subclass in BP per prop type with its own mesh and ImpactEventType/Intensity for distinct signatures.
 */
UCLASS()
class BLINDSIGHT_API ABSThrowable : public AActor
{
	GENERATED_BODY()

public:
	ABSThrowable();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "Sound") EBSSoundEventType ImpactEventType = EBSSoundEventType::ThrowImpact;
	UPROPERTY(EditDefaultsOnly, Category = "Sound") float ImpactIntensityScale = 1.f;
	/** Secondary bounces emit at this fraction; 0 disables. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound") float BounceIntensityScale = 0.35f;
	UPROPERTY(EditDefaultsOnly, Category = "Sound") int32 MaxBounceEvents = 2;
	UPROPERTY(EditDefaultsOnly, Category = "Lifetime") float LifeSeconds = 12.f;
	/** Impacts slower than this (cm/s) are ignored (rolling to a stop). */
	UPROPERTY(EditDefaultsOnly, Category = "Sound") float MinImpactSpeed = 120.f;

	/** Server: give it its initial velocity. */
	void Launch(const FVector& Velocity);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	int32 ImpactCount = 0;
};
