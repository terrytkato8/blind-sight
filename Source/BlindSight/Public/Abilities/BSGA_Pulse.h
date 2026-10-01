#pragma once

#include "CoreMinimal.h"
#include "Abilities/BSGameplayAbility.h"
#include "BSGA_Pulse.generated.h"

/**
 * Hunter echolocation burst (GDD §4.1, stretch). Reveals nearby silhouettes to the Hunter client
 * for a moment, but emits an audible Pulse that Hiders can hear — information traded for exposure.
 * Cooldown is tracked on the Hunter character (version-robust, no cooldown GE asset needed).
 */
UCLASS()
class BLINDSIGHT_API UBSGA_Pulse : public UBSGameplayAbility
{
	GENERATED_BODY()

public:
	UBSGA_Pulse();

	UPROPERTY(EditDefaultsOnly, Category = "Pulse") float CooldownSeconds = 18.f;
	UPROPERTY(EditDefaultsOnly, Category = "Pulse") float RevealRadius = 1800.f;
	UPROPERTY(EditDefaultsOnly, Category = "Pulse") float RevealDuration = 1.2f;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
