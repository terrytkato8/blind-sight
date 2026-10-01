#include "Abilities/BSGA_Fire.h"
#include "Abilities/BSAttributeSet.h"
#include "Characters/BSHunterCharacter.h"
#include "Characters/BSHiderCharacter.h"
#include "Core/BSGameplayTags.h"
#include "Sound/BSSoundEventSubsystem.h"
#include "GameFramework/BSGameMode.h"
#include "Camera/CameraComponent.h"

UBSGA_Fire::UBSGA_Fire()
{
	// UE 5.5+ : AbilityTags is deprecated; asset tags are set once, here in the constructor.
	SetAssetTags(FGameplayTagContainer(BSTags::Ability_Fire));
	ActivationBlockedTags.AddTag(BSTags::State_HeadStartFrozen);
	ActivationBlockedTags.AddTag(BSTags::State_Eliminated);
}

bool UBSGA_Fire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
	const UBSAttributeSet* Attr = GetBSAttributes();
	return Attr && Attr->GetAmmo() >= 1.f;
}

void UBSGA_Fire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	ABSHunterCharacter* Hunter = Cast<ABSHunterCharacter>(GetAvatarActorFromActorInfo());
	UWorld* World = GetWorld();
	if (!Hunter || !World) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	// Spend the shot first: ammo is a hard budget (GDD §4.1).
	ModifyAttribute(UBSAttributeSet::GetAmmoAttribute(), -1.f);

	FVector Start; FRotator Rot;
	Hunter->GetActorEyesViewPoint(Start, Rot);
	FVector Dir = Rot.Vector();
	if (SpreadDegrees > 0.f)
	{
		Dir = FMath::VRandCone(Dir, FMath::DegreesToRadians(SpreadDegrees));
	}
	const FVector End = Start + Dir * Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BSFire), true, Hunter);
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

	// Gunshot is very loud and map-wide-ish — the double-edged sword (GDD §6.1).
	UBSSoundEventSubsystem::EmitSound(Hunter, EBSSoundEventType::Gunshot, Hunter->GetActorLocation(), Hunter);
	Hunter->MulticastOnFired(bHit ? Hit.ImpactPoint : End);

	if (bHit)
	{
		if (ABSHiderCharacter* Hider = Cast<ABSHiderCharacter>(Hit.GetActor()))
		{
			if (!Hider->IsEliminated())
			{
				Hider->ServerEliminate(Hunter);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
