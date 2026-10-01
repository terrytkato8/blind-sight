#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BSAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Scarcity lives here (GDD Pillar 2): Hunter ammo and Hider throwable stock are hard budgets,
 * replicated through GAS so HUDs and abilities share one source of truth.
 */
UCLASS()
class BLINDSIGHT_API UBSAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UBSAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// --- Hunter ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Ammo, Category = "Hunter")
	FGameplayAttributeData Ammo;
	ATTRIBUTE_ACCESSORS(UBSAttributeSet, Ammo)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxAmmo, Category = "Hunter")
	FGameplayAttributeData MaxAmmo;
	ATTRIBUTE_ACCESSORS(UBSAttributeSet, MaxAmmo)

	// --- Hider ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Throwables, Category = "Hider")
	FGameplayAttributeData Throwables;
	ATTRIBUTE_ACCESSORS(UBSAttributeSet, Throwables)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxThrowables, Category = "Hider")
	FGameplayAttributeData MaxThrowables;
	ATTRIBUTE_ACCESSORS(UBSAttributeSet, MaxThrowables)

protected:
	UFUNCTION() void OnRep_Ammo(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_MaxAmmo(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_Throwables(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_MaxThrowables(const FGameplayAttributeData& Old);
};
