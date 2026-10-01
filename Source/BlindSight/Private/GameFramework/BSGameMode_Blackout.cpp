#include "GameFramework/BSGameMode_Blackout.h"
#include "GameFramework/BSPlayerState.h"
#include "GameFramework/BSGameState.h"

ABSGameMode_Blackout::ABSGameMode_Blackout()
{
	ModeDisplayName = NSLOCTEXT("BlindSight", "ModeBlackout", "Blackout");
}

int32 ABSGameMode_Blackout::GetHunterAmmo(int32 NumHiders) const
{
	return BaseAmmo + (HidersPerExtraBullet > 0 ? NumHiders / HidersPerExtraBullet : 0);
}

void ABSGameMode_Blackout::CheckRoundEndAfterElimination()
{
	if (RemainingHiders <= 0) EndRound(EBSRoundEndReason::AllHidersCaught);
}

TArray<FBSRoundScoreEntry> ABSGameMode_Blackout::ResolveScoring(EBSRoundEndReason Reason)
{
	TArray<FBSRoundScoreEntry> Out;
	TArray<ABSPlayerState*> Players; GatherPlayers(Players);

	const bool bAllCaught = (Reason == EBSRoundEndReason::AllHidersCaught);
	const int32 Survivors = RemainingHiders;
	const int32 Share = bAllCaught ? 0 : UBSScoringLibrary::BlackoutSurvivorShare(Scoring, TotalHidersThisRound, Survivors);

	for (ABSPlayerState* PS : Players)
	{
		FBSRoundScoreEntry E;
		E.PlayerName = PS->GetPlayerName();
		E.Role = PS->GetBSRole();
		if (PS->GetBSRole() == EBSRole::Hunter)
		{
			E.PointsAwarded = UBSScoringLibrary::BlackoutHunterPoints(Scoring, TotalHidersThisRound, PS->GetCatchesThisRound(), bAllCaught);
		}
		else if (PS->GetBSRole() == EBSRole::Hider)
		{
			E.bSurvived = !PS->IsEliminated();
			E.PointsAwarded = E.bSurvived ? Share : 0;
		}
		if (Reason != EBSRoundEndReason::Aborted) PS->AddPoints(E.PointsAwarded);
		Out.Add(E);
	}
	return Out;
}
