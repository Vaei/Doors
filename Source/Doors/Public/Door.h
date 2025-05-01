// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "DoorTypes.h"
#include "GraspableOwner.h"

#include "Door.generated.h"

class UDoorSpriteWidgetComponent;
class UDoorEditorVisualizer;

/**
 * Net-Predicted Doors for interaction (interacting)
 * With replication for non-interaction (observed)
 *
 * Alpha and door state value correlation/range:
 * +1.0 open outward ↤ 0.0 ↦ -1.0 open inward
 *
 * OpenOutward	+1.0 ⇄ Close 0.0
 * OpenInward	-1.0 ⇄ Close 0.0
 */
UCLASS()
class DOORS_API ADoor : public AActor, public IGraspableOwner
{
	GENERATED_BODY()

public:
	/** IGraspableOwner interface */
	virtual TArray<FGameplayAbilityTargetData*> GatherOptionalGraspTargetData(const FGameplayAbilityActorInfo* ActorInfo) const override final;
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Door)
	TObjectPtr<USceneComponent> Root;
	
	/** Ties into FDoorVisualizer to draw editor visuals */
	UPROPERTY()
	TObjectPtr<UDoorEditorVisualizer> DoorVisualizer;

	/** Used to draw debug sprites during PIE in editor */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDoorSpriteWidgetComponent> DoorSprite;

protected:
	// Door State

	/**
	 * The current state of the door
	 * You can change the default state of the door
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Door)
	EDoorState DoorState = EDoorState::Closed;

	/**
	 * The direction the door entered it's current state from
	 * You can change the default direction of the door
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Door)
	EDoorDirection DoorDirection = EDoorDirection::Outward;
	
	UPROPERTY(ReplicatedUsing=OnRep_DoorState)
	EReplicatedDoorState RepDoorState = EReplicatedDoorState::ClosedOutward;

	/** Disabling replication can produce better results for automatic doors */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Door)
	bool bEnableDoorStateReplication = true;

	/** Last avatar that interacted with the door */
	TWeakObjectPtr<AActor> LastAvatar;

public:
	UPROPERTY(BlueprintAssignable, Category=Door)
	FOnDoorStateChanged OnDoorStateChangedDelegate;

public:
	ADoor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintNativeEvent, Category=Door)
	void TickDoor(float DeltaTime);

	UFUNCTION(BlueprintPure, Category=Door)
	float GetTargetDoorAlpha() const;
	
	UFUNCTION(BlueprintPure, Category=Door)
	float GetTargetDoorAlphaFromState(EDoorState State, EDoorDirection Direction) const;
	
#if WITH_EDITORONLY_DATA
	void OnToggleShowDoorStateDuringPIE(IConsoleVariable* CVar);
#endif
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	void OnRep_DoorState();

	/**
	 * Set the door state replication enabled or disabled
	 * @param bEnabled If true, the door state will be replicated to clients
	 * @param bReplicateNow If true, the current door state will be replicated to clients immediately
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Door)
	void SetDoorStateReplicationEnabled(bool bEnabled, bool bReplicateNow = true);

	/**
	 * Optionally override this to return a location that reflects the door mesh in its current state
	 * to allow GetDoorSide to accurately determine which side of the door the avatar is on, while the door is open
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category=Door)
	FVector GetDoorLocation() const;
	virtual FVector GetDoorLocation_Implementation() const { return GetActorLocation();	}

	UFUNCTION(BlueprintPure, Category=Door)
	FTransform GetDoorTransform() const
	{
		return { GetActorTransform().Rotator(), GetDoorLocation(), GetActorScale3D() };
	}

public:
	/**
	 * Last avatar that interacted with the door. Must be explicitly saved.
	 * Not replicated -- if client activate ability fails will not be sent to the server.
	 */
	UFUNCTION(BlueprintPure, Category=Door)
	AActor* GetLastAvatar() const { return LastAvatar.IsValid() ? LastAvatar.Get() : nullptr; }

	/**
	 * Used to save the last avatar if required
	 * Not replicated -- if client activate ability fails will not be sent to the server.
	 */
	UFUNCTION(BlueprintCallable, Category=Door)
	void SetLastAvatar(AActor* Avatar)
	{
		LastAvatar = Avatar;
	}
	
