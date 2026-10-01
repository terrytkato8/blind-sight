#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/BSTypes.h"
#include "BSHUDWidgets.generated.h"

class ABSGameState;
class ABSPlayerState;

/**
 * Minimal HUD bases (GDD §9). Layout is done in UMG child Blueprints (WBP_HunterHUD / WBP_HiderHUD);
 * these classes wire up the data and expose BlueprintImplementableEvents.
 */
UCLASS(Abstract)
class BLINDSIGHT_API UBSHUDWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Blind Sight") ABSGameState* GetBSGameState() const;
	UFUNCTION(BlueprintPure, Category = "Blind Sight") ABSPlayerState* GetBSPlayerState() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void OnPhaseChanged(EBSRoundPhase Phase);
	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void OnRemainingHidersChanged(int32 Remaining, int32 Total);
	/** Fires every tick while a round clock is active (Blackout). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void OnRoundTimeUpdated(float SecondsRemaining);
	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void OnNextHunterChanged(const FString& NextHunterName);

protected:
	UFUNCTION() void HandlePhase(EBSRoundPhase Phase);
	UFUNCTION() void HandleRemaining(int32 Remaining);
};

/** Hunter HUD: ammo counter, world-space/compass pings, Pulse cooldown. */
UCLASS(Abstract)
class BLINDSIGHT_API UBSHunterHUDWidget : public UBSHUDWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Called by the PlayerController for each perceived ping. Render as world-space ripple or compass blip. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void OnSoundPing(const FBSSoundPing& Ping);
	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void OnAmmoChanged(int32 Ammo, int32 MaxAmmo);
	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void OnPulseCooldownUpdated(float SecondsRemaining, bool bPulseEnabled);

	/** Helper: project a ping to a screen-edge compass bearing relative to the camera yaw (-180..180). */
	UFUNCTION(BlueprintPure, Category = "Blind Sight") float GetBearingToLocation(FVector WorldLocation) const;

protected:
	UFUNCTION() void HandleAmmo(float NewAmmo);
	bool bBoundAmmo = false;
};

/** Hider HUD: throwable count, remaining Hiders, round timer (Blackout), own noise tier. */
UCLASS(Abstract)
class BLINDSIGHT_API UBSHiderHUDWidget : public UBSHUDWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void OnThrowablesChanged(int32 Count, int32 Max);
	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void OnNoiseTierChanged(EBSNoiseTier Tier);
	UFUNCTION(BlueprintImplementableEvent, Category = "Blind Sight") void OnEliminated();

protected:
	UFUNCTION() void HandleNoiseTier(EBSNoiseTier Tier);
	UFUNCTION() void HandleThrowables();
	bool bBound = false;
	bool bReportedElimination = false;
};
