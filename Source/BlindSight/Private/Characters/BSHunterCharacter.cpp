#include "Characters/BSHunterCharacter.h"
#include "Characters/BSHiderCharacter.h"
#include "Abilities/BSAttributeSet.h"
#include "GameFramework/BSGameState.h"
#include "Core/BSGameplayTags.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Camera/CameraComponent.h"

ABSHunterCharacter::ABSHunterCharacter()
{
	VisionPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("VisionPostProcess"));
	VisionPostProcess->SetupAttachment(GetRootComponent());
	VisionPostProcess->bUnbound = true;
	VisionPostProcess->bEnabled = false;	// enabled only for the local Hunter in BeginPlay
	
	GetMesh()->SetOwnerNoSee(true); //Hunter won't see their own body
	Camera->bUsePawnControlRotation = true;
	bUseControllerRotationYaw = true;
}

void ABSHunterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABSHunterCharacter, PulseReadyServerTime);
	DOREPLIFETIME(ABSHunterCharacter, bPulseEnabled);
}

void ABSHunterCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABSHunterCharacter::ApplyVisionSettings()
{
	// Baseline impairment without any material: crushed saturation + heavy vignette + near DOF.
	// The real "short sight radius" look should come from a PP material (see README).
	FPostProcessSettings& S = VisionPostProcess->Settings;
	S.bOverride_VignetteIntensity = true;      S.VignetteIntensity = VignetteIntensity;
	S.bOverride_ColorSaturation = true;        S.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.f);
	S.bOverride_DepthOfFieldFstop = true;      S.DepthOfFieldFstop = 0.6f;
	S.bOverride_DepthOfFieldFocalDistance = true; S.DepthOfFieldFocalDistance = SightRadius * 0.5f;
	S.bOverride_DepthOfFieldSensorWidth = true; S.DepthOfFieldSensorWidth = 60.f;
	S.bOverride_AutoExposureMinBrightness = true; S.AutoExposureMinBrightness = 0.03f;
	S.bOverride_AutoExposureMaxBrightness = true; S.AutoExposureMaxBrightness = 0.03f;
	VisionPostProcess->bEnabled = true;
}

void ABSHunterCharacter::OnAbilitySystemReady()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || AmmoDelegateHandle.IsValid()) return;
	AmmoDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UBSAttributeSet::GetAmmoAttribute())
		.AddLambda([this](const FOnAttributeChangeData& Data) { OnAmmoChanged.Broadcast(Data.NewValue); });
}

void ABSHunterCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	
	if (IsLocallyControlled()) ApplyVisionSettings();
}

void ABSHunterCharacter::Input_Primary()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(BSTags::Ability_Fire));
}

void ABSHunterCharacter::Input_Secondary()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(BSTags::Ability_Pulse));
}

float ABSHunterCharacter::GetPulseCooldownRemaining() const
{
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const float Now = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	return FMath::Max(0.f, PulseReadyServerTime - Now);
}

void ABSHunterCharacter::StartPulseCooldown(float Seconds)
{
	if (!HasAuthority()) return;
	const AGameStateBase* GS = GetWorld()->GetGameState();
	PulseReadyServerTime = (GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds()) + Seconds;
}

void ABSHunterCharacter::ClientPulseReveal_Implementation(float Radius, float Duration)
{
	ClearPulseReveal();
	const float R2 = Radius * Radius;
	for (TActorIterator<ABSHiderCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsEliminated()) continue;
		if (FVector::DistSquared(It->GetActorLocation(), GetActorLocation()) > R2) continue;
		if (USkeletalMeshComponent* HiderMesh = It->GetMesh())
		{
			HiderMesh->SetRenderCustomDepth(true);
			HiderMesh->SetCustomDepthStencilValue(1);
			RevealedActors.Add(*It);
		}
	}
	BP_OnPulse(Radius, Duration);
	GetWorldTimerManager().SetTimer(RevealTimer, this, &ABSHunterCharacter::ClearPulseReveal, Duration, false);
}

void ABSHunterCharacter::ClearPulseReveal()
{
	for (const TWeakObjectPtr<AActor>& A : RevealedActors)
	{
		if (const ABSHiderCharacter* H = Cast<ABSHiderCharacter>(A.Get()))
		{
			if (USkeletalMeshComponent* HiderMesh = H->GetMesh()) HiderMesh->SetRenderCustomDepth(false);
		}
	}
	RevealedActors.Reset();
}

void ABSHunterCharacter::MulticastOnFired_Implementation(FVector ImpactPoint)
{
	BP_OnFired(ImpactPoint);
}
