// Copyright (c) Jared Taylor


#include "Door.h"

#include "DoorStatics.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Engine/World.h"
#include "TimerManager.h"

#if WITH_EDITORONLY_DATA
#include "Visualizers/DoorEditorVisualizer.h"
#include "Visualizers/DoorSpriteWidgetComponent.h"
#endif

#include "DoorTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Door)

namespace DoorCVars
{
#if WITH_EDITORONLY_DATA
	static bool bShowDoorStateDuringPIE = true;
	FAutoConsoleVariableRef CVarShowDoorStateDuringPIE(
		TEXT("p.Door.ShowDoorStateDuringPIE"),
		bShowDoorStateDuringPIE,
		TEXT("If true, draw sprites showing the door state during PIE\n"),
		ECVF_Default);
#endif

#if UE_ENABLE_DEBUG_DRAWING && WITH_EDITOR
	static bool bDoorDebugServer = false;
	FAutoConsoleVariableRef CVarDoorDebugServer(
		TEXT("p.Door.DebugServer"),
		bDoorDebugServer,
		TEXT("Draw door bounds for the server during PIE.\n"),
		ECVF_Default);
#endif
}

TArray<FGameplayAbilityTargetData*> ADoor::GatherOptionalGraspTargetData(const FGameplayAbilityActorInfo* ActorInfo) const
{
	// Send data to interaction ability via Grasp
	if (const AActor* AvatarActor = ActorInfo && ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor.Get() : nullptr)
	{
		const EDoorState CurrentDoorState = GetDoorState();
		const EDoorDirection CurrentDoorDirection = GetDoorDirection();
		const EDoorSide CurrentDoorSide = GetDoorSide(AvatarActor);
		auto* DoorTargetData = new FDoorAbilityTargetData(CurrentDoorState, CurrentDoorDirection, CurrentDoorSide);
		return { DoorTargetData };
	}
	return {};
}

ADoor::ADoor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	NetCullDistanceSquared = 25000000.0;  // 5000cm
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	
#if WITH_EDITORONLY_DATA
	// Draw editor visualization
	DoorVisualizer = CreateEditorOnlyDefaultSubobject<UDoorEditorVisualizer>(TEXT("DoorVisualizer"));

	// Draw PIE visualization
	DoorSprite = CreateEditorOnlyDefaultSubobject<UDoorSpriteWidgetComponent>(TEXT("DoorSprite"));
	DoorSprite->SetGenerateOverlapEvents(false);
	DoorSprite->SetCollisionProfileName(TEXT("NoCollision"));
	DoorSprite->SetupAttachment(RootComponent);
	DoorSprite->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	DoorSprite->SetWidgetSpace(EWidgetSpace::Screen);
	DoorSprite->SetDrawAtDesiredSize(true);
#endif
}

void ADoor::BeginPlay()
{
#if WITH_EDITORONLY_DATA
	ClearPreviewAnimation();
#endif
	
	Super::BeginPlay();

	UE_LOG(LogDoors, Verbose, TEXT("%s ADoor::BeginPlay Initialize Alpha: %.2f, %s"), *GetRoleString(), DoorAlpha, *GetName());

	// Initialize the position of the door
	OnDoorStateChanged(DoorState, DoorState, DoorDirection, DoorDirection, nullptr, false);

	// Initialize the alpha -- OnDoorStateChanged won't do this for these specific states
	switch (DoorState)
	{
	case EDoorState::Closing:
		SetDoorAlpha(DoorDirection == EDoorDirection::Inward ? -1.f : 1.f);
		break;
	case EDoorState::Opening:
		SetDoorAlpha(0.f);
		break;
	default: break;
	}

#if WITH_EDITORONLY_DATA
	if (GetNetMode() != NM_DedicatedServer)
	{
		// Bind the CVar delegate so we know if user toggled the drawing
		const FConsoleVariableDelegate ShowDoorStateDuringPIEDelegate = FConsoleVariableDelegate::CreateUObject(this,
			&ThisClass::OnToggleShowDoorStateDuringPIE);
	
		DoorCVars::CVarShowDoorStateDuringPIE->SetOnChangedCallback(ShowDoorStateDuringPIEDelegate);
	}
#endif
}

void ADoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickDoor(DeltaTime);

#if UE_ENABLE_DEBUG_DRAWING && WITH_EDITOR
	if (HasAuthority() && GetWorld() && GetWorld()->IsPlayInEditor() && DoorCVars::bDoorDebugServer)
	{
		const FBox Box = GetComponentsBoundingBox();
		const FVector Extents = Box.GetExtent();
		const FVector Origin = Box.GetCenter();
		DrawDebugBox(GetWorld(), Origin, Extents, GetActorQuat(), FColor::Orange,
			false, -1.f, SDPG_Foreground, 1.f);
	}
#endif
}

