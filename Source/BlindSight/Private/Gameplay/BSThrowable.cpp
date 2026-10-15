#include "Gameplay/BSThrowable.h"

#include "BlindSight.h"
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
}

void ABSThrowable::BeginPlay()
{
	Super::BeginPlay();
	SpawnTime = GetWorld()->GetTimeSeconds();
	if (HasAuthority())
	{
		Mesh->OnComponentHit.AddDynamic(this, &ABSThrowable::OnHit);
		
	}
}

void ABSThrowable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Mesh->SetMassOverrideInKg(NAME_None, 0.6f, true);
}

void ABSThrowable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogBlindSight, Log, TEXT("Throwable %s [%s] gone after %.1fs at z=%0.f(reason %d)"),
		*GetName(), GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server"),
		GetWorld()->GetTimeSeconds() - SpawnTime, GetActorLocation().Z, int32(EndPlayReason));
	Super::EndPlay(EndPlayReason);
}

void ABSThrowable::Launch(const FVector& Velocity)
{
	if (!HasAuthority()) return;
	bThrown = true;
	SetLifeSpan(LifeSeconds);
	Mesh->SetPhysicsLinearVelocity(Velocity);
	Mesh->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * 360.f);
}

void ABSThrowable::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bThrown) return;   // placed props settling on the floor don't ping the hunter
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
