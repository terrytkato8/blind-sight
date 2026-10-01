#include "Sound/BSSoundProfile.h"
#include "Core/BSGameplayTags.h"

static FBSSoundEventDefinition MakeDef(float Radius, float Intensity, float Lifetime, const FGameplayTag& Tag)
{
	FBSSoundEventDefinition D;
	D.Radius = Radius; D.Intensity = Intensity; D.PingLifetime = Lifetime; D.Tag = Tag;
	return D;
}

UBSSoundProfile::UBSSoundProfile()
{
	// GDD §6.1 baseline. Units are cm. Tune in the Data Asset, not here.
	Events.Add(EBSSoundEventType::FootstepCrouch, MakeDef( 150.f, 0.10f, 0.8f, BSTags::Sound_Footstep_Crouch));
	Events.Add(EBSSoundEventType::FootstepWalk,   MakeDef( 700.f, 0.35f, 1.0f, BSTags::Sound_Footstep_Walk));
	Events.Add(EBSSoundEventType::FootstepSprint, MakeDef(2000.f, 0.80f, 1.4f, BSTags::Sound_Footstep_Sprint));
	Events.Add(EBSSoundEventType::ItemPickup,     MakeDef( 500.f, 0.30f, 1.0f, BSTags::Sound_Pickup));
	Events.Add(EBSSoundEventType::ThrowRelease,   MakeDef( 900.f, 0.40f, 1.0f, BSTags::Sound_Throw_Release));
	Events.Add(EBSSoundEventType::ThrowImpact,    MakeDef(2500.f, 0.90f, 2.0f, BSTags::Sound_Throw_Impact));
	Events.Add(EBSSoundEventType::Door,           MakeDef(1400.f, 0.55f, 1.5f, BSTags::Sound_Door));
	Events.Add(EBSSoundEventType::Gunshot,        MakeDef(9000.f, 1.00f, 2.5f, BSTags::Sound_Gunshot));
	Events.Add(EBSSoundEventType::HunterPulse,    MakeDef(2500.f, 0.60f, 1.5f, BSTags::Sound_Pulse));
	Events.Add(EBSSoundEventType::Custom,         MakeDef(1000.f, 0.50f, 1.5f, FGameplayTag()));
}

const FBSSoundEventDefinition& UBSSoundProfile::GetDefinition(EBSSoundEventType Type) const
{
	if (const FBSSoundEventDefinition* Found = Events.Find(Type))
	{
		return *Found;
	}
	static const FBSSoundEventDefinition Fallback;
	return Fallback;
}
