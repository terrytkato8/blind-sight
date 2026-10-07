#include "Abilities/BSGameplayAbility.h"
#include "Abilities/BSAttributeSet.h"
#include "Characters/BSCharacterBase.h"
#include "AbilitySystemComponent.h"

UBSGameplayAbility::UBSGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;
	
}

ABSCharacterBase* UBSGameplayAbility::GetBSCharacter() const
{
	return Cast<ABSCharacterBase>(GetAvatarActorFromActorInfo());
}

const UBSAttributeSet* UBSGameplayAbility::GetBSAttributes() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	return ASC ? ASC->GetSet<UBSAttributeSet>() : nullptr;
}

void UBSGameplayAbility::ModifyAttribute(const FGameplayAttribute& Attribute, float Delta) const
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ApplyModToAttribute(Attribute, EGameplayModOp::Additive, Delta);
	}
}
