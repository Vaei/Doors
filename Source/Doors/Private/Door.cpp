// Copyright (c) Jared Taylor


#include "Door.h"

#include "DoorStatics.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#if WITH_EDITORONLY_DATA
#include "Components/DoorEditorVisualizer.h"
#include "Components/DoorSpriteWidgetComponent.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(Door)

namespace DoorCVars
{
#if WITH_EDITORONLY_DATA
	static bool bShowDoorStateDuringPIE = true;
	FAutoConsoleVariableRef CVarShowDoorStateDuringPIE(
		TEXT("p.Door.ShowDoorStateDuringPIE"),
		bShowDoorStateDuringPIE,
		TEXT("If true, draw billboard sprites showing the door state during PIE\n"),
		ECVF_Default);
#endif
}

TArray<FGameplayAbilityTargetData*> ADoor::GatherOptionalGraspTargetData(const FGameplayAbilityActorInfo* ActorInfo) const
{
	// Send data to interaction ability via Grasp
	if (const AActor* AvatarActor = ActorInfo && ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor.Get() : nullptr)
	{
		FDoorAbilityTargetData* DoorTargetData = new FDoorAbilityTargetData(GetDoorState(), GetDoorDirection(), GetDoorSide(AvatarActor));
		return { DoorTargetData };
	}
	return {};
}

ADoor::ADoor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	NetCullDistanceSquared = 25000000.0;  // 5000cm
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	
#if WITH_EDITORONLY_DATA
	// Draw editor visualization
	DoorVisualizer = CreateEditorOnlyDefaultSubobject<UDoorEditorVisualizer>(TEXT("DoorVisualizer"));

	// Draw PIE visualization
	DoorSprite = CreateEditorOnlyDefaultSubobject<UDoorSpriteWidgetComponent>(TEXT("DoorSprite"));
	DoorSprite->SetupAttachment(RootComponent);
#endif
}

void ADoor::BeginPlay()
{
	Super::BeginPlay();

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
	
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RepDoorState, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DoorAccess, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DoorOpenDirection, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DoorOpenMotion, SharedParams);
}

// -------------------------------------------------------------
// Door State

void ADoor::OnRep_DoorState()
{
	EDoorState NewDoorState;
	EDoorDirection NewDoorDirection;
	UDoorStatics::UnpackDoorState(RepDoorState, NewDoorState, NewDoorDirection);
	SetDoorState(NewDoorState, NewDoorDirection);

#if WITH_EDITORONLY_DATA
	if (GetNetMode() != NM_DedicatedServer && DoorCVars::bShowDoorStateDuringPIE)
	{
		DoorSprite->OnRepDoorStateChanged(RepDoorState);
	}
#endif
}

void ADoor::SetDoorState(EDoorState NewDoorState, EDoorDirection NewDoorDirection)
{
	if (DoorState != NewDoorState || DoorDirection != NewDoorDirection)
	{
		const EDoorState OldDoorState = DoorState;
		const EDoorDirection OldDoorDirection = DoorDirection;
		DoorState = NewDoorState;
		DoorDirection = NewDoorDirection;
		OnDoorStateChanged(OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection);
	}
}

void ADoor::OnDoorStateChanged(EDoorState OldDoorState, EDoorState NewDoorState, EDoorDirection OldDoorDirection,
	EDoorDirection NewDoorDirection)
{
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
	if (NewDoorState == EDoorState::Closed)
	{
		OnDoorFinishedClosing();
	}
	else if (NewDoorState == EDoorState::Open)
	{
		OnDoorFinishedOpening();
	}
	else if (NewDoorState == EDoorState::Opening)
	{
		OnDoorStartedOpening();
	}
	else if (NewDoorState == EDoorState::Closing)
	{
		OnDoorStartedClosing();
	}
	else if (IsDoorStateInMotion(OldDoorState))
	{
		OnDoorInMotionInterrupted(OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection);
	}

	// Replicate the door state to clients
	if (HasAuthority() && GetNetMode() != NM_Standalone)
	{
		RepDoorState = UDoorStatics::PackDoorState(DoorState, DoorDirection);
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RepDoorState, this);
	}

	// Blueprint callback
	K2_OnDoorStateChanged(OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection);

	// Delegate callback
	if (OnDoorStateChangedDelegate.IsBound())
	{
		OnDoorStateChangedDelegate.Broadcast(this, OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection);
	}

	// Cosmetic notifies for VFX/SFX
	if (GetNetMode() != NM_DedicatedServer)
	{
		K2_OnDoorStateChangedCosmetic(OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection);
	}

#if WITH_EDITORONLY_DATA
	if (GetNetMode() != NM_DedicatedServer && DoorCVars::bShowDoorStateDuringPIE)
	{
		DoorSprite->SetVisibility(true);
	}
#endif
}

void ADoor::OnDoorFinishedOpening()
{
	switch (DoorDirection)
	{
	case EDoorDirection::Outward:
		break;
	case EDoorDirection::Inward:
		break;
	}
	
	K2_OnDoorFinishedOpening();
	if (GetNetMode() != NM_DedicatedServer)
	{
		K2_OnDoorFinishedOpeningCosmetic();
	}
}

