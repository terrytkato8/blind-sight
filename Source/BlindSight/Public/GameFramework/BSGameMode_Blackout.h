#pragma once

#include "CoreMinimal.h"
#include "GameFramework/BSGameMode.h"
#include "GameFramework/BSScoringLibrary.h"
#include "BSGameMode_Blackout.generated.h"

/**
 * Blackout — Timed Round (GDD §7.1). Shared-fate sprint under a clock.
 * Low ammo; survivors split a pot that grows as fewer survive; all-caught hands the whole pot to the Hunter.
 */
UCLASS()
class BLINDSIGHT_API ABSGameMode_Blackout : public ABSGameMode
{
	GENERATED_BODY()

public:
	ABSGameMode_Blackout();

	UPROPERTY(EditDefaultsOnly, Category = "Blackout") float RoundSeconds = 270.f;		// 4.5 min, tunable 4–5
	UPROPERTY(EditDefaultsOnly, Category = "Blackout") int32 BaseAmmo = 2;
	/** Extra bullet per this many Hiders (so a 7-Hider lobby isn't hopeless). */
	UPROPERTY(EditDefaultsOnly, Category = "Blackout") int32 HidersPerExtraBullet = 3;
	UPROPERTY(EditDefaultsOnly, Category = "Blackout") FBSBlackoutScoring Scoring;

	virtual float GetRoundDurationSeconds() const override { return RoundSeconds; }
	virtual int32 GetHunterAmmo(int32 NumHiders) const override;
	virtual void CheckRoundEndAfterElimination() override;
	virtual TArray<FBSRoundScoreEntry> ResolveScoring(EBSRoundEndReason Reason) override;
};
