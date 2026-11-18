#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Core/BSTypes.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffectTypes.h"
#include "BSPlayerState.generated.h"

class UBSAbilitySystemComponent;
class UBSAttributeSet;
class UBSSkillDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnRoleChanged, EBSRole, NewRole);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBSOnLobbyPlayerChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBSOnLoadoutChanged);

/** Owns the ASC so it persists across repossession during role rotation. Holds session score + round state. */
UCLASS()
class BLINDSIGHT_API ABSPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABSPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UBSAttributeSet* GetAttributeSet() const { return AttributeSet; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Blind Sight") EBSRole GetBSRole() const { return BSRole; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") bool IsEliminated() const { return bEliminated; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") int32 GetTotalPoints() const { return TotalPoints; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") int32 GetPlacement() const { return Placement; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") int32 GetCatchesThisRound() const { return CatchesThisRound; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") int32 GetRotationIndex() const { return RotationIndex; }
	UFUNCTION(BlueprintPure, Category = "Blind Sight") int32 GetHunterTurnsTaken() const { return HunterTurnsTaken; }

	/** Backend account id, set by the server from the join URL. Empty in offline play. */
	UFUNCTION(BlueprintPure, Category = "Blind Sight") const FString& GetBackendPlayerId() const { return BackendPlayerId; }
	void SetBackendPlayerId(const FString& In) { BackendPlayerId = In; }

	// ---- Lobby (ready-up + map vote). Reset naturally on travel: not copied to the match PlayerState. ----
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Lobby") bool IsLobbyReady() const { return bLobbyReady; }
	/** Index into the lobby's map options, or -1 for no vote. */
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Lobby") int32 GetLobbyMapVote() const { return LobbyMapVote; }
	void SetLobbyReady(bool bNew);
	void SetLobbyMapVote(int32 MapIndex);

	// ---- Skill loadout. Picked in the lobby, copied to the match PlayerState on travel. ----
	/** How many skills a player may equip for each role. */
	static constexpr int32 MaxSkillsPerRole = 2;
	
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Skills")
	TArray<UBSSkillDefinition*> GetEquippedSkills() const;
	
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Skills")
	bool HasSkillEquipped(UBSSkillDefinition* Skill) const;
	
	/** Server only. Returns false if the skill is null, already equipped, or the role's slots are full.*/
	bool EquipSkill(UBSSkillDefinition* Skill);
	void UnEquipSkill(UBSSkillDefinition* Skill);
	
	/** Server only. Grants the equipped skills that fit this round's role. Call after respawn and attribute init. */
	void ApplySkillsForRound();
	/** Server only. Takes away everything ApplySkillsForRound granted. */
	void RemoveSkillsForRound();
	
	// Per-round stats gathered for telemetry and progression.
	void AddShotFired() { ++ShotsFiredThisRound; }
	void AddThrowableUsed() { ++ThrowablesUsedThisRound; }
	int32 GetShotsFiredThisRound() const { return ShotsFiredThisRound; }
	int32 GetThrowablesUsedThisRound() const { return ThrowablesUsedThisRound; }
	float GetSurvivalSeconds() const { return SurvivalSeconds; }
	void SetSurvivalSeconds(float S) { SurvivalSeconds = S; }

	// Server setters
	void SetBSRole(EBSRole NewRole);
	void SetEliminated(bool bNew) { bEliminated = bNew; }
	void SetPlacement(int32 P) { Placement = P; }
	void AddCatch() { ++CatchesThisRound; }
	void AddPoints(int32 P) { TotalPoints += P; }
	void SetRotationIndex(int32 I) { RotationIndex = I; }
	void IncrementHunterTurns() { ++HunterTurnsTaken; }
	void ResetForRound();

	UPROPERTY(BlueprintAssignable) FBSOnRoleChanged OnRoleChanged;
	/** Fires on every machine when this player's ready state or map vote changes. */
	UPROPERTY(BlueprintAssignable, Category = "Blind Sight|Lobby") FBSOnLobbyPlayerChanged OnLobbyPlayerChanged;
	UPROPERTY(BlueprintAssignable) FBSOnLoadoutChanged OnLoadoutChanged;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBSAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY() TObjectPtr<UBSAttributeSet> AttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_Role) EBSRole BSRole = EBSRole::None;
	UPROPERTY(Replicated) bool bEliminated = false;
	UPROPERTY(Replicated) int32 TotalPoints = 0;
	UPROPERTY(Replicated) int32 Placement = 0;
	UPROPERTY(Replicated) int32 CatchesThisRound = 0;
	UPROPERTY(Replicated) int32 RotationIndex = 0;
	UPROPERTY(Replicated) int32 HunterTurnsTaken = 0;
	UPROPERTY(ReplicatedUsing = OnRep_Lobby) bool bLobbyReady = false;
	UPROPERTY(ReplicatedUsing = OnRep_Lobby) int32 LobbyMapVote = -1;
	UPROPERTY(ReplicatedUsing = OnRep_Loadout) TArray<TObjectPtr<UBSSkillDefinition>> EquippedSkills;
	//What this round's skills gave us, so it can be taken away again. Server only, not replciated.
	TArray<FGameplayAbilitySpecHandle> GrantedSkillAbilities;
	TArray<FActiveGameplayEffectHandle> ActiveSkillEffects;
	
	UPROPERTY() FString BackendPlayerId;
	int32 ShotsFiredThisRound = 0;
	int32 ThrowablesUsedThisRound = 0;
	float SurvivalSeconds = 0.f;

	UFUNCTION() void OnRep_Role();
	UFUNCTION() void OnRep_Lobby();
	UFUNCTION() void OnRep_Loadout();
	
	virtual void CopyProperties(APlayerState* PlayerState) override;
};