public:
	UFUNCTION(BlueprintPure, Category=Door)
	EDoorState GetDoorState() const { return DoorState; }
	
	UFUNCTION(BlueprintPure, Category=Door)
	EDoorDirection GetDoorDirection() const { return DoorDirection; }

	UFUNCTION(BlueprintPure, Category=Door)
	EReplicatedDoorState GetRepDoorState() const { return RepDoorState; }

	/**
	 * Call to set the door state
	 * @param NewDoorState The new state of the door
	 * @param NewDoorDirection The new direction of the door
	 * @param Avatar The avatar that is interacting with the door -- not valid from replication
	 * @param bClientSimulation If true, this change occurred from replication and not from predicted interaction
	 */
	UFUNCTION(BlueprintCallable, Category=Door, meta=(HidePin="bClientSimulation"))
	void SetDoorState(EDoorState NewDoorState, EDoorDirection NewDoorDirection, AActor* Avatar, bool bClientSimulation = false);

	/**
	 * Called when the door state changes
	 * @param OldDoorState The previous state of the door
	 * @param NewDoorState The new state of the door
	 * @param OldDoorDirection The previous direction of the door
	 * @param NewDoorDirection The new direction of the door
	 * @param Avatar The avatar that is interacting with the door -- not valid from replication
	 * @param bClientSimulation If true, this change occurred from replication and not from predicted interaction
	 */
	void OnDoorStateChanged(EDoorState OldDoorState, EDoorState NewDoorState, EDoorDirection OldDoorDirection,
		EDoorDirection NewDoorDirection, AActor* Avatar, bool bClientSimulation);

	/**
	 * Called from ShouldDoorAbilityRespondToEvent() as an early extension point to override before any other checks occur
	 * @return false if you want to prevent the door from changing state
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category=Door)
	bool CanDoorChangeToAnyState(const AActor* Avatar) const;
	bool CanDoorChangeToAnyState_Implementation(const AActor* Avatar) const	{ return true; }
	
	/**
	 * Called from ShouldDoorAbilityRespondToEvent() as an extension point to override if the change in door state would otherwise succeed
	 * @return false if you want to prevent an otherwise successful change in door state
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category=Door)
	bool CanChangeDoorState(const AActor* Avatar, EDoorState CurrentDoorState, EDoorState NewDoorState,
		EDoorDirection OldDoorDirection, EDoorDirection NewDoorDirection) const;
	virtual bool CanChangeDoorState_Implementation(const AActor* Avatar, EDoorState CurrentDoorState,
		EDoorState NewDoorState, EDoorDirection OldDoorDirection, EDoorDirection NewDoorDirection) const
	{
		return true;
	}

	/**
	 * Callback for when the door state changes
	 * If bClientSimulation is true, this change occurred from replication and not from predicted interaction
	 * @param OldDoorState The previous state of the door
	 * @param NewDoorState The new state of the door
	 * @param OldDoorDirection The previous direction of the door
	 * @param NewDoorDirection The new direction of the door
	 * @param Avatar The avatar that is interacting with the door -- not valid from replication
	 * @param bClientSimulation If true, this change occurred from replication and not from predicted interaction
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door State Changed"))
	void K2_OnDoorStateChanged(EDoorState OldDoorState, EDoorState NewDoorState, EDoorDirection OldDoorDirection,
		EDoorDirection NewDoorDirection, AActor* Avatar, bool bClientSimulation);

	/**
	 * Same as OnDoorStateChanged but only for cosmetic events, i.e. does not occur on dedicated server
	 * If bClientSimulation is true, this change occurred from replication and not from predicted interaction
	 * @param OldDoorState The previous state of the door
	 * @param NewDoorState The new state of the door
	 * @param OldDoorDirection The previous direction of the door
	 * @param NewDoorDirection The new direction of the door
	 * @param Avatar The avatar that is interacting with the door -- not valid from replication
	 * @param bClientSimulation If true, this change occurred from replication and not from predicted interaction
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category=Door, meta=(DisplayName="On Door State Changed (Cosmetic)"))
	void K2_OnDoorStateChangedCosmetic(EDoorState OldDoorState, EDoorState NewDoorState, EDoorDirection OldDoorDirection,
		EDoorDirection NewDoorDirection, AActor* Avatar, bool bClientSimulation);
	
protected:
	// Door State Events
	
	virtual void OnDoorFinishedOpening(bool bClientSimulation);
	virtual void OnDoorFinishedClosing(bool bClientSimulation);
	virtual void OnDoorStartedOpening(bool bClientSimulation);
	virtual void OnDoorStartedClosing(bool bClientSimulation);

	virtual void OnDoorInMotionInterrupted(EDoorState OldDoorState, EDoorState NewDoorState,
		EDoorDirection OldDoorDirection, EDoorDirection NewDoorDirection, bool bClientSimulation);

	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Finished Closing"))
	void K2_OnDoorFinishedClosing(bool bClientSimulation);
	
	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Finished Opening"))
	void K2_OnDoorFinishedOpening(bool bClientSimulation);

	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Started Closing"))
	void K2_OnDoorStartedClosing(bool bClientSimulation);
	
	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Started Opening"))
	void K2_OnDoorStartedOpening(bool bClientSimulation);

	/** Called when the door is in motion, and was interacted with, causing it to change motion */
	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door In Motion Interrupted"))
	void K2_OnDoorInMotionInterrupted(EDoorState OldDoorState, EDoorState NewDoorState, EDoorDirection OldDoorDirection,
		EDoorDirection NewDoorDirection, bool bClientSimulation);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category=Door, meta=(DisplayName="On Door Finished Closing (Cosmetic)"))
	void K2_OnDoorFinishedClosingCosmetic();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category=Door, meta=(DisplayName="On Door Finished Opening (Cosmetic)"))
	void K2_OnDoorFinishedOpeningCosmetic();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category=Door, meta=(DisplayName="On Door Started Closing (Cosmetic)"))
	void K2_OnDoorStartedClosingCosmetic();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category=Door, meta=(DisplayName="On Door Started Opening (Cosmetic)"))
	void K2_OnDoorStartedOpeningCosmetic();

	/** Called when the door is in motion, and was interacted with, causing it to change motion */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category=Door, meta=(DisplayName="On Door In Motion Interrupted (Cosmetic)"))
	void K2_OnDoorInMotionInterruptedCosmetic(EDoorState OldDoorState, EDoorState NewDoorState,
		EDoorDirection OldDoorDirection, EDoorDirection NewDoorDirection);

