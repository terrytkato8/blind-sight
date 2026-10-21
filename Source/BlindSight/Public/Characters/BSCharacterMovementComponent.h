// Copyright 2026 Kato.8 Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BSCharacterMovementComponent.generated.h"

/** Adds client-predicted sprint: the owning client sets bWantsToSprint, and it rides along with every move to the server. */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLINDSIGHT_API UBSCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Sprint") float SprintSpeed = 720.f;
	
	/** Set by input on the owning client; the server receives it through the saved-move flags.*/
	bool bWantsToSprint = false;
	
	UFUNCTION(BlueprintCallable, Category = "Sprint") bool IsSprinting() const;
	
	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	
};
