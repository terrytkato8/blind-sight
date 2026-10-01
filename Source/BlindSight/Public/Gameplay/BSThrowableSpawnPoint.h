#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/BSInteractable.h"
#include "BSThrowableSpawnPoint.generated.h"

class UStaticMeshComponent;
class USphereComponent;

/**
 * World stash of throwables (GDD §6.3 resupply loop). Limited stock, optional slow refill.
 * Level design places these at a mix of safe and exposed spots (GDD §8).
 */
UCLASS()
class BLINDSIGHT_API ABSThrowableSpawnPoint : public AActor, public IBSInteractable
{
	GENERATED_BODY()

public:
	ABSThrowableSpawnPoint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stock", meta = (ClampMin = "0")) int32 InitialStock = 3;
	/** Seconds per +1 refill; 0 = never refills within a round. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stock", meta = (ClampMin = "0")) float RefillSeconds = 0.f;

	UFUNCTION(BlueprintPure, Category = "Blind Sight") bool HasStock() const { return Stock > 0; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") int32 GetStock() const { return Stock; }

	/** Server: decrement stock. Returns false if empty. */
	bool TakeOne();
	/** Server: reset to InitialStock at round start. */
	void ResetStock();

	virtual void Interact_Implementation(AActor* InstigatorActor) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void BP_OnStockChanged(int32 NewStock);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_Stock) int32 Stock = 0;
	UFUNCTION() void OnRep_Stock();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FTimerHandle RefillTimer;
	void Refill();
};