void ADoor::OnDoorFinishedClosing()
{
	K2_OnDoorFinishedClosing();
	if (GetNetMode() != NM_DedicatedServer)
	{
		K2_OnDoorFinishedClosingCosmetic();
	}
}

void ADoor::OnDoorStartedOpening()
{
	K2_OnDoorStartedOpening();
	if (GetNetMode() != NM_DedicatedServer)
	{
		K2_OnDoorStartedOpeningCosmetic();
	}
}

void ADoor::OnDoorStartedClosing()
{
	K2_OnDoorStartedClosing();
	if (GetNetMode() != NM_DedicatedServer)
	{
		K2_OnDoorStartedClosingCosmetic();
	}
}

void ADoor::OnDoorInMotionInterrupted(EDoorState OldDoorState, EDoorState NewDoorState, EDoorDirection OldDoorDirection,
	EDoorDirection NewDoorDirection)
{
	K2_OnDoorInMotionInterrupted(OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection);
	if (GetNetMode() != NM_DedicatedServer)
	{
		K2_OnDoorInMotionInterruptedCosmetic(OldDoorState, NewDoorState, OldDoorDirection, NewDoorDirection);
	}
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
		NewDoorAlpha = 0.f;
	}

	// Snap if nearly finished opening
	if (IsDoorOpenOrOpening() && FMath::IsNearlyEqual(FMath::Abs<float>(NewDoorAlpha), 1.f))
	{
		NewDoorAlpha = FMath::Sign(NewDoorAlpha);
	}

	// Update the door alpha
	DoorAlpha = NewDoorAlpha;

	OnDoorAlphaChanged(PrevDoorAlpha, NewDoorAlpha);
	return true;
}

void ADoor::OnDoorAlphaChanged(float OldDoorAlpha, float NewDoorAlpha)
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogDoors, VeryVerbose, TEXT("OnDoorAlphaChanged: OldDoorAlpha: %f, NewDoorAlpha: %f"),
		OldDoorAlpha, NewDoorAlpha);
#endif

	// Finalize the door state if required
	if (DoorState == EDoorState::Opening && FMath::IsNearlyEqual(GetDoorAlphaAbs(), 1.f))
	{
		SetDoorState(EDoorState::Open, DoorDirection);
	}
	else if (DoorState == EDoorState::Closing && FMath::IsNearlyZero(GetDoorAlphaAbs()))
	{
		SetDoorState(EDoorState::Closed, DoorDirection);
	}

	K2_OnDoorAlphaChanged(OldDoorAlpha, NewDoorAlpha);
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
	if (HasAuthority() && GetNetMode() != NM_Standalone)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DoorOpenMotion, this);
	}
	
	K2_OnDoorOpenMotionChanged(OldDoorOpenMotion, NewDoorOpenMotion);
}

void ADoor::StartInteractCooldown()
{
	LastInteractTime = GetWorld()->GetTimeSeconds();
}

bool ADoor::IsDoorOnInteractCooldown() const
{
	return LastInteractTime >= 0.f && GetWorld()->TimeSince(LastInteractTime) < InteractCooldown;
}

bool ADoor::IsDoorOnStationaryCooldown() const
{
	return LastStationaryTime >= 0.f && GetWorld()->TimeSince(LastStationaryTime) < StationaryCooldown;
}

bool ADoor::ShouldAbilityRespondToDoorEvent(const AActor* Avatar, EDoorState ClientDoorState,
	EDoorDirection ClientDoorDirection, EDoorSide ClientDoorSide, EDoorState& NewDoorState,
	EDoorDirection& NewDoorDirection, EDoorMotion& DoorMotion) const
{
	// General optional override
	if (!CanDoorChangeToAnyState(Avatar))
	{
		return false;
	}
	
	// Check if the door is on cooldown
	if (IsDoorOnCooldown())
	{
		return false;
	}

	// Check if the door is in motion
	if (IsDoorInMotion() && !CanInteractWhileInMotion())
	{
		return false;
	}

	// Determine which side of the door the avatar is on
	const EDoorSide CurrentDoorSide = UDoorStatics::GetDoorSide(Avatar, this);

	// Check if we can use the client's door side
	if (ClientDoorSide != CurrentDoorSide && !bTrustClientDoorSide)
	{
		return false;
	}

	// Check if the client can interact with the door
	if (!UDoorStatics::ProgressDoorState(this, ClientDoorState, ClientDoorDirection,
		ClientDoorSide, NewDoorState, NewDoorDirection, DoorMotion))
	{
		return false;
	}

	// General optional override
	if (!CanChangeDoorState(Avatar, DoorState, NewDoorState, DoorDirection, NewDoorDirection))
	{
		return false;
	}

	return true;
}

EDoorSide ADoor::GetDoorSide(const AActor* Avatar) const
{
	return UDoorStatics::GetDoorSide(Avatar, this);
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
	
	if (PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, DoorState)) ||
		PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, DoorDirection)))
	{
		HandleDoorPropertyChange();
	}
}

#endif
