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
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"

ABSHunterCharacter::ABSHunterCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	VisionPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("VisionPostProcess"));
	VisionPostProcess->SetupAttachment(GetRootComponent());
	VisionPostProcess->bUnbound = true;
	VisionPostProcess->bEnabled = false;	// enabled only for the local Hunter in BeginPlay
	
	
	
		//ThirdPersonCameraBoom
    	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    	CameraBoom->SetupAttachment(RootComponent);
    	CameraBoom->TargetArmLength = 175.f;
    	CameraBoom->SocketOffset = FVector(0.0f, 60.0f, 30.0f); //Raise camera above player
    	CameraBoom->bUsePawnControlRotation = true;
    	
    	Camera->SetupAttachment(CameraBoom,USpringArmComponent::SocketName);
    	Camera->SetRelativeLocation(FVector::ZeroVector);
    	Camera->bUsePawnControlRotation = false;
    	
    	// character turns toward movement input not the mouse look input
    	bUseControllerRotationYaw = true;
    	GetCharacterMovement()->bOrientRotationToMovement = false;
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
	//Needed to connect PP_HunterVision to the players on Vision Post Process JRB 10.12.26
	VisionPostProcess->bEnabled = true;
}

void ABSHunterCharacter::OnAbilitySystemReady()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || AmmoDelegateHandle.IsValid()) return;
	AmmoDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UBSAttributeSet::GetAmmoAttribute())
		.AddWeakLambda(this,[this](const FOnAttributeChangeData& Data) { OnAmmoChanged.Broadcast(Data.NewValue); });
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

//Helps with Removing Vision Blendable for the Hunters Post Processing when a Sound is generated JRB 10.12.26
void ABSHunterCharacter::RemoveVisionBlendable(UObject* Blendable)
{
	if (!VisionPostProcess || !Blendable) return;
	VisionPostProcess->Settings.WeightedBlendables.Array.RemoveAll([Blendable](const FWeightedBlendable& W) {return W.Object == Blendable;});
}
