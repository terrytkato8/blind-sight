#include "GameFramework/BSPlayerState.h"
#include "Abilities/BSAbilitySystemComponent.h"
#include "Abilities/BSAttributeSet.h"
#include "Core/BSGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Abilities/Skills/BSSkillDefinition.h"
#include "Abilities/BSGameplayAbility.h"

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
	DOREPLIFETIME(ABSPlayerState, bLobbyReady);
	DOREPLIFETIME(ABSPlayerState, LobbyMapVote);
	DOREPLIFETIME(ABSPlayerState, EquippedSkills);
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
	RemoveSkillsForRound();
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

void ABSPlayerState::SetLobbyReady(bool bNew)
{
	if (!HasAuthority() || bLobbyReady == bNew) return;
	bLobbyReady = bNew;
	OnRep_Lobby();	// listen server / local
}

void ABSPlayerState::SetLobbyMapVote(int32 MapIndex)
{
	if (!HasAuthority() || LobbyMapVote == MapIndex) return;
	LobbyMapVote = MapIndex;
	OnRep_Lobby();	// listen server / local
}

TArray<UBSSkillDefinition*> ABSPlayerState::GetEquippedSkills() const
{
	TArray<UBSSkillDefinition*> Out;
	for (UBSSkillDefinition* Skill : EquippedSkills)
	{
		if (Skill) Out.Add(Skill);
	}
	return Out;
}

bool ABSPlayerState::HasSkillEquipped(UBSSkillDefinition* Skill) const
{
	return Skill && EquippedSkills.Contains(Skill);
}

bool ABSPlayerState::EquipSkill(UBSSkillDefinition* Skill)
{
	if (!HasAuthority() || !Skill || EquippedSkills.Contains(Skill)) return false;
	
	int32 SameRoleCount = 0;
	for (const UBSSkillDefinition* Equipped : EquippedSkills)
	{
		if (Equipped && Equipped->AllowedRole == Skill->AllowedRole) ++SameRoleCount;
	}
	if (SameRoleCount >= MaxSkillsPerRole) return false;
	
	EquippedSkills.Add(Skill);
	OnRep_Loadout(); // listen server / local
	return true;
}

void ABSPlayerState::UnEquipSkill(UBSSkillDefinition* Skill)
{
	if (!HasAuthority()) return;
	if (EquippedSkills.Remove(Skill) > 0)
	{
		OnRep_Loadout(); //Listen server/local
	}
}

void ABSPlayerState::ApplySkillsForRound()
{
	if (!HasAuthority()) return;
	RemoveSkillsForRound(); //never stacks from previous round
	
	for (const UBSSkillDefinition* Skill : EquippedSkills)
	{
		if (!Skill || !Skill->IsUsableBy(BSRole)) continue;
		
		if (Skill->GrantedAbility)
		{
			GrantedSkillAbilities.Add(AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(Skill->GrantedAbility, 1, INDEX_NONE, this)));
		}
		
		for (const TSubclassOf<UGameplayEffect>& EffectClass : Skill->PassiveEffects)
		{
			if (!EffectClass) continue;
			FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
			Context.AddSourceObject(this);
			const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, Context);
			if (Spec.IsValid())
			{
				ActiveSkillEffects.Add(AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()));
			}
		}
	}
}

void ABSPlayerState::RemoveSkillsForRound()
{
	if (!HasAuthority()) return;
	
	for (const FGameplayAbilitySpecHandle& Handle : GrantedSkillAbilities)
	{
		AbilitySystemComponent->ClearAbility(Handle);
	}
	GrantedSkillAbilities.Reset();
	
	for (const FActiveGameplayEffectHandle& Handle : ActiveSkillEffects)
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
	}
	ActiveSkillEffects.Reset();
}

void ABSPlayerState::OnRep_Lobby() { OnLobbyPlayerChanged.Broadcast(); }

void ABSPlayerState::OnRep_Loadout() { OnLoadoutChanged.Broadcast(); }

void ABSPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	if (ABSPlayerState* Other = Cast<ABSPlayerState>(PlayerState))
	{
		Other->EquippedSkills = EquippedSkills;
	}
}
