#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Backend/BSBackendTypes.h"
#include "BSServerBackend.generated.h"

/**
 * SERVER-SIDE backend access. Only ever runs on a dedicated server (or listen-server host
 * during development). Holds the server API key, which must never ship in a client build.
 *
 * This is the only thing in the game allowed to write progression. The division matters:
 * a client that can post its own match results can mint currency, so the client API has
 * no such endpoint and the backend rejects player tokens on every /server/ route.
 *
 * Responsibilities:
 *   - register this container with the backend and heartbeat so it can be allocated
 *   - validate that a connecting player actually belongs to this match
 *   - post round results (progression, currency, stats)
 *   - batch and flush telemetry
 */
UCLASS()
class BLINDSIGHT_API UBSServerBackend : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Tell the backend this server process is alive and ready to host. */
	void RegisterServer();

	/** Mark this server as hosting a match, so matchmaking stops allocating it. */
	void SetMatchState(const FString& InMatchId, const FString& State, int32 PlayerCount);

	/** Returns true if the backend confirms this player was assigned to this match. */
	void ValidatePlayer(const FString& PlayerId, const FString& MatchId, TFunction<void(bool)> OnDone);

	/** Post one finished round. The backend applies currency, XP and stats transactionally. */
	void SubmitRoundResults(const FString& MatchId, int32 RoundNumber, const FString& ModeName,
	                        const TArray<FBSMatchResultEntry>& Entries);

	/** Queue a telemetry event. Flushed on a timer and at round end. */
	void RecordEvent(const FBSTelemetryEvent& Event);
	void FlushTelemetry();

	UFUNCTION(BlueprintPure, Category = "Blind Sight|Backend")
	bool IsBackendConfigured() const { return !BaseUrl.IsEmpty() && !ServerKey.IsEmpty(); }

	const FString& GetServerId() const { return ServerId; }

private:
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> MakeRequest(const FString& Verb, const FString& Path) const;
	void Heartbeat();

	FString BaseUrl;
	FString ServerKey;
	FString ServerId;
	FString Region;
	FString AdvertisedAddress;

	TArray<FBSTelemetryEvent> PendingEvents;
	FTimerHandle HeartbeatTimer;
	FTimerHandle FlushTimer;

	static constexpr int32 MaxBufferedEvents = 500;
};