void ADoor::TickDoor_Implementation(float DeltaTime)
{
	const float TargetAlpha = GetTargetDoorAlpha();

	switch (DoorAlphaMode)
	{
	case EAlphaMode::Time:
		{
			// We want to increment the door alpha based on the time it takes to open/close
			const float DoorTime = GetDoorTransitionTime();
			const float Rate = 1.f / FMath::Max<float>(DoorTime, 0.001f);
			const float Direction = DoorDirection == EDoorDirection::Inward ? -1.f : 1.f;
			const float Time = Rate * Direction * DeltaTime;
			switch (DoorState)
			{
			case EDoorState::Closing:
				{
					float NewAlpha = DoorAlpha - Time;
					// Detect change of direction, i.e. overshot the closing
					if (FMath::Sign(DoorAlpha) != FMath::Sign(NewAlpha))
					{
						NewAlpha = 0.f;
					}
					SetDoorAlpha(NewAlpha);
				}
				break;
			case EDoorState::Opening:
				{
					SetDoorAlpha(DoorAlpha + Time);
				}
				break;
			default:
				if (ShouldAutoDisableTickState())
				{
					SetActorTickEnabled(false);
				}
			}
		}
		break;
	case EAlphaMode::InterpConstant:
		{
			const float InterpRate = GetDoorInterpRate();
			const float NewAlpha = FMath::FInterpConstantTo(DoorAlpha, TargetAlpha, DeltaTime, InterpRate);
			SetDoorAlpha(NewAlpha);
		}
		break;
	case EAlphaMode::InterpTo:
		{
			const float InterpRate = GetDoorInterpRate();
			const float NewAlpha = FMath::FInterpTo(DoorAlpha, TargetAlpha, DeltaTime, InterpRate);
			if (FMath::IsNearlyEqual(NewAlpha, TargetAlpha, DoorInterpToTolerance))
			{
				SetDoorAlpha(TargetAlpha);
			}
			else
			{
				SetDoorAlpha(NewAlpha);
			}
		}
		break;
	case EAlphaMode::Disabled:
		break;
	}
}

float ADoor::GetTargetDoorAlpha() const
{
	return GetTargetDoorAlphaFromState(DoorState, DoorDirection);
}

float ADoor::GetTargetDoorAlphaFromState(EDoorState State, EDoorDirection Direction) const
{
	switch (State)
	{
	case EDoorState::Opening:
	case EDoorState::Open:
		switch (Direction)
		{
		case EDoorDirection::Outward: return 1.f;
		case EDoorDirection::Inward: return -1.f;
		}
		break;
	default: return 0.f;
	}
	return 0.f;
}

#if WITH_EDITORONLY_DATA

void ADoor::OnToggleShowDoorStateDuringPIE(IConsoleVariable* CVar)
{
	if (CVar)
	{
		DoorSprite->SetVisibility(CVar->GetBool());
	}
}

#endif

void ADoor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	SharedParams.Condition = COND_None;
	
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DoorAccess, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DoorOpenDirection, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DoorOpenMotion, SharedParams);

	// We make the LastAvatar the owner for a short time so they don't fight the prediction
	SharedParams.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RepDoorState, SharedParams);
}

// -------------------------------------------------------------
// Door State

void ADoor::OnRep_DoorState()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ADoor::OnRep_DoorState);
	
	EDoorState NewDoorState;
	EDoorDirection NewDoorDirection;
	UDoorStatics::UnpackDoorState(RepDoorState, NewDoorState, NewDoorDirection);
	SetDoorState(NewDoorState, NewDoorDirection, nullptr, true);

#if WITH_EDITORONLY_DATA
	if (GetNetMode() != NM_DedicatedServer && DoorCVars::bShowDoorStateDuringPIE)
	{
		DoorSprite->OnRepDoorStateChanged(NewDoorState, NewDoorDirection);
	}
#endif
}

void ADoor::SetDoorStateReplicationEnabled(bool bEnabled, bool bReplicateNow)
{
	if (HasAuthority() && GetNetMode() != NM_Standalone && bEnableDoorStateReplication != bEnabled)
	{
		bEnableDoorStateReplication = bEnabled;
		if (bEnableDoorStateReplication && bReplicateNow)
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RepDoorState, this);
			ForceNetUpdate();
		}
	}
}

float ADoor::GetLastAvatarReplicationExpirationTime_Implementation() const
{
	return LastAvatarReplicationExpirationTime;
}

void ADoor::GetRepDoorState(EDoorState& OutDoorState, EDoorDirection& OutDoorDirection) const
{
	UDoorStatics::UnpackDoorState(RepDoorState, OutDoorState, OutDoorDirection);
}

void ADoor::SetDoorState(EDoorState NewDoorState, EDoorDirection NewDoorDirection, AActor* Avatar, bool bClientSimulation)
{
	if (DoorState != NewDoorState || DoorDirection != NewDoorDirection)
	{
		const EDoorState OldDoorState = DoorState;
		const EDoorDirection OldDoorDirection = DoorDirection;
		DoorState = NewDoorState;
		DoorDirection = NewDoorDirection;
		OnDoorStateChanged(OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection, Avatar, bClientSimulation);
	}
}

