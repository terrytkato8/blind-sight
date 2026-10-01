#pragma once

#include "CoreMinimal.h"
#include "Abilities/BSGameplayAbility.h"
#include "BSGA_Fire.generated.h"

/**
 * Hunter's single ranged weapon (GDD §4.1). Hitscan, one ammo per shot, no reload.
 * Emits a map-wide Gunshot sound event — every Hider learns roughly where the Hunter is.
 */
UCLASS()
class BLINDSIGHT_API UBSGA_Fire : public UBSGameplayAbility
{
	GENERATED_BODY()

public:
	UBSGA_Fire();

	UPROPERTY(EditDefaultsOnly, Category = "Weapon") float Range = 6000.f;
	/** Cosmetic spread; the Hunter's real limiter is ammo and vision. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon") float SpreadDegrees = 0.75f;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
