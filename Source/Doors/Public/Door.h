// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "DoorTypes.h"

#include "Door.generated.h"

/**
 * Net-Predicted Doors for interaction (interacting)
 * With replication for non-interaction (observed)
 */
UCLASS()
class DOORS_API ADoor : public AActor
{
	GENERATED_BODY()
	
protected:
	// Door State

	/**
	 * The current state of the door
	 * You can change the default state of the door
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Door)
	EDoorState DoorState = EDoorState::Closed;

	/**
	 * The last side the door was interacted from
	 * You can change the default side of the door
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Door)
	EDoorSide DoorSide = EDoorSide::Front;
	
	UPROPERTY(ReplicatedUsing=OnRep_DoorState)
	EReplicatedDoorState RepDoorState = EReplicatedDoorState::ClosedFront;

public:
	ADoor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	void OnRep_DoorState();

public:
	UFUNCTION(BlueprintPure, Category=Door)
	EDoorState GetDoorState() const { return DoorState; }
	
	UFUNCTION(BlueprintPure, Category=Door)
	EDoorSide GetDoorSide() const { return DoorSide; }

	void SetDoorState(EDoorState NewDoorState, EDoorSide NewDoorSide);

	void OnDoorStateChanged(EDoorState OldDoorState, EDoorState NewDoorState, EDoorSide OldDoorSide, EDoorSide NewDoorSide);

	/**
	 * Called from ShouldDoorAbilityRespondToEvent() as an early extension point to override before any other checks occur
	 * @return false if you want to prevent the door from changing state
	 */
	UFUNCTION(BlueprintNativeEvent, Category=Door)
	bool CanDoorChangeToAnyState(const AActor* Avatar) const;
	bool CanDoorChangeToAnyState_Implementation(const AActor* Avatar) const	{ return true; }
	
	/**
	 * Called from ShouldDoorAbilityRespondToEvent() as an extension point to override if the change in door state would otherwise succeed
	 * @return false if you want to prevent an otherwise successful change in door state
	 */
	UFUNCTION(BlueprintNativeEvent, Category=Door)
	bool CanChangeDoorState(const AActor* Avatar, EDoorState CurrentDoorState, EDoorState NewDoorState, EDoorSide OldDoorSide, EDoorSide NewDoorSide) const;
	bool CanChangeDoorState_Implementation(const AActor* Avatar, EDoorState CurrentDoorState, EDoorState NewDoorState, EDoorSide OldDoorSide, EDoorSide NewDoorSide) const
	{
		return true;
	}

	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door State Changed"))
	void K2_OnDoorStateChanged(EDoorState OldDoorState, EDoorState NewDoorState, EDoorSide OldDoorSide, EDoorSide NewDoorSide);

protected:
	// Door State Events
	
	virtual void OnDoorFinishedOpening() { K2_OnDoorFinishedOpening(); }
	virtual void OnDoorFinishedClosing() { K2_OnDoorFinishedClosing(); }
	virtual void OnDoorStartedOpening() { K2_OnDoorStartedOpening(); }
	virtual void OnDoorStartedClosing() { K2_OnDoorStartedClosing(); }
	virtual void OnDoorInMotionInterrupted(EDoorState OldDoorState, EDoorState NewDoorState, EDoorSide OldDoorSide, EDoorSide NewDoorSide)
	{
		K2_OnDoorInMotionInterrupted(OldDoorState, NewDoorState, OldDoorSide, NewDoorSide);
	}

	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Finished Closing"))
	void K2_OnDoorFinishedClosing();
	
	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Finished Opening"))
	void K2_OnDoorFinishedOpening();

	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Started Closing"))
	void K2_OnDoorStartedClosing();
	
	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Started Opening"))
	void K2_OnDoorStartedOpening();

	/** Called when the door is in motion, and was interacted with, causing it to change motion */
	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door In Motion Interrupted"))
	void K2_OnDoorInMotionInterrupted(EDoorState OldDoorState, EDoorState NewDoorState, EDoorSide OldDoorSide, EDoorSide NewDoorSide);
	
protected:
	// Door Access

	/** Which side(s) of the door can we interact from */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Door)
	EDoorAccess DoorAccess = EDoorAccess::Bidirectional;

	/** What to do if changing door access based on the door state */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Door)
	EDoorChangeType DoorAccessChangeType = EDoorChangeType::Wait;

	/** Used if change type is set to Wait */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=Door)
	EDoorAccess PendingDoorAccess = DoorAccess;

	/** Used if change type is set to Wait */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=Door)
	bool bHasPendingDoorAccess = false;
	
	/** Which ways can the door open */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Door)
	EDoorOpenDirection DoorOpenDirection = EDoorOpenDirection::Bidirectional;

	/** What to do if changing door open mode based on the door state */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Door)
	EDoorChangeType DoorOpenDirectionChangeType = EDoorChangeType::Wait;

	/** Used if change type is set to Wait */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=Door)
	EDoorOpenDirection PendingDoorOpenDirection = DoorOpenDirection;

	/** Used if change type is set to Wait */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=Door)
	bool bHasPendingDoorOpenDirection = false;
	
	/**
	 * Which action we prefer to use when opening the door
	 * We might not always use the preferred motion, e.g. we would only push a door open if we're behind it and it opens outwards
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Door)
	EDoorMotion DoorOpenMotion = EDoorMotion::Push;

	/** What to do if changing door access based on the door state */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Door)
	EDoorChangeType DoorOpenMotionChangeType = EDoorChangeType::Immediate;

	/** Used if change type is set to Wait */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=Door)
	EDoorMotion PendingDoorOpenMotion = DoorOpenMotion;

	/** Used if change type is set to Wait */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=Door)
	bool bHasPendingDoorOpenMotion = false;
	
