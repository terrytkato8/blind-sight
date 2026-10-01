#include "GameFramework/BSGameMode_Manhunt.h"
#include "GameFramework/BSPlayerState.h"

ABSGameMode_Manhunt::ABSGameMode_Manhunt()
{
	ModeDisplayName = NSLOCTEXT("BlindSight", "ModeManhunt", "Manhunt");
}

void ABSGameMode_Manhunt::CheckRoundEndAfterElimination()
{
	if (RemainingHiders <= 1) EndRound(EBSRoundEndReason::LastHiderStanding);
}

TArray<FBSRoundScoreEntry> ABSGameMode_Manhunt::ResolveScoring(EBSRoundEndReason Reason)
{
	TArray<FBSRoundScoreEntry> Out;
	TArray<ABSPlayerState*> Players; GatherPlayers(Players);

	for (ABSPlayerState* PS : Players)
	{
		FBSRoundScoreEntry E;
		E.PlayerName = PS->GetPlayerName();
		E.Role = PS->GetBSRole();
		if (PS->GetBSRole() == EBSRole::Hunter)
		{
			E.PointsAwarded = UBSScoringLibrary::ManhuntHunterPoints(Scoring, PS->GetCatchesThisRound());
		}
		else if (PS->GetBSRole() == EBSRole::Hider)
		{
			E.bSurvived = !PS->IsEliminated();
			if (E.bSurvived) { PS->SetPlacement(1); E.bIsSurvivorCallout = true; }
			E.Placement = PS->GetPlacement();
			// Timer-capped or aborted rounds with several survivors: all remaining tie for 1st? Keep it simple — survivors get 2nd-tier.
			const int32 Place = (E.bSurvived && RemainingHiders > 1) ? 2 : E.Placement;
			E.PointsAwarded = UBSScoringLibrary::ManhuntPlacementPoints(Scoring, TotalHidersThisRound, Place);
			E.bIsSurvivorCallout = E.bSurvived && RemainingHiders == 1;
		}
		if (Reason != EBSRoundEndReason::Aborted) PS->AddPoints(E.PointsAwarded);
		Out.Add(E);
	}
	return Out;
}
