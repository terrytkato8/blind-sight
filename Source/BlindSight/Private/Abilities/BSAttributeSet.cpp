#include "Abilities/BSAttributeSet.h"
#include "Net/UnrealNetwork.h"

UBSAttributeSet::UBSAttributeSet()
{
	InitAmmo(0.f); InitMaxAmmo(0.f);
	InitThrowables(0.f); InitMaxThrowables(2.f);	// GDD §6.3: 1–2 at a time
}

void UBSAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UBSAttributeSet, Ammo,          COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBSAttributeSet, MaxAmmo,       COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBSAttributeSet, Throwables,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBSAttributeSet, MaxThrowables, COND_None, REPNOTIFY_Always);
}

void UBSAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetAmmoAttribute())       NewValue = FMath::Clamp(NewValue, 0.f, GetMaxAmmo());
	if (Attribute == GetThrowablesAttribute()) NewValue = FMath::Clamp(NewValue, 0.f, GetMaxThrowables());
}

void UBSAttributeSet::OnRep_Ammo(const FGameplayAttributeData& Old)          { GAMEPLAYATTRIBUTE_REPNOTIFY(UBSAttributeSet, Ammo, Old); }
void UBSAttributeSet::OnRep_MaxAmmo(const FGameplayAttributeData& Old)       { GAMEPLAYATTRIBUTE_REPNOTIFY(UBSAttributeSet, MaxAmmo, Old); }
void UBSAttributeSet::OnRep_Throwables(const FGameplayAttributeData& Old)    { GAMEPLAYATTRIBUTE_REPNOTIFY(UBSAttributeSet, Throwables, Old); }
void UBSAttributeSet::OnRep_MaxThrowables(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBSAttributeSet, MaxThrowables, Old); }