void ADoor::OnDoorStateChanged(EDoorState OldDoorState, EDoorState NewDoorState, EDoorDirection OldDoorDirection,
	EDoorDirection NewDoorDirection, AActor* Avatar, bool bClientSimulation)
{
	UE_LOG(LogDoors, Verbose, TEXT("%s ADoor::OnDoorStateChanged: %s → %s"), *GetRoleString(),
		*UDoorStatics::DoorStateDirectionToString(OldDoorState, OldDoorDirection),
		*UDoorStatics::DoorStateDirectionToString(NewDoorState, NewDoorDirection));
	
	// Update the last time the door state changed
	LastDoorStateChangeTime = GetWorld()->GetTimeSeconds();

	// Update the last avatar
	LastAvatar = Avatar;
	if (HasAuthority() && IsValid(Avatar))
	{
		// We make the LastAvatar the owner for a short time so their replication doesn't fight the prediction
		SetOwner(Avatar);

		// Clear the LastAvatar after a short time
		const float ReplicationExpirationTime = GetLastAvatarReplicationExpirationTime();
		if (ReplicationExpirationTime > 0.f)
		{
			// Clear in-progress timer
			GetWorldTimerManager().ClearTimer(LastAvatarReplicationTimerHandle);
		
			const FTimerDelegate TimerDelegate = FTimerDelegate::CreateWeakLambda(this, [this]
			{
				LastAvatar = nullptr;
				SetOwner(nullptr);
			});
			GetWorldTimerManager().SetTimer(LastAvatarReplicationTimerHandle, TimerDelegate, ReplicationExpirationTime, false);
		}
	}

	// Update door access
	if (bHasPendingDoorAccess)
	{
		if (DoorAccess != PendingDoorAccess)
		{
			if (CanChangeDoorAccess(PendingDoorAccess))
			{
				DoorAccess = PendingDoorAccess;
				OnDoorAccessChanged(DoorAccess, PendingDoorAccess);
			}
		}
		else
		{
			bHasPendingDoorAccess = false;
		}
	}

	// Update door open direction
	if (bHasPendingDoorOpenDirection)
	{
		if (DoorOpenDirection != PendingDoorOpenDirection)
		{
			if (CanChangeDoorOpenDirection(PendingDoorOpenDirection))
			{
				DoorOpenDirection = PendingDoorOpenDirection;
				OnDoorOpenDirectionChanged(DoorOpenDirection, PendingDoorOpenDirection);
			}
		}
		else
		{
			bHasPendingDoorOpenDirection = false;
		}
	}

	// Update door open motion
	if (bHasPendingDoorOpenMotion)
	{
		if (DoorOpenMotion != PendingDoorOpenMotion)
		{
			if (CanChangeDoorOpenMotion(PendingDoorOpenMotion))
			{
				DoorOpenMotion = PendingDoorOpenMotion;
				OnDoorOpenMotionChanged(DoorOpenMotion, PendingDoorOpenMotion);
			}
		}
		else
		{
			bHasPendingDoorOpenMotion = false;
		}
	}
	
	// Helper events
	switch (NewDoorState)
	{
	case EDoorState::Closed:
		OnDoorFinishedClosing(bClientSimulation);
		break;
	case EDoorState::Closing:
		OnDoorStartedClosing(bClientSimulation);
		break;
	case EDoorState::Opening:
		OnDoorStartedOpening(bClientSimulation);
		break;
	case EDoorState::Open:
		OnDoorFinishedOpening(bClientSimulation);
		break;
	}
	
	if (IsDoorStateInMotion(OldDoorState))
	{
		OnDoorInMotionInterrupted(OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection, bClientSimulation);
	}

	// Replicate the door state to clients
	if (HasAuthority() && GetNetMode() != NM_Standalone && bEnableDoorStateReplication)
	{
		RepDoorState = UDoorStatics::PackDoorState(DoorState, DoorDirection);
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RepDoorState, this);
	}

	// Blueprint callback
	K2_OnDoorStateChanged(OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection, Avatar, bClientSimulation);

	// Delegate callback
	if (OnDoorStateChangedDelegate.IsBound())
	{
		OnDoorStateChangedDelegate.Broadcast(this, OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection);
	}

	// Cosmetic notifies for VFX/SFX
	if (GetNetMode() != NM_DedicatedServer)
	{
		K2_OnDoorStateChangedCosmetic(OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection, Avatar, bClientSimulation);
	}

#if WITH_EDITORONLY_DATA
	if (GetNetMode() != NM_DedicatedServer && DoorCVars::bShowDoorStateDuringPIE)
	{
		DoorSprite->SetVisibility(true);
	}
#endif
}

void ADoor::OnDoorFinishedOpening(bool bClientSimulation)
{
	if (ShouldAutoDisableTickState()) { SetActorTickEnabled(false); }

	UE_LOG(LogDoors, Verbose, TEXT("%s ADoor::OnDoorFinishedOpening: %s"), *GetRoleString(), *GetNameSafe(this));

	LastStationaryTime = GetWorld()->GetTimeSeconds();
	if (StationaryCooldown > 0.f)
	{
		GetWorldTimerManager().SetTimer(StationaryCooldownTimerHandle, this, &ThisClass::OnStationaryCooldownFinished,
			StationaryCooldown, false);
	}

	SetDoorAlpha(GetTargetDoorAlpha());

	K2_OnDoorFinishedOpening(bClientSimulation);
	if (GetNetMode() != NM_DedicatedServer)
	{
		K2_OnDoorFinishedOpeningCosmetic();
	}
}

void ADoor::OnDoorFinishedClosing(bool bClientSimulation)
{
	if (ShouldAutoDisableTickState()) { SetActorTickEnabled(false); }

	UE_LOG(LogDoors, Verbose, TEXT("%s ADoor::OnDoorFinishedClosing: %s"), *GetRoleString(), *GetNameSafe(this));
	
	LastStationaryTime = GetWorld()->GetTimeSeconds();
	if (StationaryCooldown > 0.f)
	{
		GetWorldTimerManager().SetTimer(StationaryCooldownTimerHandle, this, &ThisClass::OnStationaryCooldownFinished,
			StationaryCooldown, false);
	}

	SetDoorAlpha(GetTargetDoorAlpha());
	
	K2_OnDoorFinishedClosing(bClientSimulation);
	if (GetNetMode() != NM_DedicatedServer)
	{
		K2_OnDoorFinishedClosingCosmetic();
	}
}

