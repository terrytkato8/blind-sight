#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/BSTypes.h"
#include "BSPlayerController.generated.h"

class UBSHUDWidgetBase;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnSoundPing, const FBSSoundPing&, Ping);

/** Client endpoint for Hunter pings, HUD swapping per role, spectating and results. */
UCLASS()
class BLINDSIGHT_API ABSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Server -> this Hunter client only. Already attenuated/occluded. */
	UFUNCTION(Client, Reliable)
	void ClientReceiveSoundPing(const FBSSoundPing& Ping);

	/** Server -> client: swap HUD for the role assigned this round. */
	UFUNCTION(Client, Reliable)
	void ClientOnRoleAssigned(EBSRole NewRole);

	/** Server -> eliminated client: after Delay seconds, spectate the Hunter's POV (GDD §11.2 policy: hunter-POV spectate). */
	UFUNCTION(Client, Reliable)
	void ClientBeginSpectate(float Delay);

	UFUNCTION(Client, Reliable)
	void ClientShowResults();

	UPROPERTY(BlueprintAssignable, Category = "Blind Sight") FBSOnSoundPing OnSoundPing;

	/** Assign in BP_PlayerController. */
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UBSHUDWidgetBase> HunterHUDClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UBSHUDWidgetBase> HiderHUDClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UUserWidget> ResultsWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "Debug") bool bDebugDrawPings = true;

	UFUNCTION(BlueprintPure, Category = "Blind Sight") UBSHUDWidgetBase* GetActiveHUD() const { return ActiveHUD; }

	/** Console: server-only shortcut to start the next round from a listen server. */
	UFUNCTION(Exec) void BSStartRound();

protected:
	void ShowHUDForRole(EBSRole InRole);
	void DoSpectate();

	UPROPERTY() TObjectPtr<UBSHUDWidgetBase> ActiveHUD;
	UPROPERTY() TObjectPtr<UUserWidget> ResultsWidget;
	FTimerHandle SpectateTimer;
};
