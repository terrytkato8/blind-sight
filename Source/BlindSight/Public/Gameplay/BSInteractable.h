#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BSInteractable.generated.h"

UINTERFACE(BlueprintType)
class UBSInteractable : public UInterface { GENERATED_BODY() };

/** Anything a Hider can press "interact" on (doors, spawn points, switches). Executed on the server. */
class BLINDSIGHT_API IBSInteractable
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Blind Sight")
	void Interact(AActor* InstigatorActor);
};
