#pragma once

#include "CoreMinimal.h"
#include "GameFramework/BSGameMode.h"
#include "GameFramework/BSScoringLibrary.h"
#include "BSGameMode_Manhunt.generated.h"

/**
 * Manhunt — Last One Standing (GDD §7.2). No clock; ends when exactly one Hider remains.
 * Higher ammo; winner-take-most for the Survivor, consolation by placement, Hunter paid per catch.
 */
UCLASS()
class BLINDSIGHT_API ABSGameMode_Manhunt : public ABSGameMode
{
	GENERATED_BODY()

public:
	ABSGameMode_Manhunt();

	/** Ammo = NumHiders + ExtraAmmo (needs to realistically work through the lobby). */
	UPROPERTY(EditDefaultsOnly, Category = "Manhunt") int32 ExtraAmmo = 2;
	/** Safety valve so a passive Hunter can't stall forever; 0 = truly no clock. */
	UPROPERTY(EditDefaultsOnly, Category = "Manhunt") float HardCapSeconds = 0.f;
	UPROPERTY(EditDefaultsOnly, Category = "Manhunt") FBSManhuntScoring Scoring;

	virtual float GetRoundDurationSeconds() const override { return HardCapSeconds; }
	virtual int32 GetHunterAmmo(int32 NumHiders) const override { return NumHiders + ExtraAmmo; }
	virtual void CheckRoundEndAfterElimination() override;
	virtual TArray<FBSRoundScoreEntry> ResolveScoring(EBSRoundEndReason Reason) override;
};
