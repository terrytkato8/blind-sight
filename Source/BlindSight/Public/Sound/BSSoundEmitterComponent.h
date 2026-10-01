#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/BSTypes.h"
#include "BSSoundEmitterComponent.generated.h"

/**
 * Attach to any pawn. On the server, converts movement into footstep sound events based on
 * the owner's current noise tier (GDD §4.2). Also exposes a one-shot Emit() helper.
 */
UCLASS(ClassGroup = (BlindSight), meta = (BlueprintSpawnableComponent))
class BLINDSIGHT_API UBSSoundEmitterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBSSoundEmitterComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Distance (cm) travelled between footstep events, per tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	float StrideCrouch = 70.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	float StrideWalk = 110.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	float StrideSprint = 150.f;

	/** Called by the owning character to report the current noise tier (replicated upstream by the character). */
	void SetNoiseTier(EBSNoiseTier Tier) { CurrentTier = Tier; }
	EBSNoiseTier GetNoiseTier() const { return CurrentTier; }

	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Sound")
	void Emit(EBSSoundEventType Type, float IntensityScale = 1.f);

private:
	EBSNoiseTier CurrentTier = EBSNoiseTier::Still;
	float DistanceAccum = 0.f;
	FVector LastLocation = FVector::ZeroVector;
	bool bHasLast = false;
};
