#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/BSTypes.h"
#include "BSPlayerController.generated.h"

class UBSHUDWidgetBase;
class UUserWidget;
class UBSSkillDefinition;

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

	// ---- Lobby: call these from the owning player's machine (ready pad, vote spot, host UI). ----
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Lobby") void SetLobbyReady(bool bReady);
	/** MapIndex is an index into the lobby game state's map options; -1 clears the vote. */
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Lobby") void VoteLobbyMap(int32 MapIndex);
	/** Host only; ignored for everyone else and while the lobby isn't ready. */
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Lobby") void RequestStartMatch();
	/** True only for the listen-server host's own controller. Use this to decide who sees the host UI. */
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Lobby") bool IsLobbyHost() const { return HasAuthority() && IsLocalController(); }

	//---- Skill: call these from the owning player's machine (lobby skill picker). ----
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Skills") void EquipSkill(UBSSkillDefinition* Skill);
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Skills") void UnEquipSkill(UBSSkillDefinition* Skill);
	
	/** Console: server-only shortcut to start the next round from a listen server. */
	UFUNCTION(Exec) void BSStartRound();

protected:
	UFUNCTION(Server, Reliable) void ServerSetLobbyReady(bool bReady);
	UFUNCTION(Server, Reliable) void ServerVoteLobbyMap(int32 MapIndex);
	UFUNCTION(Server, Reliable) void ServerRequestStartMatch();
	UFUNCTION(Server, Reliable) void ServerEquipSkill(UBSSkillDefinition* Skill);
	UFUNCTION(Server, Reliable) void ServerUnEquipSkill(UBSSkillDefinition* Skill);
	
	void ShowHUDForRole(EBSRole InRole);
	void DoSpectate();

	UPROPERTY() TObjectPtr<UBSHUDWidgetBase> ActiveHUD;
	UPROPERTY() TObjectPtr<UUserWidget> ResultsWidget;
	FTimerHandle SpectateTimer;
};
