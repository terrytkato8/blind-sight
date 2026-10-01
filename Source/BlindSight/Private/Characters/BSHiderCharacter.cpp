#include "Characters/BSHiderCharacter.h"
#include "Abilities/BSAbilitySystemComponent.h"
#include "Abilities/BSAttributeSet.h"
#include "Sound/BSSoundEmitterComponent.h"
#include "Sound/BSSoundEventSubsystem.h"
#include "Gameplay/BSThrowableSpawnPoint.h"
#include "Gameplay/BSInteractable.h"
#include "GameFramework/BSGameMode.h"
#include "GameFramework/BSPlayerState.h"
#include "Core/BSGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"

ABSHiderCharacter::ABSHiderCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABSHiderCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABSHiderCharacter, NoiseTier);
	DOREPLIFETIME(ABSHiderCharacter, bSprinting);
}

void ABSHiderCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority()) UpdateNoiseTier();
}

void ABSHiderCharacter::UpdateNoiseTier()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const float Speed2D = GetVelocity().Size2D();
	EBSNoiseTier NewTier = EBSNoiseTier::Still;
	if (Speed2D > 10.f)
	{
		if (bIsCrouched)         NewTier = EBSNoiseTier::Crouch;
		else if (bSprinting)     NewTier = EBSNoiseTier::Sprint;
		else                     NewTier = EBSNoiseTier::Walk;
	}
	Move->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;
	Move->MaxWalkSpeedCrouched = CrouchSpeed;

	if (NewTier != NoiseTier)
	{
		NoiseTier = NewTier;
		SoundEmitter->SetNoiseTier(NoiseTier);
		OnRep_NoiseTier();	// listen server / local
	}
}

void ABSHiderCharacter::OnRep_NoiseTier()
{
	OnNoiseTierChanged.Broadcast(NoiseTier);
}

// ---------------- Input ----------------
void ABSHiderCharacter::Input_Primary()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(BSTags::Ability_Throw));
	}
}
void ABSHiderCharacter::Input_Secondary()   { ServerInteract(); }
void ABSHiderCharacter::Input_SprintStart() { ServerSetSprinting(true); }
void ABSHiderCharacter::Input_SprintStop()  { ServerSetSprinting(false); }
void ABSHiderCharacter::Input_CrouchToggle(){ ServerToggleCrouch(); }

void ABSHiderCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	if (bEliminated) return;
	bSprinting = bNewSprinting && !bIsCrouched;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (bSprinting) ASC->AddLooseGameplayTag(BSTags::State_Sprinting);
		else            ASC->RemoveLooseGameplayTag(BSTags::State_Sprinting);
	}
}

void ABSHiderCharacter::ServerToggleCrouch_Implementation()
{
	if (bEliminated) return;
	if (bIsCrouched) UnCrouch(); else { bSprinting = false; Crouch(); }
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (bIsCrouched) ASC->AddLooseGameplayTag(BSTags::State_Crouching);
		else             ASC->RemoveLooseGameplayTag(BSTags::State_Crouching);
	}
}

void ABSHiderCharacter::ServerInteract_Implementation()
{
	if (bEliminated) return;

	FVector Eye; FRotator Rot;
	GetActorEyesViewPoint(Eye, Rot);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BSInteract), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + Rot.Vector() * InteractRange, ECC_Visibility, Params))
	{
		if (Hit.GetActor() && Hit.GetActor()->Implements<UBSInteractable>())
		{
			IBSInteractable::Execute_Interact(Hit.GetActor(), this);
			return;
		}
	}
	// Fallback: nearest spawn point within range (forgiving pickup radius).
	TryPickup();
}

void ABSHiderCharacter::TryPickup()
{
	if (!HasAuthority()) return;
	UBSAttributeSet* Attr = GetAttributeSet();
	if (!Attr || Attr->GetThrowables() >= Attr->GetMaxThrowables()) return;

	ABSThrowableSpawnPoint* Best = nullptr; float BestDist = InteractRange * InteractRange;
	for (TActorIterator<ABSThrowableSpawnPoint> It(GetWorld()); It; ++It)
	{
		const float D = FVector::DistSquared(It->GetActorLocation(), GetActorLocation());
		if (D < BestDist && It->HasStock()) { Best = *It; BestDist = D; }
	}
	if (Best && Best->TakeOne())
	{
		GetAbilitySystemComponent()->ApplyModToAttribute(UBSAttributeSet::GetThrowablesAttribute(), EGameplayModOp::Additive, 1.f);
		UBSSoundEventSubsystem::EmitSound(this, EBSSoundEventType::ItemPickup, GetActorLocation(), this);
		OnThrowablesChanged();
	}
}

void ABSHiderCharacter::OnThrowablesChanged()
{
	OnThrowablesChangedDelegate.Broadcast();
}

// ---------------- Elimination ----------------
void ABSHiderCharacter::ServerEliminate(AActor* Eliminator)
{
	if (!HasAuthority() || bEliminated) return;
	bEliminated = true;
	bSprinting = false;
	NoiseTier = EBSNoiseTier::Still;
	SoundEmitter->SetNoiseTier(NoiseTier);
	GetCharacterMovement()->DisableMovement();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(BSTags::State_Eliminated);
	}
	if (ABSPlayerState* PS = GetPlayerState<ABSPlayerState>())
	{
		PS->SetEliminated(true);
	}
	MulticastOnEliminated();

	if (ABSGameMode* GM = GetWorld()->GetAuthGameMode<ABSGameMode>())
	{
		GM->HandleHiderEliminated(GetPlayerState<ABSPlayerState>(), Eliminator);
	}
}

void ABSHiderCharacter::MulticastOnEliminated_Implementation()
{
	// GDD §6.4: brief "downed" hit-confirm rather than instant vanish. BP decides the look.
	BP_OnEliminated();
}
