// Copyright 2026 Kato.8 Studios LLC. All Rights Reserved.


#include "Characters/BSCharacterMovementComponent.h"
#include "GameFramework/Character.h"

// Each saved move remembers whether sprint was held, so the server and the client's replays use the same speed. JRB 10.20.26
class FSavedMove_BS : public FSavedMove_Character
{
	public:
		uint8 bSavedWantsToSprint : 1;
	
		virtual void Clear() override
		{
			FSavedMove_Character::Clear();
			bSavedWantsToSprint = 0;
		}
	
		virtual uint8 GetCompressedFlags() const override
		{
			uint8 Result = FSavedMove_Character::GetCompressedFlags();
			if (bSavedWantsToSprint) Result |= FLAG_Custom_0;
			return Result;
		}
		
		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override
		{
			if (bSavedWantsToSprint != static_cast<FSavedMove_BS*>(NewMove.Get())->bSavedWantsToSprint) return false;
			return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
		}
	
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, 
			class FNetworkPredictionData_Client_Character& ClientData) override
		{
			FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
			if (const UBSCharacterMovementComponent* Move = Cast<UBSCharacterMovementComponent>(C->GetCharacterMovement()))
				bSavedWantsToSprint = Move->bWantsToSprint;
		}
	
		virtual void PrepMoveFor(ACharacter* C) override
		{
			FSavedMove_Character::PrepMoveFor(C);
			if (UBSCharacterMovementComponent* Move = Cast<UBSCharacterMovementComponent>(C->GetCharacterMovement()))
				Move->bWantsToSprint = bSavedWantsToSprint;
		}
};

class FNetworkPredictionData_Client_BS : public FNetworkPredictionData_Client_Character
{
	public:
		explicit FNetworkPredictionData_Client_BS(const UCharacterMovementComponent& ClientMovement)
			: FNetworkPredictionData_Client_Character(ClientMovement) {}
	
	virtual FSavedMovePtr AllocateNewMove() override {return FSavedMovePtr(new FSavedMove_BS()); }
};


		bool UBSCharacterMovementComponent::IsSprinting() const
		{
			return bWantsToSprint && !IsCrouching() && IsMovingOnGround();
		}

		float UBSCharacterMovementComponent::GetMaxSpeed() const
		{
			return IsSprinting() ? SprintSpeed : Super::GetMaxSpeed();
		}

		void UBSCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
		{
			Super::UpdateFromCompressedFlags(Flags);
			bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
		}

		FNetworkPredictionData_Client* UBSCharacterMovementComponent::GetPredictionData_Client() const
		{
			if (!ClientPredictionData)
			{
				UBSCharacterMovementComponent* MutableThis = const_cast<UBSCharacterMovementComponent*>(this);
				MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_BS(*this);
			}
			return ClientPredictionData;
		}