public:
	UFUNCTION(BlueprintPure, Category=Door)
	EDoorAccess GetDoorAccess() const { return DoorAccess; }

	UFUNCTION(BlueprintCallable, Category=Door)
	void SetDoorAccess(EDoorAccess NewDoorAccess);

	UFUNCTION(BlueprintNativeEvent, Category=Door)
	bool CanChangeDoorAccess(EDoorAccess NewDoorAccess) const;

	void OnDoorAccessChanged(EDoorAccess OldDoorAccess, EDoorAccess NewDoorAccess);

	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Access Changed"))
	void K2_OnDoorAccessChanged(EDoorAccess OldDoorAccess, EDoorAccess NewDoorAccess);
	
public:
	UFUNCTION(BlueprintPure, Category=Door)
	EDoorOpenDirection GetDoorOpenDirection() const { return DoorOpenDirection; }

	UFUNCTION(BlueprintCallable, Category=Door)
	void SetDoorOpenDirection(EDoorOpenDirection NewDoorOpenDirection);

	UFUNCTION(BlueprintNativeEvent, Category=Door)
	bool CanChangeDoorOpenDirection(EDoorOpenDirection NewDoorOpenDirection) const;

	void OnDoorOpenDirectionChanged(EDoorOpenDirection OldDoorOpenDirection, EDoorOpenDirection NewDoorOpenDirection);

	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Open Mode Changed"))
	void K2_OnDoorOpenDirectionChanged(EDoorOpenDirection OldDoorOpenDirection, EDoorOpenDirection NewDoorOpenDirection);
	
public:
	UFUNCTION(BlueprintPure, Category=Door)
	EDoorMotion GetDoorOpenMotion() const { return DoorOpenMotion; }

	UFUNCTION(BlueprintCallable, Category=Door)
	void SetDoorOpenMotion(EDoorMotion NewDoorOpenMotion);

	UFUNCTION(BlueprintNativeEvent, Category=Door)
	bool CanChangeDoorOpenMotion(EDoorMotion NewDoorOpenMotion) const;

	void OnDoorOpenMotionChanged(EDoorMotion OldDoorOpenMotion, EDoorMotion NewDoorOpenMotion);

	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Open  Motion Changed"))
	void K2_OnDoorOpenMotionChanged(EDoorMotion OldDoorOpenMotion, EDoorMotion NewDoorOpenMotion);

protected:
	/** If false, interaction will be rejected if the client's door side differs from the server's */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Door)
	bool bTrustClientDoorSide = true;

	/** If true, the door can be interacted with while in motion, otherwise they must wait for it to finish opening or closing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Door)
	bool bCanInteractWhileInMotion = true;

public:
	UFUNCTION(BlueprintPure, Category=Door)
	bool CanInteractWhileInMotion() const { return bCanInteractWhileInMotion; }

public:
	/** How long to wait before interaction since the door was last interacted with */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Door)
	float InteractCooldown = 0.2f;

	/** How long to wait before interaction since the door entered a stationary state after being in motion */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Door)
	float StationaryCooldown = 0.3f;
	
protected:
	/** Last time the door was interacted with successfully */
	UPROPERTY(BlueprintReadOnly, Category=Interaction)
	float LastInteractTime = -1.f;

	/** Last time the door entered a stationary state after being in motion */
	UPROPERTY(BlueprintReadOnly, Category=Interaction)
	float LastStationaryTime = -1.f;
	
	/** Represents the value in -1 to 1 range by which the door is open or closed, -1 and 1 are fully open inward / outward and 0 is fully closed */
	UPROPERTY(VisibleAnywhere, Category=Door, meta=(ClampMin="-1", UIMin="-1", ClampMax="1", UIMax="1", ForceUnits="Percent"))
	float DoorAlpha = 0.f;

public:
	/** Call from your gameplay ability after door interaction succeeds to start the interact cooldown */
	UFUNCTION(BlueprintCallable, Category=Door)
	void StartInteractCooldown();
	
	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorOnCooldown() const
	{
		return IsDoorOnInteractCooldown() || IsDoorOnStationaryCooldown();
	}

	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorOnInteractCooldown() const;

	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorOnStationaryCooldown() const;

public:
	UFUNCTION(BlueprintCallable, Category=Door)
	virtual bool ShouldAbilityRespondToDoorEvent(const AActor* Avatar, EDoorState ClientDoorState, EDoorSide ClientDoorSide,
		EDoorSide CurrentDoorSide, EDoorState& NewDoorState, EDoorSide& NewDoorSide, EDoorMotion& DoorMotion) const;
	
public:
	// Door State Helpers

	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorInMotion() const
	{
		return IsDoorStateInMotion(DoorState);
	}

	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorStationary() const
	{
		return IsDoorStateStationary(DoorState);
	}

	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorOpenOrOpening() const
	{
		return IsDoorStateOpenOrOpening(DoorState);
	}

	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorClosedOrClosing() const
	{
		return IsDoorStateClosedOrClosing(DoorState);
	}

	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorStateInMotion(EDoorState State) const
	{
		return State == EDoorState::Opening || State == EDoorState::Closing;
	}
	
	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorStateStationary(EDoorState State) const
	{
		return !IsDoorStateInMotion(State);
	}

	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorStateOpenOrOpening(EDoorState State) const
	{
		return State == EDoorState::Open || State == EDoorState::Opening;
	}

	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorStateClosedOrClosing(EDoorState State) const
	{
		return  State == EDoorState::Closed || State == EDoorState::Closing;
	}

public:
#if WITH_EDITOR
	virtual void HandleDoorPropertyChange();
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