public:
	/**
	 * Controls how Alpha updates
	 * @warning InterpTo is framerate dependent and should be avoided if the door can collide with player characters
	 * @warning Use InterpConstant instead
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Time")
	EAlphaMode DoorAlphaMode = EAlphaMode::Time;

	/**
	 * If true, Tick will be disabled when no longer in motion (transitioning)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Time", meta=(EditCondition="DoorAlphaMode!=EAlphaMode::Disabled", EditConditionHides))
	bool bAutoDisableTickState = true;

	bool ShouldAutoDisableTickState() const { return bAutoDisableTickState && DoorAlphaMode != EAlphaMode::Disabled; }
	
	/** How long the door takes to open in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Time", meta=(EditCondition="DoorAlphaMode==EAlphaMode::Time", EditConditionHides, ClampMin="0", UIMin="0", UIMax="3", Delta="0.05", ForceUnits="seconds"))
	float DoorOpenOutwardTime = 0.5f;

	/** How long the door takes to open in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Time", meta=(EditCondition="DoorAlphaMode==EAlphaMode::Time", EditConditionHides, ClampMin="0", UIMin="0", UIMax="3", Delta="0.05", ForceUnits="seconds"))
	float DoorOpenInwardTime = 0.5f;

	/** How long the door takes to close in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Time", meta=(EditCondition="DoorAlphaMode==EAlphaMode::Time", EditConditionHides, ClampMin="0", UIMin="0", UIMax="3", Delta="0.05", ForceUnits="seconds"))
	float DoorCloseOutwardTime = 0.5f;

	/** How long the door takes to close in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Time", meta=(EditCondition="DoorAlphaMode==EAlphaMode::Time", EditConditionHides, ClampMin="0", UIMin="0", UIMax="3", Delta="0.05", ForceUnits="seconds"))
	float DoorCloseInwardTime = 0.5f;
	
	/**
	 * How fast the door interpolates to the target alpha
	 * @warning InterpTo is framerate dependent and should be avoided if the door can collide with player characters
	 * @warning Use InterpConstant instead
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Time", meta=(EditCondition="DoorAlphaMode==EAlphaMode::InterpTo||DoorAlphaMode==EAlphaMode::InterpConstant", EditConditionHides, ClampMin="0", UIMin="0", UIMax="300", Delta="0.5", ForceUnits="x"))
	float DoorOpenOutwardInterpRate = 4.f;

	/**
	 * How fast the door interpolates to the target alpha
	 * @warning InterpTo is framerate dependent and should be avoided if the door can collide with player characters
	 * @warning Use InterpConstant instead
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Time", meta=(EditCondition="DoorAlphaMode==EAlphaMode::InterpTo||DoorAlphaMode==EAlphaMode::InterpConstant", EditConditionHides, ClampMin="0", UIMin="0", UIMax="300", Delta="0.5", ForceUnits="x"))
	float DoorOpenInwardInterpRate = 4.f;

	/**
	 * How fast the door interpolates to the target alpha
	 * @warning InterpTo is framerate dependent and should be avoided if the door can collide with player characters
	 * @warning Use InterpConstant instead
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Time", meta=(EditCondition="DoorAlphaMode==EAlphaMode::InterpTo||DoorAlphaMode==EAlphaMode::InterpConstant", EditConditionHides, ClampMin="0", UIMin="0", UIMax="300", Delta="0.5", ForceUnits="x"))
	float DoorCloseOutwardInterpRate = 4.f;

	/**
	 * How fast the door interpolates to the target alpha
	 * @warning InterpTo is framerate dependent and should be avoided if the door can collide with player characters
	 * @warning Use InterpConstant instead
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Time", meta=(EditCondition="DoorAlphaMode==EAlphaMode::InterpTo||DoorAlphaMode==EAlphaMode::InterpConstant", EditConditionHides, ClampMin="0", UIMin="0", UIMax="300", Delta="0.5", ForceUnits="x"))
	float DoorCloseInwardInterpRate = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Time", meta=(EditCondition="DoorAlphaMode==EAlphaMode::InterpTo", EditConditionHides, ClampMin="0.0001", UIMin="0.0001", UIMax="1", Delta="0.01"))
	float DoorInterpToTolerance = 0.01f;
	
#if WITH_EDITORONLY_DATA
public:
	UPROPERTY(EditDefaultsOnly, Category="Door Preview")
	bool bPlayAnimationPreview = false;

	UPROPERTY(EditDefaultsOnly, Category="Door Preview", meta=(ValidEnumValues="Opening,Closing"))
	EDoorState PreviewState = EDoorState::Opening;

	UPROPERTY(EditDefaultsOnly, Category="Door Preview")
	EDoorDirection PreviewDirection = EDoorDirection::Inward;

	UPROPERTY(EditDefaultsOnly, Category="Door Preview", meta=(ClampMin="1", UIMin="1", UIMax="240", Delta="1", ForceUnits="hz"))
	float PreviewSimulationRate = 60.f;

	UPROPERTY(EditDefaultsOnly, Category="Door Preview", meta=(ClampMin="0", UIMin="0", UIMax="3", Delta="0.1", ForceUnits="s"))
	float PreviewStartDelay = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category="Door Preview", meta=(ClampMin="0", UIMin="0", UIMax="3", Delta="0.1", ForceUnits="s"))
	float PreviewLoopDelay = 0.35f;
	
	UPROPERTY(EditDefaultsOnly, Category="Door Preview")
	bool bLoopPreview = true;

	UPROPERTY(Transient)
	bool bWasPlayingAnimationPreview = false;
	
	UPROPERTY(Transient)
	EDoorState PreviewStateStart = EDoorState::Closed;

	UPROPERTY(Transient)
	EDoorDirection PreviewDirectionStart = EDoorDirection::Outward;

	UPROPERTY(Transient)
	float PreviewDeltaTime = 0.f;
	
	FTimerHandle StartPreviewTimerHandle;
	FTimerHandle TickPreviewTimerHandle;
	
	void StartPreviewAnimation(bool bIsLoop);

	void ClearPreviewAnimation();

	UFUNCTION()
	void TickPreviewAnimation();

#endif
	
public:
	UFUNCTION(BlueprintPure, Category=Door)
	float GetDoorTransitionTime() const;

	UFUNCTION(BlueprintPure, Category=Door)
	float GetDoorTransitionTimeFromState(EDoorState State, EDoorDirection Direction) const;

	UFUNCTION(BlueprintPure, Category=Door)
	float GetDoorInterpRate() const;

	UFUNCTION(BlueprintPure, Category=Door)
	float GetDoorInterpRateFromState(EDoorState State, EDoorDirection Direction) const;
	
public:
	/** @return True if the door alpha changed */
	UFUNCTION(BlueprintCallable, Category=Door)
	bool SetDoorAlpha(float NewDoorAlpha);

	UFUNCTION(BlueprintPure, Category=Door)
	float GetDoorAlpha() const { return DoorAlpha; }

	UFUNCTION(BlueprintPure, Category=Door)
	float GetDoorAlphaAbs() const { return FMath::Abs<float>(DoorAlpha); }

	UFUNCTION(BlueprintPure, Category=Door)
	float GetDoorAlphaFromDoorTime(float DoorTime, EDoorState State, EDoorDirection Direction) const;
	
	UFUNCTION(BlueprintPure, Category=Door)
	float GetDoorTimeFromAlpha(float Alpha, EDoorState State, EDoorDirection Direction) const;

	void OnDoorAlphaChanged(float OldDoorAlpha, float NewDoorAlpha);

	/**
	 * Propagate the Door Alpha again, resulting in another call to OnDoorAlphaChanged
	 * Useful when we want to keep pushing the door on tick despite no change in alpha, e.g. with sweep detection
	 */
	UFUNCTION(BlueprintCallable, Category=Door)
	void TriggerOnDoorAlphaChanged();
	
	/**
	 * Called when the door alpha changes
	 * @param OldDoorAlpha The previous door alpha
	 * @param NewDoorAlpha The new door alpha
	 * @param State The current door state
	 * @param Direction The current door direction
	 * @param DoorTime The door time based on the new alpha ONLY if DoorAlphaMode is set to Time
	 * @param DoorTransitionTime The door transition time based on the new alpha ONLY if DoorAlphaMode is set to Time
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Alpha Changed"))
	void K2_OnDoorAlphaChanged(float OldDoorAlpha, float NewDoorAlpha, EDoorState State, EDoorDirection Direction,
		float DoorTime, float DoorTransitionTime);
	
protected:
	// Door Access

	/** Which side(s) of the door can we interact from */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category=Door)
	EDoorAccess DoorAccess = EDoorAccess::Bidirectional;

	/** Which ways can the door open */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category=Door)
	EDoorOpenDirection DoorOpenDirection = EDoorOpenDirection::Bidirectional;

	/**
	 * Which action we prefer to use when opening the door
	 * We might not always use the preferred motion, e.g. we would only push a door open if we're behind it and it opens outwards
	 */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category=Door)
	EDoorMotion DoorOpenMotion = EDoorMotion::Push;

