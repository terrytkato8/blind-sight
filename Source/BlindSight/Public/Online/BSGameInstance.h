#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "BSGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnSessionResult, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnSessionsFound, int32, Count);

/**
 * Thin session wrapper over IOnlineSession (GDD §12: EOS via OnlineSubsystemEOS).
 * Works with the NULL subsystem for LAN testing; switch DefaultPlatformService=EOS in DefaultEngine.ini.
 */
UCLASS()
class BLINDSIGHT_API UBSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Online")
	void HostSession(const FString& LobbyMap, int32 MaxPlayers, bool bLAN);

	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Online")
	void FindSessions(bool bLAN);

	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Online")
	void JoinFoundSession(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Online")
	void LeaveSession();

	UPROPERTY(BlueprintAssignable) FBSOnSessionResult OnHostComplete;
	UPROPERTY(BlueprintAssignable) FBSOnSessionsFound OnFindComplete;
	UPROPERTY(BlueprintAssignable) FBSOnSessionResult OnJoinComplete;

private:
	void HandleCreate(FName SessionName, bool bOk);
	void HandleFind(bool bOk);
	void HandleJoin(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	IOnlineSessionPtr GetSessionInterface() const;
	TSharedPtr<FOnlineSessionSearch> SearchSettings;
	FString PendingLobbyMap;
};
