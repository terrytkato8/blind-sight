#include "GameFramework/BSScoringLibrary.h"

int32 UBSScoringLibrary::BlackoutPot(const FBSBlackoutScoring& S, int32 TotalHiders, int32 Survivors)
{
	if (TotalHiders <= 0) return 0;
	const float Caught = float(FMath::Clamp(TotalHiders - Survivors, 0, TotalHiders));
	return FMath::RoundToInt(S.BasePool * (1.f + S.RiskBonus * (Caught / TotalHiders)));
}

int32 UBSScoringLibrary::BlackoutSurvivorShare(const FBSBlackoutScoring& S, int32 TotalHiders, int32 Survivors)
{
	if (Survivors <= 0) return 0;
	return BlackoutPot(S, TotalHiders, Survivors) / Survivors;
}

int32 UBSScoringLibrary::BlackoutHunterPoints(const FBSBlackoutScoring& S, int32 TotalHiders, int32 Catches, bool bAllCaught)
{
	if (bAllCaught) return BlackoutPot(S, TotalHiders, 0);	// the Hunter takes the entire (max) pot
	return S.HunterPointsPerCatch * Catches;
}

int32 UBSScoringLibrary::ManhuntPlacementPoints(const FBSManhuntScoring& S, int32 TotalHiders, int32 Placement)
{
	if (Placement <= 1) return S.SurvivorReward;
	if (TotalHiders <= 2) return 0;
	// 2nd -> ConsolationMax, last -> 0
	const float T = float(TotalHiders - Placement) / float(TotalHiders - 2);
	return FMath::RoundToInt(S.ConsolationMax * FMath::Clamp(T, 0.f, 1.f));
}

int32 UBSScoringLibrary::ManhuntHunterPoints(const FBSManhuntScoring& S, int32 Catches)
{
	return S.HunterPointsPerCatch * Catches;
}