protected:
	/** What to do if changing door access based on the door state */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Change")
	EDoorChangeType DoorAccessChangeType = EDoorChangeType::Wait;

	/** Used if change type is set to Wait */
	UPROPERTY(BlueprintReadOnly, Category="Door Change")
	EDoorAccess PendingDoorAccess = DoorAccess;

	/** Used if change type is set to Wait */
	UPROPERTY(BlueprintReadOnly, Category="Door Change")
	bool bHasPendingDoorAccess = false;

protected:
	/** What to do if changing door open mode based on the door state */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Change")
	EDoorChangeType DoorOpenDirectionChangeType = EDoorChangeType::Wait;

	/** Used if change type is set to Wait */
	UPROPERTY(BlueprintReadOnly, Category="Door Change")
	EDoorOpenDirection PendingDoorOpenDirection = DoorOpenDirection;

	/** Used if change type is set to Wait */
	UPROPERTY(BlueprintReadOnly, Category="Door Change")
	bool bHasPendingDoorOpenDirection = false;

protected:
	/** What to do if changing door access based on the door state */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Change")
	EDoorChangeType DoorOpenMotionChangeType = EDoorChangeType::Immediate;

	/** Used if change type is set to Wait */
	UPROPERTY(BlueprintReadOnly, Category="Door Change")
	EDoorMotion PendingDoorOpenMotion = DoorOpenMotion;

	/** Used if change type is set to Wait */
	UPROPERTY(BlueprintReadOnly, Category="Door Change")
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Properties")
	bool bTrustClientDoorSide = true;

	/** If true, the door can be interacted with while in motion, otherwise they must wait for it to finish opening or closing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Properties")
	bool bCanInteractWhileInMotion = true;

public:
	UFUNCTION(BlueprintPure, Category=Door)
	bool CanInteractWhileInMotion() const { return bCanInteractWhileInMotion; }

public:
	/** How long to wait before interaction since the door was last in motion (opening or closing, i.e. recently interacted with) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Properties")
	float MotionCooldown = 0.2f;

	/** How long to wait before interaction since the door entered a stationary state (open or closed) after being in motion */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Properties")
	float StationaryCooldown = 0.3f;

	UFUNCTION()
	virtual void OnStationaryCooldownFinished();

	UFUNCTION()
	virtual void OnInMotionCooldownFinished();
	
	/** Called when the door was on cooldown and the cooldown has finished */
	UPROPERTY(BlueprintAssignable, Category=Door)
	FOnDoorCooldownFinished OnDoorCooldownFinishedDelegate;

	/** Called when the door was on cooldown and the cooldown has finished */
	UPROPERTY(BlueprintAssignable, Category=Door)
	FOnDoorCooldownFinished OnDoorStationaryCooldownFinishedDelegate;
	
	/** Called when the door was on cooldown and the cooldown has finished */
	UPROPERTY(BlueprintAssignable, Category=Door)
	FOnDoorCooldownFinished OnDoorInMotionCooldownFinishedDelegate;
	
	/** Get the remaining time until OnDoorCooldownFinished is called */
	UFUNCTION(BlueprintPure, Category=Door)
	float GetRemainingCooldown() const;

	/** Get the remaining time until OnDoorCooldownFinished is called */
	UFUNCTION(BlueprintPure, Category=Door)
	float GetRemainingStationaryCooldown() const;

	/** Get the remaining time until OnDoorCooldownFinished is called */
	UFUNCTION(BlueprintPure, Category=Door)
	float GetRemainingInMotionCooldown() const;

	FTimerHandle MotionCooldownTimerHandle;
	FTimerHandle StationaryCooldownTimerHandle;

	/** Last time the door was interacted with successfully */
	UPROPERTY(BlueprintReadOnly, Category=Door)
	float LastInMotionTime = -1.f;

	/** Last time the door entered a stationary state after being in motion */
	UPROPERTY(BlueprintReadOnly, Category=Door)
	float LastStationaryTime = -1.f;

