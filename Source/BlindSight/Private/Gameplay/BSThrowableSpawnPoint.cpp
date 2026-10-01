#include "Gameplay/BSThrowableSpawnPoint.h"
#include "Characters/BSHiderCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ABSThrowableSpawnPoint::ABSThrowableSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ABSThrowableSpawnPoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABSThrowableSpawnPoint, Stock);
}

void ABSThrowableSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority()) ResetStock();
}

void ABSThrowableSpawnPoint::ResetStock()
{
	Stock = InitialStock;
	OnRep_Stock();
	if (RefillSeconds > 0.f)
		GetWorldTimerManager().SetTimer(RefillTimer, this, &ABSThrowableSpawnPoint::Refill, RefillSeconds, true);
}

void ABSThrowableSpawnPoint::Refill()
{
	if (Stock < InitialStock) { ++Stock; OnRep_Stock(); }
}

bool ABSThrowableSpawnPoint::TakeOne()
{
	if (!HasAuthority() || Stock <= 0) return false;
	--Stock; OnRep_Stock();
	return true;
}

void ABSThrowableSpawnPoint::OnRep_Stock() { BP_OnStockChanged(Stock); }

void ABSThrowableSpawnPoint::Interact_Implementation(AActor* InstigatorActor)
{
	if (ABSHiderCharacter* Hider = Cast<ABSHiderCharacter>(InstigatorActor)) Hider->TryPickup();
}
