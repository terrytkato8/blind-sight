#pragma once

#include "CoreMinimal.h"
#include "Abilities/BSGameplayAbility.h"
#include "BSGA_Throw.generated.h"

class ABSThrowable;

/** Hider throw (GDD §6.3): quiet ping at release, loud ping at impact (handled by ABSThrowable). */
UCLASS()
class BLINDSIGHT_API UBSGA_Throw : public UBSGameplayAbility
{
	GENERATED_BODY()

public:
	UBSGA_Throw();

	UPROPERTY(EditDefaultsOnly, Category = "Throw") TSubclassOf<ABSThrowable> ThrowableClass;
	UPROPERTY(EditDefaultsOnly, Category = "Throw") float ThrowSpeed = 1400.f;
	UPROPERTY(EditDefaultsOnly, Category = "Throw") float UpwardBias = 0.15f;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