protected:
	/** Represents the value in -1 to 1 range by which the door is open or closed, -1 and 1 are fully open inward / outward and 0 is fully closed */
	UPROPERTY(VisibleInstanceOnly, Category=Door, meta=(ClampMin="-1", UIMin="-1", ClampMax="1", UIMax="1", ForceUnits="Percent"))
	float DoorAlpha = 0.f;

public:
	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorOnCooldown() const;

	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorOnMotionCooldown() const;

	UFUNCTION(BlueprintPure, Category=Door)
	bool IsDoorOnStationaryCooldown() const;

public:
	/**
	 * Call from ShouldAbilityRespondToEvent() to determine if the door ability should activate, i.e. we can interact with the door
	 * @param Avatar The avatar that is interacting with the door
	 * @param ClientDoorState The door state the client is trying to set
	 * @param ClientDoorDirection The door direction the client is trying to set
	 * @param ClientDoorSide The door side the client is claiming to be on
	 * @param NewDoorState The resulting new door state to set
	 * @param NewDoorDirection The resulting new door direction to set
	 * @param DoorMotion The resulting door motion to set
	 * @param FailReason The reason the door failed to respond to the event -- useful for UI purposes such as showing a Lock icon
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Door)
	virtual bool ShouldAbilityRespondToDoorEvent(const AActor* Avatar, EDoorState ClientDoorState,
		EDoorDirection ClientDoorDirection, EDoorSide ClientDoorSide, EDoorState& NewDoorState,
		EDoorDirection& NewDoorDirection, EDoorMotion& DoorMotion, FGameplayTag& FailReason) const;

public:
	// General helpers

	/**
	 * Get the door side based on the avatar's location and the door's location
	 */
	UFUNCTION(BlueprintCallable, Category=Door)
	EDoorSide GetDoorSide(const AActor* Avatar) const;
	
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

protected:
	FString GetRoleString() const;

public:
#if WITH_EDITOR
	virtual void HandleDoorPropertyChange();
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostCDOCompiled(const FPostCDOCompiledContext& Context) override;
#endif
};