void ADoor::OnDoorStartedOpening(bool bClientSimulation)
{
	if (DoorAlphaMode != EAlphaMode::Disabled)
	{
		SetActorTickEnabled(true);
	}

	if (bClientSimulation && IsDoorInMotion())
	{
		return;
	}
	
	UE_LOG(LogDoors, Verbose, TEXT("%s ADoor::OnDoorStartedOpening: %s"), *GetRoleString(), *GetNameSafe(this));

	LastInMotionTime = GetWorld()->GetTimeSeconds();
	if (MotionCooldown > 0.f)
	{
		GetWorldTimerManager().SetTimer(MotionCooldownTimerHandle, this, &ThisClass::OnInMotionCooldownFinished,
			MotionCooldown, false);
	}
	
	K2_OnDoorStartedOpening(bClientSimulation);
	if (GetNetMode() != NM_DedicatedServer)
	{
		K2_OnDoorStartedOpeningCosmetic();
	}
}

void ADoor::OnDoorStartedClosing(bool bClientSimulation)
{
	if (DoorAlphaMode != EAlphaMode::Disabled)
	{
		SetActorTickEnabled(true);
	}
	
	if (bClientSimulation && IsDoorInMotion())
	{
		return;
	}
	
	UE_LOG(LogDoors, Verbose, TEXT("%s ADoor::OnDoorStartedClosing: %s"), *GetRoleString(), *GetNameSafe(this));

	LastInMotionTime = GetWorld()->GetTimeSeconds();
	if (MotionCooldown > 0.f)
	{
		GetWorldTimerManager().SetTimer(MotionCooldownTimerHandle, this, &ThisClass::OnInMotionCooldownFinished,
			MotionCooldown, false);
	}

	K2_OnDoorStartedClosing(bClientSimulation);
	if (GetNetMode() != NM_DedicatedServer)
	{
		K2_OnDoorStartedClosingCosmetic();
	}
}

void ADoor::OnDoorInMotionInterrupted(EDoorState OldDoorState, EDoorState NewDoorState, EDoorDirection OldDoorDirection,
	EDoorDirection NewDoorDirection, bool bClientSimulation)
{
	UE_LOG(LogDoors, Verbose, TEXT("%s ADoor::OnDoorInMotionInterrupted: %s"), *GetRoleString(), *GetNameSafe(this));
	
	K2_OnDoorInMotionInterrupted(OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection, bClientSimulation);
	if (GetNetMode() != NM_DedicatedServer)
	{
		K2_OnDoorInMotionInterruptedCosmetic(OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection);
	}
}

#if WITH_EDITOR

void ADoor::StartPreviewAnimation(bool bIsLoop)
{
	if (!bPlayAnimationPreview)
	{
		ClearPreviewAnimation();
		return;
	}
	
	// Cache properties to restore after the preview
	PreviewStateStart = DoorState;
	PreviewDirectionStart = DoorDirection;
	bWasPlayingAnimationPreview = true;
	PreviewDeltaTime = 1.f / PreviewSimulationRate;

	DoorState = PreviewState;
	DoorDirection = PreviewDirection;
	if (IsDoorStateOpenOrOpening(PreviewState))
	{
		DoorAlpha = 0.f;
	}
	else
	{
		DoorAlpha = PreviewDirection == EDoorDirection::Inward ? -1.f : 1.f;
	}

	// Set the timers to tick the preview
	const float& StartDelay = bIsLoop ? PreviewLoopDelay : PreviewStartDelay;
	if (StartDelay > 0.01f)
	{
		const FTimerDelegate StartPreviewDelegate = FTimerDelegate::CreateWeakLambda(this, [this]
		{
			if (bWasPlayingAnimationPreview)
			{
				// Start the preview
				GetWorldTimerManager().SetTimer(TickPreviewTimerHandle, this, &ThisClass::TickPreviewAnimation, PreviewDeltaTime, true);
			}
		});

		// Start the preview after a start delay
		GetWorldTimerManager().SetTimer(StartPreviewTimerHandle, StartPreviewDelegate, StartDelay, false);
	}
	else
	{
		// Start the preview immediately
		GetWorldTimerManager().SetTimer(TickPreviewTimerHandle, this, &ThisClass::TickPreviewAnimation, PreviewDeltaTime, true);
	}
}

void ADoor::ClearPreviewAnimation()
{
	if (bWasPlayingAnimationPreview)
	{
		// Restore the door state
		DoorState = PreviewStateStart;
		DoorDirection = PreviewDirectionStart;
		SetDoorAlpha(0.f);
		bWasPlayingAnimationPreview = false;

		GetWorldTimerManager().ClearTimer(StartPreviewTimerHandle);
		GetWorldTimerManager().ClearTimer(TickPreviewTimerHandle);
	}
}

void ADoor::TickPreviewAnimation()
{
	if (bWasPlayingAnimationPreview)
	{
		if (bPlayAnimationPreview)
		{
			TickDoor(PreviewDeltaTime);

			if (IsDoorStationary())
			{
				ClearPreviewAnimation();

				if (bLoopPreview)
				{
					StartPreviewAnimation(true);
				}
			}
		}
		else
		{
			ClearPreviewAnimation();
		}
	}
}
#endif

