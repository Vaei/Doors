// Copyright (c) Jared Taylor


#include "Door.h"

#include "DoorStatics.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Door)


ADoor::ADoor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	NetCullDistanceSquared = 25000000.0;  // 5000cm
	NetDormancy = DORM_Initial;
	SetReplicates(true);
}

void ADoor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	SharedParams.Condition = COND_SimulatedOnly;
	
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RepDoorState, SharedParams);
}

// -------------------------------------------------------------
// Door State

void ADoor::OnRep_DoorState()
{
	EDoorState NewDoorState;
	EDoorSide NewDoorSide;
	UDoorStatics::UnpackDoorState(RepDoorState, NewDoorState, NewDoorSide);
	SetDoorState(NewDoorState, NewDoorSide);
}

void ADoor::SetDoorState(EDoorState NewDoorState, EDoorSide NewDoorSide)
{
	if (DoorState != NewDoorState || DoorSide != NewDoorSide)
	{
		const EDoorState OldDoorState = DoorState;
		const EDoorSide OldDoorSide = DoorSide;
		DoorState = NewDoorState;
		DoorSide = NewDoorSide;
		OnDoorStateChanged(OldDoorState, NewDoorState, OldDoorSide, NewDoorSide);
	}
}

void ADoor::OnDoorStateChanged(EDoorState OldDoorState, EDoorState NewDoorState, EDoorSide OldDoorSide, EDoorSide NewDoorSide)
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
		OnDoorInMotionInterrupted(OldDoorState, NewDoorState, OldDoorSide, NewDoorSide);
	}

	// Replicate the door state to clients
	if (HasAuthority() && GetNetMode() != NM_Standalone)
	{
		RepDoorState = UDoorStatics::PackDoorState(DoorState, DoorSide);
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RepDoorState, this);
	}
	
	K2_OnDoorStateChanged(OldDoorState, NewDoorState, OldDoorSide, NewDoorSide);
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

bool ADoor::ShouldAbilityRespondToDoorEvent(const AActor* Avatar, EDoorState ClientDoorState, EDoorSide ClientDoorSide,
	EDoorSide CurrentDoorSide, EDoorState& NewDoorState, EDoorSide& NewDoorSide, EDoorMotion& DoorMotion) const
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

	// Check if we can use the client's door side
	if (ClientDoorSide != CurrentDoorSide && !bTrustClientDoorSide)
	{
		return false;
	}

	// Check if the client can interact with the door
	if (!UDoorStatics::ProgressDoorState(this, ClientDoorState, ClientDoorSide,
		NewDoorState, NewDoorSide, DoorMotion))
	{
		return false;
	}

	// General optional override
	if (!CanChangeDoorState(Avatar, DoorState, NewDoorState, DoorSide, NewDoorSide))
	{
		return false;
	}

	return true;
}

#if WITH_EDITOR

void ADoor::HandleDoorPropertyChange()
{
	// Make sure we initialize the replicated property based on the default state
	RepDoorState = UDoorStatics::PackDoorState(DoorState, DoorSide);
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
		PropertyName.IsEqual(GET_MEMBER_NAME_CHECKED(ThisClass, DoorSide)))
	{
		HandleDoorPropertyChange();
	}
}

#endif
