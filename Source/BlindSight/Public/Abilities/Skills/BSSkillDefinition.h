// Copyright 2026 Kato.8 Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Core/BSTypes.h"
#include "BSSkillDefinition.generated.h"


class UGameplayEffect;


/** One pickable skill. Create a Data Asset per skill (DA_Skill_Name).*/

UCLASS(BlueprintType)
class BLINDSIGHT_API UBSSkillDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	/** Unique id for this skill, e.g. Skill.Hider.QuietStep. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
	FGameplayTag SkillTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
	FText DisplayName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
	FText Description;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
	TSoftObjectPtr<class UTexture2D> Icon;
	
	/** Which role may equip this. None = either role. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
	EBSRole AllowedRole = EBSRole::None;
	
	/** Active skill: an ability the player triggers. Leave empty for a passive. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
	TSubclassOf<class UBSGameplayAbility> GrantedAbility;
	
	/** Passive skill: effects applied for the whole round. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
	TArray<TSubclassOf<class UGameplayEffect>> PassiveEffects;
	
	UFUNCTION(BlueprintPure, Category = "Skill")
	bool IsUsableBy(EBSRole Role) const
	{
		return AllowedRole == EBSRole::None || AllowedRole == Role;
	}
};
