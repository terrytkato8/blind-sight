#include "GameFramework/BSLobbyGameMode.h"
#include "GameFramework/BSGameMode.h"
#include "GameFramework/BSPlayerState.h"
#include "GameFramework/BSPlayerController.h"
#include "BlindSight.h"

ABSLobbyGameMode::ABSLobbyGameMode()
{
	DefaultPawnClass = nullptr;
	bStartPlayersAsSpectators = true;
	bUseSeamlessTravel = true;
	PlayerStateClass = ABSPlayerState::StaticClass();
	PlayerControllerClass = ABSPlayerController::StaticClass();
}

void ABSLobbyGameMode::StartMatch(const FString& MapName, TSubclassOf<ABSGameMode> ModeClass)
{
	if (!ModeClass) { UE_LOG(LogBlindSight, Warning, TEXT("StartMatch: no mode class")); return; }
	const FString URL = FString::Printf(TEXT("%s?game=%s?listen"), *MapName, *ModeClass->GetPathName());
	GetWorld()->ServerTravel(URL, false);
}
