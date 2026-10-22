#include "Sound/BSSoundEmitterComponent.h"
#include "Sound/BSSoundEventSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UBSSoundEmitterComponent::UBSSoundEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(false);
}

void UBSSoundEmitterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;

	const FVector Loc = Owner->GetActorLocation();
	if (!bHasLast) { LastLocation = Loc; bHasLast = true; return; }

	// Only count grounded horizontal movement.
	const ACharacter* Char = Cast<ACharacter>(Owner);
	const bool bGrounded = !Char || (Char->GetCharacterMovement() && Char->GetCharacterMovement()->IsMovingOnGround());
	const float Moved = bGrounded ? FVector::Dist2D(Loc, LastLocation) : 0.f;
	LastLocation = Loc;

	if (CurrentTier == EBSNoiseTier::Still) { DistanceAccum = 0.f; return; }// actually stopped JRB 10.20.26
	if (Moved <= KINDA_SMALL_NUMBER) return; //no move arrived this frame (common under lag); keep the stride progress

	DistanceAccum += Moved;
	float Stride = StrideWalk; EBSSoundEventType Type = EBSSoundEventType::FootstepWalk;
	switch (CurrentTier)
	{
	case EBSNoiseTier::Crouch: Stride = StrideCrouch; Type = EBSSoundEventType::FootstepCrouch; break;
	case EBSNoiseTier::Sprint: Stride = StrideSprint; Type = EBSSoundEventType::FootstepSprint; break;
	default: break;
	}

	if (DistanceAccum >= Stride)
	{
		DistanceAccum -= Stride;
		Emit(Type);
	}
}

void UBSSoundEmitterComponent::Emit(EBSSoundEventType Type, float IntensityScale)
{
	if (AActor* Owner = GetOwner())
	{
		UBSSoundEventSubsystem::EmitSound(this, Type, Owner->GetActorLocation(), Owner, IntensityScale);
	}
}
