#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Backend/BSBackendTypes.h"
#include "BSBackendSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnLoginComplete, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnProfileUpdated, const FBSPlayerProfile&, Profile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnTicketStateChanged, EBSTicketState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBSOnMatchFound, const FBSMatchAssignment&, Assignment);

/**
 * CLIENT-SIDE backend access (GameInstance subsystem, so it survives level travel).
 *
 * Security model, which is the whole point of this class:
 *   - The client can only READ its own profile and QUEUE for a match.
 *   - The client can never write points, currency, unlocks or match results.
 *     Those endpoints reject a player token outright; only a dedicated server
 *     holding the server key can call them (see UBSServerBackend).
 *   - The player token is obtained by exchanging a platform identity token
 *     (EOS / Steam) at /auth/login, so a player cannot claim another player's id.
 *
 * Everything here is async. Nothing blocks the game thread.
 */
UCLASS()
class BLINDSIGHT_API UBSBackendSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Exchange a platform identity token for a backend session token. Call once at boot. */
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Backend")
	void Login(const FString& PlatformIdToken, const FString& PlatformType = TEXT("eos"));

	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Backend")
	void FetchProfile();

	/** Cosmetics the player already owns. Rejected server-side if they do not own it. */
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Backend")
	void EquipCosmetic(const FString& Slot, const FString& CosmeticId);

	/** Spend currency on an unlock. The backend re-checks price and balance; never trust the client's maths. */
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Backend")
	void PurchaseCosmetic(const FString& CosmeticId);

	// ---- Matchmaking ----
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Matchmaking")
	void EnterQueue(const FString& ModeName, const FString& Region, const FString& PartyId = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Matchmaking")
	void LeaveQueue();

	/** Travels this client to the allocated dedicated server. */
	UFUNCTION(BlueprintCallable, Category = "Blind Sight|Matchmaking")
	void JoinAssignedMatch();

	UFUNCTION(BlueprintPure, Category = "Blind Sight|Backend")
	const FBSPlayerProfile& GetProfile() const { return CachedProfile; }

	UFUNCTION(BlueprintPure, Category = "Blind Sight|Backend")
	bool IsLoggedIn() const { return !PlayerToken.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "Blind Sight|Matchmaking")
	EBSTicketState GetTicketState() const { return TicketState; }

	UPROPERTY(BlueprintAssignable) FBSOnLoginComplete OnLoginComplete;
	UPROPERTY(BlueprintAssignable) FBSOnProfileUpdated OnProfileUpdated;
	UPROPERTY(BlueprintAssignable) FBSOnTicketStateChanged OnTicketStateChanged;
	UPROPERTY(BlueprintAssignable) FBSOnMatchFound OnMatchFound;

private:
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> MakeRequest(const FString& Verb, const FString& Path, bool bAuthed = true) const;
	void PollTicket();
	void SetTicketState(EBSTicketState NewState);

	FString BaseUrl;
	FString PlayerToken;
	FString TicketId;
	EBSTicketState TicketState = EBSTicketState::None;
	FBSPlayerProfile CachedProfile;
	FBSMatchAssignment PendingAssignment;
	FTimerHandle PollTimer;
};
