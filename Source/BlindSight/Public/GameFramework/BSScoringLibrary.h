#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BSScoringLibrary.generated.h"

/** Blackout scoring knobs (GDD §7.1). All tunable on the GameMode; defaults here. */
USTRUCT(BlueprintType)
struct FBSBlackoutScoring
{
	GENERATED_BODY()
	/** Base pot when everyone survives. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BasePool = 1000;
	/** Pot grows by this fraction of BasePool as survivors drop (fewer survivors -> bigger pot to split). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RiskBonus = 1.0f;
	/** Hunter points per catch when the round is NOT an all-caught sweep. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 HunterPointsPerCatch = 150;
};

/** Manhunt scoring knobs (GDD §7.2). */
USTRUCT(BlueprintType)
struct FBSManhuntScoring
{
	GENERATED_BODY()
	/** Winner-take-most payout for the last Hider standing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 SurvivorReward = 1000;
	/** Max consolation for 2nd place; scales down linearly to ~0 for first caught. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ConsolationMax = 300;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 HunterPointsPerCatch = 150;
};

UCLASS()
class BLINDSIGHT_API UBSScoringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Total pot split among survivors. Survivors=Total -> BasePool; Survivors->0 -> BasePool*(1+RiskBonus). */
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Scoring")
	static int32 BlackoutPot(const FBSBlackoutScoring& S, int32 TotalHiders, int32 Survivors);

	/** Each survivor's share of the pot (0 if no survivors). */
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Scoring")
	static int32 BlackoutSurvivorShare(const FBSBlackoutScoring& S, int32 TotalHiders, int32 Survivors);

	/** Hunter payout: full max pot on an all-caught sweep, otherwise per-catch. */
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Scoring")
	static int32 BlackoutHunterPoints(const FBSBlackoutScoring& S, int32 TotalHiders, int32 Catches, bool bAllCaught);

	/** Placement 1 = Survivor. Others scale from ConsolationMax (2nd) down to 0 (last place). */
	UFUNCTION(BlueprintPure, Category = "Blind Sight|Scoring")
	static int32 ManhuntPlacementPoints(const FBSManhuntScoring& S, int32 TotalHiders, int32 Placement);

	UFUNCTION(BlueprintPure, Category = "Blind Sight|Scoring")
	static int32 ManhuntHunterPoints(const FBSManhuntScoring& S, int32 Catches);
};
