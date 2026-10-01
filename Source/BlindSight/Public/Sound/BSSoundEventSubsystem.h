#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/BSTypes.h"
#include "BSSoundEventSubsystem.generated.h"

class UBSSoundProfile;
class ABSPlayerController;

DECLARE_MULTICAST_DELEGATE_OneParam(FBSOnSoundEmitted, const FBSSoundEvent&);

/**
 * Server-authoritative sound event bus (GDD §6.1, §12 replication notes).
 *
 * Any gameplay code calls EmitSound(). On the server we:
 *   1. look up radius/intensity from the UBSSoundProfile,
 *   2. for every Hunter, compute distance falloff + geometry occlusion,
 *   3. send a Client RPC (FBSSoundPing) ONLY to Hunters who would perceive it,
 *   4. multicast the actual audio playback to all clients via GameState.
 *
 * Hider clients never receive ping data, so local-only audio cues can't be exploited.
 */
UCLASS()
class BLINDSIGHT_API UBSSoundEventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Emit a profiled event. Server-only; silently ignored on clients. */
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Sound", meta = (WorldContext = "WorldContextObject"))
	static void EmitSound(UObject* WorldContextObject, EBSSoundEventType Type, FVector Origin, AActor* Instigator = nullptr, float IntensityScale = 1.f);

	/** Emit a fully custom event (radius/intensity specified by caller). Server-only. */
	void EmitSoundEvent(const FBSSoundEvent& Event);

	void SetSoundProfile(UBSSoundProfile* InProfile);
	UBSSoundProfile* GetSoundProfile() const { return SoundProfile; }

	/** Fired on the server for every emitted event (analytics, AI, hazards). */
	FBSOnSoundEmitted OnSoundEmitted;

private:
	float ComputePerceivedIntensity(const FBSSoundEvent& Event, const FVector& ListenerLocation, bool& bOutOccluded) const;
	int32 CountBlockingSurfaces(const FVector& From, const FVector& To, AActor* IgnoreA, AActor* IgnoreB) const;

	UPROPERTY() TObjectPtr<UBSSoundProfile> SoundProfile;
};
