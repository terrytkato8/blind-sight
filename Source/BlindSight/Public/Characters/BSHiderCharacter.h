#pragma once

#include "CoreMinimal.h"
#include "Characters/BSCharacterBase.h"
#include "BSHiderCharacter.generated.h"

class ABSThrowableSpawnPoint;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnNoiseTierChanged, EBSNoiseTier, NewTier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBSOnThrowablesChanged);

/** Hider (GDD §4.2): three-tier noise movement, throwable stock, elimination on hit. */
UCLASS()
class BLINDSIGHT_API ABSHiderCharacter : public ABSCharacterBase
{
	GENERATED_BODY()

public:
	ABSHiderCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Movement") float WalkSpeed = 420.f;
	UPROPERTY(EditDefaultsOnly, Category = "Movement") float SprintSpeed = 720.f;
	UPROPERTY(EditDefaultsOnly, Category = "Movement") float CrouchSpeed = 200.f;
	UPROPERTY(EditDefaultsOnly, Category = "Interaction") float InteractRange = 220.f;

	UFUNCTION(BlueprintPure, Category = "Blind Sight") EBSNoiseTier GetNoiseTier() const { return NoiseTier; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") bool IsSprinting() const { return bSprinting; }

	/** Server: eliminate this Hider. Notifies GameMode for placement/scoring. */
	void ServerEliminate(AActor* Eliminator);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnEliminated();

	/** Cosmetic hook for BP (ragdoll, "downed" pose, comedic yelp). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight")
	void BP_OnEliminated();

	/** Server: attempt to take one throwable from a spawn point in range. */
	void TryPickup();
	void OnThrowablesChanged();

	UPROPERTY(BlueprintAssignable) FBSOnNoiseTierChanged OnNoiseTierChanged;
	UPROPERTY(BlueprintAssignable) FBSOnThrowablesChanged OnThrowablesChangedDelegate;

protected:
	virtual void Input_Primary() override;		// throw
	virtual void Input_Secondary() override;	// interact / pickup
	virtual void Input_SprintStart() override;
	virtual void Input_SprintStop() override;
	virtual void Input_CrouchToggle() override;

	UFUNCTION(Server, Reliable) void ServerSetSprinting(bool bNewSprinting);
	UFUNCTION(Server, Reliable) void ServerToggleCrouch();
	UFUNCTION(Server, Reliable) void ServerInteract();

	void UpdateNoiseTier();

	UPROPERTY(ReplicatedUsing = OnRep_NoiseTier) EBSNoiseTier NoiseTier = EBSNoiseTier::Still;
	UPROPERTY(Replicated) bool bSprinting = false;

	UFUNCTION() void OnRep_NoiseTier();
};
