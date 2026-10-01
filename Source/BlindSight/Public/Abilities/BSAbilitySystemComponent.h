#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "BSAbilitySystemComponent.generated.h"

UCLASS()
class BLINDSIGHT_API UBSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** Helper: try to activate every ability whose AbilityTags contain Tag. */
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Abilities")
	bool TryActivateByTag(FGameplayTag Tag);
};