float ADoor::GetDoorTransitionTime() const
{
	return GetDoorTransitionTimeFromState(DoorState, DoorDirection);
}

float ADoor::GetDoorTransitionTimeFromState(EDoorState State, EDoorDirection Direction) const
{
	switch (State)
	{
	case EDoorState::Opening:
	case EDoorState::Open:
		switch (Direction)
		{
		case EDoorDirection::Outward: return DoorOpenOutwardTime;
		case EDoorDirection::Inward: return DoorOpenInwardTime;
		}
		break;
	case EDoorState::Closing:
	case EDoorState::Closed:
		switch (Direction)
		{
		case EDoorDirection::Outward: return DoorCloseOutwardTime;
		case EDoorDirection::Inward: return DoorCloseInwardTime;
		}
		break;
	default: return 0.f;
	}
	return 0.f;
}

float ADoor::GetDoorInterpRate() const
{
	return GetDoorInterpRateFromState(DoorState, DoorDirection);
}

float ADoor::GetDoorInterpRateFromState(EDoorState State, EDoorDirection Direction) const
{
	switch (State)
	{
	case EDoorState::Opening:
	case EDoorState::Open:
		switch (Direction)
		{
	case EDoorDirection::Outward: return DoorOpenOutwardInterpRate;
	case EDoorDirection::Inward: return DoorOpenInwardInterpRate;
		}
		break;
	case EDoorState::Closing:
	case EDoorState::Closed:
		switch (Direction)
		{
	case EDoorDirection::Outward: return DoorCloseOutwardInterpRate;
	case EDoorDirection::Inward: return DoorCloseInwardInterpRate;
		}
		break;
	default: return 0.f;
	}
	return 0.f;
}

// -------------------------------------------------------------
// Door Alpha

bool ADoor::SetDoorAlpha(float NewDoorAlpha)
{
	const float PrevDoorAlpha = DoorAlpha;

	// Clamp -1 to 1
	NewDoorAlpha = FMath::Clamp<float>(NewDoorAlpha, -1.f, 1.f);

	// Snap if nearly finished closing
	if (IsDoorClosedOrClosing() && FMath::IsNearlyZero(NewDoorAlpha))
	{
		UE_LOG(LogDoors, VeryVerbose, TEXT("%s ADoor::SetDoorAlpha: Snap to 0"), *GetRoleString());
		NewDoorAlpha = 0.f;
	}

	// Snap if nearly finished opening
	if (IsDoorOpenOrOpening() && FMath::IsNearlyEqual(FMath::Abs<float>(NewDoorAlpha), 1.f))
	{
		UE_LOG(LogDoors, VeryVerbose, TEXT("%s ADoor::SetDoorAlpha: Snap to 1"), *GetRoleString());
		NewDoorAlpha = FMath::Sign(NewDoorAlpha);
	}

	// Update the door alpha
	DoorAlpha = NewDoorAlpha;

	OnDoorAlphaChanged(PrevDoorAlpha, NewDoorAlpha);
	return true;
}

float ADoor::GetDoorAlphaFromDoorTime(float DoorTime, EDoorState State, EDoorDirection Direction) const
{
	// Determine the alpha based on where DoorTime is in the range of 0 to TransitionTime
	const float TransitionTime = GetDoorTransitionTimeFromState(State, Direction);
	const float DirectionScalar = Direction == EDoorDirection::Inward ? -1.f : 1.f;
	return FMath::Clamp<float>(DoorTime / TransitionTime, 0.f, 1.f) * DirectionScalar;
}

float ADoor::GetDoorTimeFromAlpha(float Alpha, EDoorState State, EDoorDirection Direction) const
{
	// Convert the alpha to time
	const float TransitionTime = GetDoorTransitionTimeFromState(State, Direction);
	return FMath::Lerp<float>(0.f, TransitionTime, FMath::Abs(Alpha));
}

void ADoor::OnDoorAlphaChanged(float OldDoorAlpha, float NewDoorAlpha)
{
	UE_LOG(LogDoors, VeryVerbose, TEXT("%s OnDoorAlphaChanged: OldDoorAlpha: %f, NewDoorAlpha: %f"),
		*GetRoleString(), OldDoorAlpha, NewDoorAlpha);

	// Finalize the door state if required
	if (DoorState == EDoorState::Opening && FMath::IsNearlyEqual(GetDoorAlphaAbs(), 1.f))
	{
		SetDoorState(EDoorState::Open, DoorDirection, nullptr, false);
	}
	else if (DoorState == EDoorState::Closing && FMath::IsNearlyZero(GetDoorAlphaAbs()))
	{
		SetDoorState(EDoorState::Closed, DoorDirection, nullptr, false);
	}

	const float DoorTime = DoorAlphaMode == EAlphaMode::Time ? GetDoorTimeFromAlpha(NewDoorAlpha, DoorState, DoorDirection) : 0.f;
	const float TransitionTime = DoorAlphaMode == EAlphaMode::Time ? GetDoorTransitionTimeFromState(DoorState, DoorDirection) : 0.f;
	
	K2_OnDoorAlphaChanged(OldDoorAlpha, NewDoorAlpha, DoorState, DoorDirection, DoorTime, TransitionTime);

	// Trigger notifies due to change in alpha
	HandleDoorAlphaNotifies(OldDoorAlpha, NewDoorAlpha);
}

