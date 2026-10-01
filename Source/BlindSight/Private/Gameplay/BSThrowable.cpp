#include "Gameplay/BSThrowable.h"
#include "Sound/BSSoundEventSubsystem.h"
#include "Components/StaticMeshComponent.h"

ABSThrowable::ABSThrowable()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetNotifyRigidBodyCollision(true);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);	// don't let it hit the thrower
	Mesh->SetMassOverrideInKg(NAME_None, 0.6f, true);
}

void ABSThrowable::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Mesh->OnComponentHit.AddDynamic(this, &ABSThrowable::OnHit);
		SetLifeSpan(LifeSeconds);
	}
}

void ABSThrowable::Launch(const FVector& Velocity)
{
	if (!HasAuthority()) return;
	Mesh->SetPhysicsLinearVelocity(Velocity);
	Mesh->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * 360.f);
}

void ABSThrowable::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority()) return;
	if (Mesh->GetPhysicsLinearVelocity().Size() < MinImpactSpeed && ImpactCount > 0) return;

	const int32 Idx = ImpactCount++;
	if (Idx == 0)
	{
		// The loud one — this is the decoy/sabotage/lure data point (GDD §6.3).
		UBSSoundEventSubsystem::EmitSound(this, ImpactEventType, Hit.ImpactPoint, GetInstigator(), ImpactIntensityScale);
	}
	else if (Idx <= MaxBounceEvents && BounceIntensityScale > 0.f)
	{
		UBSSoundEventSubsystem::EmitSound(this, ImpactEventType, Hit.ImpactPoint, GetInstigator(), ImpactIntensityScale * BounceIntensityScale);
	}
}
