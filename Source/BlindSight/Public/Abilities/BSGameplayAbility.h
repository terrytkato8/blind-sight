#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "BSGameplayAbility.generated.h"

class ABSCharacterBase;
class UBSAttributeSet;

/** Base for all Blind Sight abilities. Server-only execution keeps ammo/throwables cheat-proof (GDD §12). */
UCLASS(Abstract)
class BLINDSIGHT_API UBSGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBSGameplayAbility();

protected:
	ABSCharacterBase* GetBSCharacter() const;
	const UBSAttributeSet* GetBSAttributes() const;

	/** Convenience: additive modify an attribute on the server. */
	void ModifyAttribute(const FGameplayAttribute& Attribute, float Delta) const;
};
