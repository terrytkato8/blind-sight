#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Core/BSTypes.h"
#include "BSPlayerState.generated.h"

class UBSAbilitySystemComponent;
class UBSAttributeSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnRoleChanged, EBSRole, NewRole);

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
	UPROPERTY() FString BackendPlayerId;
	int32 ShotsFiredThisRound = 0;
	int32 ThrowablesUsedThisRound = 0;
	float SurvivalSeconds = 0.f;

	UFUNCTION() void OnRep_Role();
};