void ADoor::TriggerOnDoorAlphaChanged()
{
	OnDoorAlphaChanged(DoorAlpha, DoorAlpha);
}

void ADoor::HandleDoorAlphaNotifies(float OldDoorAlpha, float NewDoorAlpha)
{
	// Notifies
	if (!bNotifyCosmeticOnly || GetNetMode() != NM_DedicatedServer)
	{
		// This can occur from completion events that ensure state/alpha is reached
		if (NewDoorAlpha == OldDoorAlpha)
		{
			return;
		}

		NewDoorAlpha = FMath::Abs<float>(NewDoorAlpha);
		OldDoorAlpha = FMath::Abs<float>(OldDoorAlpha);

		// Iterate door notifies to see if we need to trigger any -- these are pre-sorted by alpha
		const TArray<FDoorNotify>& Notifies = GetDoorNotifies();
		for (const FDoorNotify& Notify : Notifies)
		{
			if (IsDoorOpenOrOpening())
			{
				if (OldDoorAlpha <= Notify.Alpha && NewDoorAlpha >= Notify.Alpha)
				{
					OnDoorNotify(Notify.NotifyTag);
					K2_OnDoorNotify(Notify.NotifyTag);
					break;  // No point testing others due to sorting
				}
			}
			else
			{
				const float ClosingAlpha = 1.f - Notify.Alpha;
				if (OldDoorAlpha >= ClosingAlpha && NewDoorAlpha <= ClosingAlpha)
				{
					OnDoorNotify(Notify.NotifyTag);
					K2_OnDoorNotify(Notify.NotifyTag);
					break;  // No point testing others due to sorting
				}
			}
		}
	}
}

const TArray<FDoorNotify>& ADoor::GetDoorNotifies() const
{
	return GetDoorNotifiesForState(DoorState, DoorDirection);
}

const TArray<FDoorNotify>& ADoor::GetDoorNotifiesForState(EDoorState State, EDoorDirection Direction) const
{
	if (IsDoorStateOpenOrOpening(State))
	{
		switch (Direction)
		{
		case EDoorDirection::Outward: return OpenOutwardNotifies;
		case EDoorDirection::Inward: return OpenInwardNotifies;
		}
	}
	else
	{
		switch (Direction)
		{
		case EDoorDirection::Outward: return CloseOutwardNotifies;
		case EDoorDirection::Inward: return CloseInwardNotifies;
		}
	}
	return CloseOutwardNotifies;
}

// -------------------------------------------------------------
// Door Access

void ADoor::SetDoorAccess(EDoorAccess NewDoorAccess)
{
	if (DoorAccess != NewDoorAccess && CanChangeDoorAccess(NewDoorAccess))
	{
		switch (DoorAccessChangeType)
		{
		case EDoorChangeType::Wait:
			{
				PendingDoorAccess = NewDoorAccess;
				bHasPendingDoorAccess = true;
			}
			break;
		case EDoorChangeType::Immediate:
			{
				const EDoorAccess OldDoorAccess = DoorAccess;
				DoorAccess = NewDoorAccess;
				OnDoorAccessChanged(OldDoorAccess, NewDoorAccess);
			}
			break;
		default:
			break;
		}
	}
}

bool ADoor::CanChangeDoorAccess_Implementation(EDoorAccess NewDoorAccess) const
{
	switch (DoorAccessChangeType)
	{
	case EDoorChangeType::Disabled: return false;
	case EDoorChangeType::Closed: return DoorState == EDoorState::Closed;
	case EDoorChangeType::Wait: return true;
	case EDoorChangeType::Immediate: return true;
	default: return false;
	}
}

void ADoor::OnDoorAccessChanged(EDoorAccess OldDoorAccess, EDoorAccess NewDoorAccess)
{
	UE_LOG(LogDoors, Verbose, TEXT("%s OnDoorAccessChanged: OldDoorAccess: %s, NewDoorAccess: %s"),
		*GetRoleString(), *UDoorStatics::DoorAccessToString(OldDoorAccess), *UDoorStatics::DoorAccessToString(NewDoorAccess));
	
	bHasPendingDoorAccess = false;

	if (HasAuthority() && GetNetMode() != NM_Standalone)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DoorAccess, this);
	}

	K2_OnDoorAccessChanged(OldDoorAccess, NewDoorAccess);
}

// -------------------------------------------------------------
// Door Open Mode

void ADoor::SetDoorOpenDirection(EDoorOpenDirection NewDoorOpenDirection)
{
	if (DoorOpenDirection != NewDoorOpenDirection && CanChangeDoorOpenDirection(NewDoorOpenDirection))
	{
		switch (DoorOpenDirectionChangeType)
		{
		case EDoorChangeType::Wait:
			{
				PendingDoorOpenDirection = NewDoorOpenDirection;
				bHasPendingDoorOpenDirection = true;
			}
			break;
		case EDoorChangeType::Immediate:
			{
				const EDoorOpenDirection OldDoorOpenDirection = DoorOpenDirection;
				DoorOpenDirection = NewDoorOpenDirection;
				OnDoorOpenDirectionChanged(OldDoorOpenDirection, NewDoorOpenDirection);
			}
			break;
		default:
			break;
		}
	}
}

