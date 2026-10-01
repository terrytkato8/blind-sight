#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/BSInteractable.h"
#include "BSDoor.generated.h"

class UStaticMeshComponent;

/** Environmental interaction with a medium, localized sound (GDD §6.1). Encourages route variety. */
UCLASS()
class BLINDSIGHT_API ABSDoor : public AActor, public IBSInteractable
{
	GENERATED_BODY()

public:
	ABSDoor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<USceneComponent> Pivot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "Door") float OpenYaw = 95.f;
	UPROPERTY(EditAnywhere, Category = "Door") float SwingSeconds = 0.5f;

	virtual void Interact_Implementation(AActor* InstigatorActor) override;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated) bool bOpen = false;
};
