#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Core/BSTypes.h"
#include "BSCharacterBase.generated.h"

class UBSAbilitySystemComponent;
class UBSAttributeSet;
class UBSSoundEmitterComponent;
class UInputMappingContext;
class UInputAction;
class UCameraComponent;
struct FInputActionValue;

/**
 * Shared base for Hunter and Hider. The ASC lives on ABSPlayerState so it survives
 * round-to-round repossession during role rotation (GDD §4.3).
 */
UCLASS(Abstract)
class BLINDSIGHT_API ABSCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABSCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UBSAttributeSet* GetAttributeSet() const;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Blind Sight") EBSRole GetBSRole() const;
	UFUNCTION(BlueprintPure, Category = "Blind Sight") bool IsEliminated() const { return bEliminated; }

	/** Server: freeze/unfreeze during head start (Hunter frozen; Hiders free). */
	void SetHeadStartFrozen(bool bFrozen);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBSSoundEmitterComponent> SoundEmitter;

	// ---- Enhanced Input (assign the assets in the Blueprint child) ----
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Move;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Look;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Primary;	// Fire / Throw
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Secondary;	// Pulse / Interact
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Sprint;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Crouch;

	/** Abilities granted when this character is possessed (server). */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

protected:
	virtual void InitAbilityActorInfo();
	virtual void GrantStartupAbilities();
	virtual void OnAbilitySystemReady() {}

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	virtual void Input_Primary() {}
	virtual void Input_Secondary() {}
	virtual void Input_SprintStart() {}
	virtual void Input_SprintStop() {}
	virtual void Input_CrouchToggle() {}

	UPROPERTY(ReplicatedUsing = OnRep_Eliminated, BlueprintReadOnly, Category = "Blind Sight")
	bool bEliminated = false;

	UPROPERTY(Replicated)
	bool bHeadStartFrozen = false;

	UFUNCTION() virtual void OnRep_Eliminated();

	bool bAbilitiesGranted = false;
};
