#include "Abilities/BSAbilitySystemComponent.h"

bool UBSAbilitySystemComponent::TryActivateByTag(FGameplayTag Tag)
{
	return TryActivateAbilitiesByTag(FGameplayTagContainer(Tag));
}
