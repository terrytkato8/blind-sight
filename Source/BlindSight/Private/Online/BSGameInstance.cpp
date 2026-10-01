#include "Online/BSGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "BlindSight.h"

static const FName BS_SESSION_NAME = NAME_GameSession;

void UBSGameInstance::Init()
{
	Super::Init();
	if (IOnlineSessionPtr S = GetSessionInterface())
	{
		S->OnCreateSessionCompleteDelegates.AddUObject(this, &UBSGameInstance::HandleCreate);
		S->OnFindSessionsCompleteDelegates.AddUObject(this, &UBSGameInstance::HandleFind);
		S->OnJoinSessionCompleteDelegates.AddUObject(this, &UBSGameInstance::HandleJoin);
	}
}

IOnlineSessionPtr UBSGameInstance::GetSessionInterface() const
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	return OSS ? OSS->GetSessionInterface() : nullptr;
}

void UBSGameInstance::HostSession(const FString& LobbyMap, int32 MaxPlayers, bool bLAN)
{
	IOnlineSessionPtr S = GetSessionInterface();
	if (!S.IsValid()) { OnHostComplete.Broadcast(false); return; }
	PendingLobbyMap = LobbyMap;

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = FMath::Clamp(MaxPlayers, 2, 8);
	Settings.bIsLANMatch = bLAN;
	Settings.bShouldAdvertise = true;
	Settings.bUsesPresence = !bLAN;
	Settings.bUseLobbiesIfAvailable = !bLAN;
	Settings.bAllowJoinInProgress = false;	// rounds rotate in-map; joiners wait for the next session
	Settings.Set(FName("BS_BUILD"), FString("mvp"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	if (S->GetNamedSession(BS_SESSION_NAME)) S->DestroySession(BS_SESSION_NAME);
	S->CreateSession(0, BS_SESSION_NAME, Settings);
}

void UBSGameInstance::HandleCreate(FName SessionName, bool bOk)
{
	OnHostComplete.Broadcast(bOk);
	if (bOk && !PendingLobbyMap.IsEmpty())
	{
		UGameplayStatics::OpenLevel(this, FName(*PendingLobbyMap), true, TEXT("listen"));
	}
}

void UBSGameInstance::FindSessions(bool bLAN)
{
	IOnlineSessionPtr S = GetSessionInterface();
	if (!S.IsValid()) { OnFindComplete.Broadcast(0); return; }
	SearchSettings = MakeShared<FOnlineSessionSearch>();
	SearchSettings->bIsLanQuery = bLAN;
	SearchSettings->MaxSearchResults = 50;
	// SEARCH_PRESENCE is deprecated as of 5.6 and stops being a valid key next release.
	// Lobby discovery is driven by bUseLobbiesIfAvailable on the session settings instead.
	SearchSettings->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	S->FindSessions(0, SearchSettings.ToSharedRef());
}

void UBSGameInstance::HandleFind(bool bOk)
{
	OnFindComplete.Broadcast(bOk && SearchSettings.IsValid() ? SearchSettings->SearchResults.Num() : 0);
}

void UBSGameInstance::JoinFoundSession(int32 Index)
{
	IOnlineSessionPtr S = GetSessionInterface();
	if (!S.IsValid() || !SearchSettings.IsValid() || !SearchSettings->SearchResults.IsValidIndex(Index)) { OnJoinComplete.Broadcast(false); return; }
	S->JoinSession(0, BS_SESSION_NAME, SearchSettings->SearchResults[Index]);
}

void UBSGameInstance::HandleJoin(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr S = GetSessionInterface();
	FString Connect;
	if (Result == EOnJoinSessionCompleteResult::Success && S.IsValid() && S->GetResolvedConnectString(SessionName, Connect))
	{
		if (APlayerController* PC = GetFirstLocalPlayerController()) PC->ClientTravel(Connect, TRAVEL_Absolute);
		OnJoinComplete.Broadcast(true);
		return;
	}
	UE_LOG(LogBlindSight, Warning, TEXT("JoinSession failed: %d"), int32(Result));
	OnJoinComplete.Broadcast(false);
}

void UBSGameInstance::LeaveSession()
{
	if (IOnlineSessionPtr S = GetSessionInterface()) S->DestroySession(BS_SESSION_NAME);
}
