#pragma once

#include "CoreMinimal.h"
#include "Characters/BSCharacterBase.h"
#include "BSHunterCharacter.generated.h"

class UPostProcessComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnAmmoChanged, float, NewAmmo);

/**
 * Hunter (GDD §4.1): near-blind, normal speed, hard ammo budget, optional Pulse.
 * Vision impairment is a heavy post-process on this character's own camera only.
 */
UCLASS()
class BLINDSIGHT_API ABSHunterCharacter : public ABSCharacterBase
{
	GENERATED_BODY()

public:
	ABSHunterCharacter();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Vision post-process (GDD §6.2). Assign PP_HunterVision material in BP for the blur/short-radius look. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPostProcessComponent> VisionPostProcess;

	UPROPERTY(EditDefaultsOnly, Category = "Vision") float VignetteIntensity = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Vision") float Saturation = 0.12f;
	/** Effective sight radius in cm — "final confirmation" range (GDD §6.2). Read by PP material via MPC/param. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vision") float SightRadius = 250.f;

	// ---- Pulse (GDD §4.1, stretch; toggle per match) ----
	UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadOnly, Category = "Pulse") bool bPulseEnabled = true;
	float GetPulseCooldownRemaining() const;
	void StartPulseCooldown(float Seconds);

	/** Client: briefly reveal Hider silhouettes within radius (custom depth outline). */
	UFUNCTION(Client, Reliable)
	void ClientPulseReveal(float Radius, float Duration);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastOnFired(FVector ImpactPoint);

	/** Cosmetic hooks. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void BP_OnFired(FVector ImpactPoint);
	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void BP_OnPulse(float Radius, float Duration);

	UPROPERTY(BlueprintAssignable) FBSOnAmmoChanged OnAmmoChanged;

protected:
	virtual void Input_Primary() override;		// fire
	virtual void Input_Secondary() override;	// pulse
	virtual void OnAbilitySystemReady() override;
	virtual void NotifyControllerChanged() override;

	UPROPERTY(ReplicatedUsing = OnRep_PulseReadyTime) float PulseReadyServerTime = 0.f;
	UFUNCTION() void OnRep_PulseReadyTime() {}

	void ApplyVisionSettings();
	void ClearPulseReveal();
	TArray<TWeakObjectPtr<AActor>> RevealedActors;
	FTimerHandle RevealTimer;
	FDelegateHandle AmmoDelegateHandle;
};
