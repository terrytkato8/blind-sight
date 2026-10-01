#include "Abilities/BSGA_Throw.h"
#include "Abilities/BSAttributeSet.h"
#include "Characters/BSHiderCharacter.h"
#include "Gameplay/BSThrowable.h"
#include "Core/BSGameplayTags.h"
#include "Sound/BSSoundEventSubsystem.h"

UBSGA_Throw::UBSGA_Throw()
{
	// UE 5.5+ : AbilityTags is deprecated; asset tags are set once, here in the constructor.
	SetAssetTags(FGameplayTagContainer(BSTags::Ability_Throw));
	ActivationBlockedTags.AddTag(BSTags::State_Eliminated);
	ThrowableClass = ABSThrowable::StaticClass();
}

bool UBSGA_Throw::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
	const UBSAttributeSet* Attr = GetBSAttributes();
	return Attr && Attr->GetThrowables() >= 1.f && ThrowableClass != nullptr;
}

void UBSGA_Throw::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	ABSHiderCharacter* Hider = Cast<ABSHiderCharacter>(GetAvatarActorFromActorInfo());
	UWorld* World = GetWorld();
	if (!Hider || !World) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	ModifyAttribute(UBSAttributeSet::GetThrowablesAttribute(), -1.f);

	FVector Eye; FRotator Rot;
	Hider->GetActorEyesViewPoint(Eye, Rot);
	const FVector Dir = (Rot.Vector() + FVector::UpVector * UpwardBias).GetSafeNormal();
	const FVector SpawnLoc = Eye + Dir * 60.f;

	FActorSpawnParameters SP;
	SP.Owner = Hider; SP.Instigator = Hider;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	if (ABSThrowable* Obj = World->SpawnActor<ABSThrowable>(ThrowableClass, SpawnLoc, Dir.Rotation(), SP))
	{
		Obj->Launch(Dir * ThrowSpeed);
	}

	// Quieter ping at the throw point (GDD §6.3) — the impact ping comes from the projectile.
	UBSSoundEventSubsystem::EmitSound(Hider, EBSSoundEventType::ThrowRelease, Hider->GetActorLocation(), Hider);
	Hider->OnThrowablesChanged();

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
