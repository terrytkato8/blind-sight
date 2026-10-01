#include "Gameplay/BSDoor.h"
#include "Sound/BSSoundEventSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

ABSDoor::ABSDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	SetRootComponent(Pivot);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Pivot);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ABSDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABSDoor, bOpen);
}

void ABSDoor::Interact_Implementation(AActor* InstigatorActor)
{
	if (!HasAuthority()) return;
	bOpen = !bOpen;
	UBSSoundEventSubsystem::EmitSound(this, EBSSoundEventType::Door, GetActorLocation(), InstigatorActor);
}

void ABSDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const float Target = bOpen ? OpenYaw : 0.f;
	const float Current = Mesh->GetRelativeRotation().Yaw;
	const float Next = FMath::FInterpConstantTo(Current, Target, DeltaTime, FMath::Abs(OpenYaw) / FMath::Max(SwingSeconds, 0.01f));
	Mesh->SetRelativeRotation(FRotator(0.f, Next, 0.f));
}