bool ADoor::CanChangeDoorOpenDirection_Implementation(EDoorOpenDirection NewDoorOpenDirection) const
{
	switch (DoorOpenDirectionChangeType)
	{
	case EDoorChangeType::Disabled: return false;
	case EDoorChangeType::Closed: return DoorState == EDoorState::Closed;
	case EDoorChangeType::Wait: return true;
	case EDoorChangeType::Immediate: return true;
	default: return false;
	}
}

void ADoor::OnDoorOpenDirectionChanged(EDoorOpenDirection OldDoorOpenDirection, EDoorOpenDirection NewDoorOpenDirection)
{
	UE_LOG(LogDoors, Verbose, TEXT("%s OnDoorOpenDirectionChanged: OldDoorOpenDirection: %s, NewDoorOpenDirection: %s"),
		*GetRoleString(), *UDoorStatics::DoorOpenDirectionToString(OldDoorOpenDirection), *UDoorStatics::DoorOpenDirectionToString(NewDoorOpenDirection));
	
	bHasPendingDoorAccess = false;

	if (HasAuthority() && GetNetMode() != NM_Standalone)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DoorOpenDirection, this);
	}
	
	K2_OnDoorOpenDirectionChanged(OldDoorOpenDirection, NewDoorOpenDirection);
}

// -------------------------------------------------------------
// Door Open Motion

void ADoor::SetDoorOpenMotion(EDoorMotion NewDoorOpenMotion)
{
	if (DoorOpenMotion != NewDoorOpenMotion && CanChangeDoorOpenMotion(NewDoorOpenMotion))
	{
		switch (DoorOpenMotionChangeType)
		{
		case EDoorChangeType::Wait:
			{
				PendingDoorOpenMotion = NewDoorOpenMotion;
				bHasPendingDoorOpenMotion = true;
			}
			break;
		case EDoorChangeType::Immediate:
			{
				const EDoorMotion OldDoorOpenMotion = DoorOpenMotion;
				DoorOpenMotion = NewDoorOpenMotion;
				OnDoorOpenMotionChanged(OldDoorOpenMotion, NewDoorOpenMotion);
			}
			break;
		default:
			break;
		}
	}
}

bool ADoor::CanChangeDoorOpenMotion_Implementation(EDoorMotion NewDoorOpenMotion) const
{
	switch (DoorOpenMotionChangeType)
	{
	case EDoorChangeType::Disabled: return false;
	case EDoorChangeType::Closed: return DoorState == EDoorState::Closed;
	case EDoorChangeType::Wait: return true;
	case EDoorChangeType::Immediate: return true;
	default: return false;
	}
}

void ADoor::OnDoorOpenMotionChanged(EDoorMotion OldDoorOpenMotion, EDoorMotion NewDoorOpenMotion)
{
	UE_LOG(LogDoors, Verbose, TEXT("%s OnDoorOpenMotionChanged: OldDoorOpenMotion: %s, NewDoorOpenMotion: %s"), *GetRoleString(),
		*UDoorStatics::DoorMotionToString(OldDoorOpenMotion), *UDoorStatics::DoorMotionToString(NewDoorOpenMotion));
	
	if (HasAuthority() && GetNetMode() != NM_Standalone)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DoorOpenMotion, this);
	}
	
	K2_OnDoorOpenMotionChanged(OldDoorOpenMotion, NewDoorOpenMotion);
}

void ADoor::OnStationaryCooldownFinished()
{
	// Clear the timer -- its still considered active on this frame which could cause other checks to fail
	GetWorldTimerManager().ClearTimer(StationaryCooldownTimerHandle);
	
	// Broadcast the delegate
	if (OnDoorStationaryCooldownFinishedDelegate.IsBound())
	{
		OnDoorStationaryCooldownFinishedDelegate.Broadcast(this);
	}

	// Broadcast the generic cooldown finished delegate
	if (OnDoorCooldownFinishedDelegate.IsBound() && !IsDoorOnMotionCooldown())
	{
		OnDoorCooldownFinishedDelegate.Broadcast(this);
	}
}

void ADoor::OnInMotionCooldownFinished()
{
	// Clear the timer -- its still considered active on this frame which could cause other checks to fail
	GetWorldTimerManager().ClearTimer(MotionCooldownTimerHandle);
	
	// Broadcast the delegate
	if (OnDoorInMotionCooldownFinishedDelegate.IsBound())
	{
		OnDoorInMotionCooldownFinishedDelegate.Broadcast(this);
	}

	// Broadcast the generic cooldown finished delegate
	if (OnDoorCooldownFinishedDelegate.IsBound() && !IsDoorOnStationaryCooldown())
	{
		OnDoorCooldownFinishedDelegate.Broadcast(this);
	}
}

float ADoor::GetRemainingCooldown() const
{
	if (!IsDoorOnCooldown())
	{
		return 0.f;
	}
	
	return FMath::Max(GetRemainingStationaryCooldown(), GetRemainingInMotionCooldown());
}

float ADoor::GetRemainingStationaryCooldown() const
{
	if (!IsDoorOnStationaryCooldown())
	{
		return 0.f;
	}
	return GetWorldTimerManager().GetTimerRemaining(StationaryCooldownTimerHandle);
}

float ADoor::GetRemainingInMotionCooldown() const
{
	if (!IsDoorOnMotionCooldown())
	{
		return 0.f;
	}
	return GetWorldTimerManager().GetTimerRemaining(MotionCooldownTimerHandle);
}

