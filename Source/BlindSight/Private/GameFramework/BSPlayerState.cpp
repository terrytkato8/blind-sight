#include "GameFramework/BSPlayerState.h"
#include "Abilities/BSAbilitySystemComponent.h"
#include "Abilities/BSAttributeSet.h"
#include "Core/BSGameplayTags.h"
#include "Net/UnrealNetwork.h"

ABSPlayerState::ABSPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UBSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AttributeSet = CreateDefaultSubobject<UBSAttributeSet>(TEXT("AttributeSet"));

	SetNetUpdateFrequency(60.f);
}

UAbilitySystemComponent* ABSPlayerState::GetAbilitySystemComponent() const { return AbilitySystemComponent; }

void ABSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABSPlayerState, BSRole);
	DOREPLIFETIME(ABSPlayerState, bEliminated);
	DOREPLIFETIME(ABSPlayerState, TotalPoints);
	DOREPLIFETIME(ABSPlayerState, Placement);
	DOREPLIFETIME(ABSPlayerState, CatchesThisRound);
	DOREPLIFETIME(ABSPlayerState, RotationIndex);
	DOREPLIFETIME(ABSPlayerState, HunterTurnsTaken);
}

void ABSPlayerState::SetBSRole(EBSRole NewRole)
{
	if (!HasAuthority()) return;
	BSRole = NewRole;
	AbilitySystemComponent->RemoveLooseGameplayTag(BSTags::Role_Hunter);
	AbilitySystemComponent->RemoveLooseGameplayTag(BSTags::Role_Hider);
	if (BSRole == EBSRole::Hunter) AbilitySystemComponent->AddLooseGameplayTag(BSTags::Role_Hunter);
	if (BSRole == EBSRole::Hider)  AbilitySystemComponent->AddLooseGameplayTag(BSTags::Role_Hider);
	OnRep_Role();
}

void ABSPlayerState::ResetForRound()
{
	bEliminated = false;
	Placement = 0;
	CatchesThisRound = 0;
	ShotsFiredThisRound = 0;
	ThrowablesUsedThisRound = 0;
	SurvivalSeconds = 0.f;
	AbilitySystemComponent->RemoveLooseGameplayTag(BSTags::State_Eliminated);
	AbilitySystemComponent->RemoveLooseGameplayTag(BSTags::State_Sprinting);
	AbilitySystemComponent->RemoveLooseGameplayTag(BSTags::State_Crouching);
	AbilitySystemComponent->RemoveLooseGameplayTag(BSTags::State_HeadStartFrozen);
}

void ABSPlayerState::OnRep_Role() { OnRoleChanged.Broadcast(BSRole); }
