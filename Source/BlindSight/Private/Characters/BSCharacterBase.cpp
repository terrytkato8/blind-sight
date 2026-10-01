#include "Characters/BSCharacterBase.h"
#include "Abilities/BSAbilitySystemComponent.h"
#include "Abilities/BSAttributeSet.h"
#include "Sound/BSSoundEmitterComponent.h"
#include "GameFramework/BSPlayerState.h"
#include "GameFramework/BSPlayerController.h"
#include "Core/BSGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"

ABSCharacterBase::ABSCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;

	SoundEmitter = CreateDefaultSubobject<UBSSoundEmitterComponent>(TEXT("SoundEmitter"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeed = 420.f;
	GetCharacterMovement()->MaxWalkSpeedCrouched = 200.f;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

UAbilitySystemComponent* ABSCharacterBase::GetAbilitySystemComponent() const
{
	const ABSPlayerState* PS = GetPlayerState<ABSPlayerState>();
	return PS ? PS->GetAbilitySystemComponent() : nullptr;
}

UBSAttributeSet* ABSCharacterBase::GetAttributeSet() const
{
	const ABSPlayerState* PS = GetPlayerState<ABSPlayerState>();
	return PS ? PS->GetAttributeSet() : nullptr;
}

void ABSCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABSCharacterBase, bEliminated);
	DOREPLIFETIME(ABSCharacterBase, bHeadStartFrozen);
}

EBSRole ABSCharacterBase::GetBSRole() const
{
	const ABSPlayerState* PS = GetPlayerState<ABSPlayerState>();
	return PS ? PS->GetBSRole() : EBSRole::None;
}

void ABSCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilityActorInfo();
	GrantStartupAbilities();
	OnAbilitySystemReady();
}

void ABSCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilityActorInfo();
	OnAbilitySystemReady();
}

void ABSCharacterBase::InitAbilityActorInfo()
{
	if (ABSPlayerState* PS = GetPlayerState<ABSPlayerState>())
	{
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
	}
}

void ABSCharacterBase::GrantStartupAbilities()
{
	if (!HasAuthority() || bAbilitiesGranted) return;
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	// Clear abilities from a previous role (rotation), then grant this role's set.
	ASC->ClearAllAbilities();
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : StartupAbilities)
	{
		if (AbilityClass) ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
	}
	bAbilitiesGranted = true;
}

void ABSCharacterBase::SetHeadStartFrozen(bool bFrozen)
{
	if (!HasAuthority()) return;
	bHeadStartFrozen = bFrozen;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (bFrozen) ASC->AddLooseGameplayTag(BSTags::State_HeadStartFrozen);
		else         ASC->RemoveLooseGameplayTag(BSTags::State_HeadStartFrozen);
	}
	GetCharacterMovement()->SetMovementMode(bFrozen ? MOVE_None : MOVE_Walking);
}

void ABSCharacterBase::OnRep_Eliminated()
{
	if (bEliminated)
	{
		GetCharacterMovement()->DisableMovement();
	}
}

void ABSCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext) Sub->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC) return;

	if (IA_Move)      EIC->BindAction(IA_Move,      ETriggerEvent::Triggered, this, &ABSCharacterBase::Input_Move);
	if (IA_Look)      EIC->BindAction(IA_Look,      ETriggerEvent::Triggered, this, &ABSCharacterBase::Input_Look);
	if (IA_Primary)   EIC->BindAction(IA_Primary,   ETriggerEvent::Started,   this, &ABSCharacterBase::Input_Primary);
	if (IA_Secondary) EIC->BindAction(IA_Secondary, ETriggerEvent::Started,   this, &ABSCharacterBase::Input_Secondary);
	if (IA_Sprint)
	{
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started,   this, &ABSCharacterBase::Input_SprintStart);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ABSCharacterBase::Input_SprintStop);
	}
	if (IA_Crouch)    EIC->BindAction(IA_Crouch,    ETriggerEvent::Started,   this, &ABSCharacterBase::Input_CrouchToggle);
}

void ABSCharacterBase::Input_Move(const FInputActionValue& Value)
{
	if (bEliminated || bHeadStartFrozen) return;
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), Axis.X);
}

void ABSCharacterBase::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}
