#include "Abilities/BSGA_Pulse.h"
#include "Characters/BSHunterCharacter.h"
#include "Core/BSGameplayTags.h"
#include "Sound/BSSoundEventSubsystem.h"

UBSGA_Pulse::UBSGA_Pulse()
{
	// UE 5.5+ : AbilityTags is deprecated; asset tags are set once, here in the constructor.
	SetAssetTags(FGameplayTagContainer(BSTags::Ability_Pulse));
	ActivationBlockedTags.AddTag(BSTags::State_HeadStartFrozen);
	ActivationBlockedTags.AddTag(BSTags::State_Eliminated);
}

bool UBSGA_Pulse::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
	const ABSHunterCharacter* Hunter = Cast<ABSHunterCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	return Hunter && Hunter->bPulseEnabled && Hunter->GetPulseCooldownRemaining() <= 0.f;
}

void UBSGA_Pulse::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	ABSHunterCharacter* Hunter = Cast<ABSHunterCharacter>(GetAvatarActorFromActorInfo());
	if (!Hunter) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	Hunter->StartPulseCooldown(CooldownSeconds);
	UBSSoundEventSubsystem::EmitSound(Hunter, EBSSoundEventType::HunterPulse, Hunter->GetActorLocation(), Hunter);
	Hunter->ClientPulseReveal(RevealRadius, RevealDuration);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
