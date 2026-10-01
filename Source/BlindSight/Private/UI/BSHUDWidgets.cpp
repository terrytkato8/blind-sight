#include "UI/BSHUDWidgets.h"
#include "GameFramework/BSGameState.h"
#include "GameFramework/BSPlayerState.h"
#include "Characters/BSHunterCharacter.h"
#include "Characters/BSHiderCharacter.h"
#include "Abilities/BSAttributeSet.h"

// ---------------- Base ----------------
void UBSHUDWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	if (ABSGameState* GS = GetBSGameState())
	{
		GS->OnPhaseChanged.AddDynamic(this, &UBSHUDWidgetBase::HandlePhase);
		GS->OnRemainingHidersChanged.AddDynamic(this, &UBSHUDWidgetBase::HandleRemaining);
		HandlePhase(GS->GetPhase());
		HandleRemaining(GS->GetRemainingHiders());
		if (GS->GetNextHunterPlayerState()) OnNextHunterChanged(GS->GetNextHunterPlayerState()->GetPlayerName());
	}
}

void UBSHUDWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (const ABSGameState* GS = GetBSGameState())
	{
		if (GS->HasRoundTimer()) OnRoundTimeUpdated(GS->GetRoundTimeRemaining());
	}
}

ABSGameState* UBSHUDWidgetBase::GetBSGameState() const { return GetWorld() ? GetWorld()->GetGameState<ABSGameState>() : nullptr; }
ABSPlayerState* UBSHUDWidgetBase::GetBSPlayerState() const { return GetOwningPlayerState<ABSPlayerState>(); }
void UBSHUDWidgetBase::HandlePhase(EBSRoundPhase Phase) { OnPhaseChanged(Phase); }
void UBSHUDWidgetBase::HandleRemaining(int32 Remaining)
{
	const ABSGameState* GS = GetBSGameState();
	OnRemainingHidersChanged(Remaining, GS ? GS->GetTotalHiders() : 0);
}

// ---------------- Hunter ----------------
void UBSHunterHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UBSHunterHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ABSHunterCharacter* Hunter = GetOwningPlayerPawn<ABSHunterCharacter>();
	if (!Hunter) return;
	if (!bBoundAmmo)
	{
		Hunter->OnAmmoChanged.AddDynamic(this, &UBSHunterHUDWidget::HandleAmmo);
		if (const UBSAttributeSet* A = Hunter->GetAttributeSet()) HandleAmmo(A->GetAmmo());
		bBoundAmmo = true;
	}
	OnPulseCooldownUpdated(Hunter->GetPulseCooldownRemaining(), Hunter->bPulseEnabled);
}

void UBSHunterHUDWidget::HandleAmmo(float NewAmmo)
{
	const ABSHunterCharacter* Hunter = GetOwningPlayerPawn<ABSHunterCharacter>();
	const UBSAttributeSet* A = Hunter ? Hunter->GetAttributeSet() : nullptr;
	OnAmmoChanged(FMath::RoundToInt(NewAmmo), A ? FMath::RoundToInt(A->GetMaxAmmo()) : 0);
}

float UBSHunterHUDWidget::GetBearingToLocation(FVector WorldLocation) const
{
	const APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn) return 0.f;
	const FVector ToTarget = (WorldLocation - Pawn->GetActorLocation()).GetSafeNormal2D();
	const float TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Y, ToTarget.X));
	return FRotator::NormalizeAxis(TargetYaw - Pawn->GetControlRotation().Yaw);
}

// ---------------- Hider ----------------
void UBSHiderHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UBSHiderHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ABSHiderCharacter* Hider = GetOwningPlayerPawn<ABSHiderCharacter>();
	if (!Hider) return;
	if (!bBound)
	{
		Hider->OnNoiseTierChanged.AddDynamic(this, &UBSHiderHUDWidget::HandleNoiseTier);
		Hider->OnThrowablesChangedDelegate.AddDynamic(this, &UBSHiderHUDWidget::HandleThrowables);
		HandleNoiseTier(Hider->GetNoiseTier());
		HandleThrowables();
		bBound = true;
	}
	// Attribute replication can land after the delegate; poll cheaply so the count never sticks.
	HandleThrowables();
	if (Hider->IsEliminated() && !bReportedElimination) { bReportedElimination = true; OnEliminated(); }
}

void UBSHiderHUDWidget::HandleNoiseTier(EBSNoiseTier Tier) { OnNoiseTierChanged(Tier); }
void UBSHiderHUDWidget::HandleThrowables()
{
	const ABSHiderCharacter* Hider = GetOwningPlayerPawn<ABSHiderCharacter>();
	const UBSAttributeSet* A = Hider ? Hider->GetAttributeSet() : nullptr;
	if (A) OnThrowablesChanged(FMath::RoundToInt(A->GetThrowables()), FMath::RoundToInt(A->GetMaxThrowables()));
}