bool ADoor::IsDoorOnCooldown() const
{
	return IsDoorOnMotionCooldown() || IsDoorOnStationaryCooldown();
}

bool ADoor::IsDoorOnMotionCooldown() const
{
	return GetWorldTimerManager().IsTimerActive(MotionCooldownTimerHandle);
}

bool ADoor::IsDoorOnStationaryCooldown() const
{
	return GetWorldTimerManager().IsTimerActive(StationaryCooldownTimerHandle);
}

bool ADoor::ShouldAbilityRespondToDoorEvent(const AActor* Avatar, EDoorState ClientDoorState,
	EDoorDirection ClientDoorDirection, EDoorSide ClientDoorSide, EDoorState& NewDoorState,
	EDoorDirection& NewDoorDirection, EDoorMotion& DoorMotion, FGameplayTag& FailReason) const
{
	// Output a fail reason for UI to respond to, e.g. a locked icon
	FailReason = FGameplayTag::EmptyTag;
	
	// General optional override
	if (!CanDoorChangeToAnyState(Avatar))
	{
		FailReason = FDoorTags::Door_Fail_CanDoorChangeToAnyState;
		UE_LOG(LogDoors, Verbose, TEXT("%s ADoor::ShouldAbilityRespondToDoorEvent: CanDoorChangeToAnyState failed"), *GetRoleString());
		return false;
	}
	
	// Check if the door is on cooldown
	if (IsDoorOnCooldown())
	{
		FailReason = FDoorTags::Door_Fail_OnCooldown;
		UE_LOG(LogDoors, Verbose, TEXT("%s ADoor::ShouldAbilityRespondToDoorEvent: Door is on cooldown"), *GetRoleString());
		return false;
	}

	// Check if the door is in motion
	if (IsDoorInMotion() && !CanInteractWhileInMotion())
	{
		FailReason = FDoorTags::Door_Fail_InMotion;
		UE_LOG(LogDoors, Verbose, TEXT("%s ADoor::ShouldAbilityRespondToDoorEvent: Door is in motion"), *GetRoleString());
		return false;
	}

	// Determine which side of the door the avatar is on
	const EDoorSide CurrentDoorSide = UDoorStatics::GetDoorSide(Avatar, this);

	// Check if we can use the client's door side
	if (ClientDoorSide != CurrentDoorSide && !bTrustClientDoorSide)
	{
		FailReason = FDoorTags::Door_Fail_ClientDoorSide;
		UE_LOG(LogDoors, Verbose, TEXT("%s ADoor::ShouldAbilityRespondToDoorEvent: Client door side is not trusted and does not match"), *GetRoleString());
		return false;
	}

	// Check if the client can interact with the door
	if (!UDoorStatics::ProgressDoorState(this, ClientDoorState, ClientDoorDirection,
		ClientDoorSide, NewDoorState, NewDoorDirection, DoorMotion, FailReason))
	{
		return false;
	}

	// General optional override
	if (!CanChangeDoorState(Avatar, DoorState, NewDoorState, DoorDirection, NewDoorDirection))
	{
		FailReason = FDoorTags::Door_Fail_CanChangeDoorState;
		UE_LOG(LogDoors, Verbose, TEXT("%s ADoor::ShouldAbilityRespondToDoorEvent: CanChangeDoorState failed"), *GetRoleString());
		return false;
	}

	return true;
}

EDoorSide ADoor::GetDoorSide(const AActor* Avatar) const
{
	return UDoorStatics::GetDoorSide(Avatar, this);
}

FString ADoor::GetRoleString() const
{
	return UDoorStatics::GetRoleString(this);
}

#if WITH_EDITOR

void ADoor::HandleDoorPropertyChange()
{
	// Make sure we initialize the replicated property based on the default state
	RepDoorState = UDoorStatics::PackDoorState(DoorState, DoorDirection);
}

void ADoor::PostLoad()
{
	Super::PostLoad();
	
	HandleDoorPropertyChange();
}

void ADoor::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName& PropertyName = PropertyChangedEvent.GetMemberPropertyName();

	// Door state or direction
	if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, DoorState)) ||
		PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, DoorDirection)))
	{
		HandleDoorPropertyChange();
	}

	// Sort notifies
	else if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, OpenOutwardNotifies)))
	{
		OpenOutwardNotifies.Sort();
	}
	else if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, OpenInwardNotifies)))
	{
		OpenInwardNotifies.Sort();
	}
	else if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, CloseOutwardNotifies)))
	{
		CloseOutwardNotifies.Sort();
	}
	else if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, CloseInwardNotifies)))
	{
		CloseInwardNotifies.Sort();
	}
	
#if WITH_EDITORONLY_DATA
	if (GetWorld() && GetWorld()->IsPreviewWorld())
	{
		if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, bPlayAnimationPreview)) ||
			PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, PreviewState)) ||
			PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, PreviewDirection)) ||
			PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, PreviewSimulationRate)))
		{
			ClearPreviewAnimation();
			
			if (bPlayAnimationPreview)
			{
				StartPreviewAnimation(false);
			}
		}
	}
#endif
}

void ADoor::PostCDOCompiled(const FPostCDOCompiledContext& Context)
{
	Super::PostCDOCompiled(Context);

#if WITH_EDITORONLY_DATA
	bPlayAnimationPreview = false;
	ClearPreviewAnimation();
#endif
}

#endif
