#include "GameFramework/BSPlayerController.h"
#include "GameFramework/BSGameState.h"
#include "GameFramework/BSGameMode.h"
#include "GameFramework/BSPlayerState.h"
#include "UI/BSHUDWidgets.h"
#include "Blueprint/UserWidget.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

void ABSPlayerController::ClientReceiveSoundPing_Implementation(const FBSSoundPing& Ping)
{
	OnSoundPing.Broadcast(Ping);
	if (UBSHunterHUDWidget* HunterHUD = Cast<UBSHunterHUDWidget>(ActiveHUD)) HunterHUD->OnSoundPing(Ping);

#if !UE_BUILD_SHIPPING
	if (bDebugDrawPings)
	{
		const float R = 40.f + 160.f * Ping.PerceivedIntensity;
		DrawDebugSphere(GetWorld(), Ping.Location, R, 12, Ping.bOccluded ? FColor::Orange : FColor::Cyan, false, Ping.Lifetime, 0, 2.f);
	}
#endif
}

void ABSPlayerController::ClientOnRoleAssigned_Implementation(EBSRole NewRole)
{
	if (ResultsWidget) { ResultsWidget->RemoveFromParent(); ResultsWidget = nullptr; }
	GetWorldTimerManager().ClearTimer(SpectateTimer);
	ShowHUDForRole(NewRole);
}

void ABSPlayerController::ShowHUDForRole(EBSRole InRole)
{
	if (ActiveHUD) { ActiveHUD->RemoveFromParent(); ActiveHUD = nullptr; }
	TSubclassOf<UBSHUDWidgetBase> Cls = (InRole == EBSRole::Hunter) ? HunterHUDClass : (InRole == EBSRole::Hider ? HiderHUDClass : nullptr);
	if (Cls)
	{
		ActiveHUD = CreateWidget<UBSHUDWidgetBase>(this, Cls);
		if (ActiveHUD) ActiveHUD->AddToViewport();
	}
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ABSPlayerController::ClientBeginSpectate_Implementation(float Delay)
{
	// Delay + Hunter-POV only: limits the info an eliminated player can relay to live friends (GDD §11.2).
	GetWorldTimerManager().SetTimer(SpectateTimer, this, &ABSPlayerController::DoSpectate, FMath::Max(Delay, 0.01f), false);
}

void ABSPlayerController::DoSpectate()
{
	const ABSGameState* GS = GetWorld()->GetGameState<ABSGameState>();
	if (GS && GS->GetHunterPlayerState() && GS->GetHunterPlayerState()->GetPawn())
	{
		SetViewTargetWithBlend(GS->GetHunterPlayerState()->GetPawn(), 0.5f);
	}
}

void ABSPlayerController::ClientShowResults_Implementation()
{
	if (ActiveHUD) { ActiveHUD->RemoveFromParent(); ActiveHUD = nullptr; }
	if (ResultsWidgetClass && !ResultsWidget)
	{
		ResultsWidget = CreateWidget<UUserWidget>(this, ResultsWidgetClass);
		if (ResultsWidget) ResultsWidget->AddToViewport(10);
	}
	SetInputMode(FInputModeGameAndUI());
	bShowMouseCursor = true;
}

void ABSPlayerController::BSStartRound()
{
	if (ABSGameMode* GM = GetWorld()->GetAuthGameMode<ABSGameMode>()) GM->StartRound();
}
