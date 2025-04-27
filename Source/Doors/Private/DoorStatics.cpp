// Copyright (c) Jared Taylor


#include "DoorStatics.h"

#include "Door.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DoorStatics)


EReplicatedDoorState UDoorStatics::PackDoorState(EDoorState DoorState, EDoorDirection DoorDirection)
{
	switch (DoorState)
	{
	case EDoorState::Closed:
		return DoorDirection == EDoorDirection::Outward ? EReplicatedDoorState::ClosedFront : EReplicatedDoorState::ClosedBack;
	case EDoorState::Opening:
		return DoorDirection == EDoorDirection::Outward ? EReplicatedDoorState::OpeningFront : EReplicatedDoorState::OpeningBack;
	case EDoorState::Open:
		return DoorDirection == EDoorDirection::Outward ? EReplicatedDoorState::OpenFront : EReplicatedDoorState::OpenBack;
	case EDoorState::Closing:
		return DoorDirection == EDoorDirection::Outward ? EReplicatedDoorState::ClosingFront : EReplicatedDoorState::ClosingBack;
	default:
		ensure(false);
		return EReplicatedDoorState::ClosedFront;
	}
}

void UDoorStatics::UnpackDoorState(EReplicatedDoorState DoorStatePacked, EDoorState& OutDoorState, EDoorDirection& OutDoorDirection)
{
	switch (DoorStatePacked)
	{
	case EReplicatedDoorState::ClosedFront:
		OutDoorState = EDoorState::Closed;
		OutDoorDirection = EDoorDirection::Outward;
		break;
	case EReplicatedDoorState::ClosedBack:
		OutDoorState = EDoorState::Closed;
		OutDoorDirection = EDoorDirection::Inward;
		break;
	case EReplicatedDoorState::OpeningFront:
		OutDoorState = EDoorState::Opening;
		OutDoorDirection = EDoorDirection::Outward;
		break;
	case EReplicatedDoorState::OpeningBack:
		OutDoorState = EDoorState::Opening;
		OutDoorDirection = EDoorDirection::Inward;
		break;
	case EReplicatedDoorState::OpenFront:
		OutDoorState = EDoorState::Open;
		OutDoorDirection = EDoorDirection::Outward;
		break;
	case EReplicatedDoorState::OpenBack:
		OutDoorState = EDoorState::Open;
		OutDoorDirection = EDoorDirection::Inward;
		break;
	case EReplicatedDoorState::ClosingFront:
		OutDoorState = EDoorState::Closing;
		OutDoorDirection = EDoorDirection::Outward;
		break;
	case EReplicatedDoorState::ClosingBack:
		OutDoorState = EDoorState::Closing;
		OutDoorDirection = EDoorDirection::Inward;
	default:
		ensure(false);
		OutDoorState = EDoorState::Closed;
		OutDoorDirection = EDoorDirection::Outward;
		break;
  }
}

bool UDoorStatics::ProgressDoorState(const ADoor* Door, EDoorState DoorState, EDoorDirection DoorDirection,
	EDoorSide AvatarDoorSide, EDoorState& NewDoorState, EDoorDirection& NewDoorDirection, EDoorMotion& Motion)
{
	NewDoorState = Door->GetDoorState();
	NewDoorDirection = Door->GetDoorDirection();
	
	// Door is not valid or locked
	if (!IsValid(Door) || Door->GetDoorOpenDirection() == EDoorOpenDirection::Locked)
	{
		return false;
	}

	// Determine our desired interaction
	const EDoorInteraction Interaction = GetDoorInteractionFromState(DoorState);
	
	// Are we already in the desired state?
	switch (Interaction)
	{
	case EDoorInteraction::Close:
		if (Door->IsDoorClosedOrClosing())
		{
			return false;
		}
		break;
	case EDoorInteraction::Open:
		if (Door->IsDoorOpenOrOpening())
		{
			return false;
		}
		break;
	}

	// Can we interact with the door based on where our avatar is standing relative to the door?
	switch (Door->GetDoorAccess())
	{
	case EDoorAccess::Bidirectional:
		// We can interact from either side
		break;
	case EDoorAccess::Behind:
		// We can only interact from the back -- are we behind the door?
		if (AvatarDoorSide != EDoorSide::Back)
		{
			return false;
		}
		break;
	case EDoorAccess::Front:
		// We can only interact from the front -- are we in front of the door?
		if (AvatarDoorSide != EDoorSide::Front)
		{
			return false;
		}
		break;
	}

	// Determine next door state
	// Determine what motion we use to interact with the door, do we push or pull it
	switch (Interaction)
	{
	case EDoorInteraction::Close:
		// We are closing the door
		NewDoorState = EDoorState::Closing;

		// @TODO From which side
		NewDoorDirection = AvatarDoorSide == EDoorSide::Front ? EDoorDirection::Outward : EDoorDirection::Inward;
		
		// We will push the door if we're in front of it, and pull it if we're behind it
		Motion = AvatarDoorSide == EDoorSide::Front ? EDoorMotion::Push : EDoorMotion::Pull;
		break;
	case EDoorInteraction::Open:
		// We are opening the door
		NewDoorState = EDoorState::Opening;

		// @TODO From which side
		NewDoorDirection = AvatarDoorSide == EDoorSide::Front ? EDoorDirection::Outward : EDoorDirection::Inward;
		
		// Let's try to open it the way we'd prefer
		Motion = Door->GetDoorOpenMotion();

		// But what if we can't push or can't pull?

		// We're trying to push, we're at the front of the door, but it can't open inward
		if (Motion == EDoorMotion::Push && AvatarDoorSide == EDoorSide::Front && Door->GetDoorOpenDirection() == EDoorOpenDirection::Inward)
		{
			Motion = EDoorMotion::Pull;
		}

		// We're trying to push, we're behind the door, but it can't open outward
		else if (Motion == EDoorMotion::Push && AvatarDoorSide == EDoorSide::Back && Door->GetDoorOpenDirection() == EDoorOpenDirection::Outward)
		{
			Motion = EDoorMotion::Pull;
		}

		// We're trying to pull, we're at the front of the door, but it can't open outward
		else if (Motion == EDoorMotion::Pull && AvatarDoorSide == EDoorSide::Front && Door->GetDoorOpenDirection() == EDoorOpenDirection::Outward)
		{
			Motion = EDoorMotion::Push;
		}

		// We're trying to pull, we're behind the door, but it can't open inward
		else if (Motion == EDoorMotion::Pull && AvatarDoorSide == EDoorSide::Back && Door->GetDoorOpenDirection() == EDoorOpenDirection::Inward)
		{
			Motion = EDoorMotion::Push;
		}
		break;
	}

	return true;
}

EDoorInteraction UDoorStatics::GetDoorInteractionFromState(EDoorState FromState)
{
	switch (FromState)
	{
	case EDoorState::Closed: return EDoorInteraction::Open;
	case EDoorState::Closing: return EDoorInteraction::Open;
	case EDoorState::Opening: return EDoorInteraction::Close;
	case EDoorState::Open: return EDoorInteraction::Close;
	default:
		ensure(false);
		return EDoorInteraction::Close;
	}
}

EDoorSide UDoorStatics::GetDoorSide(const AActor* Avatar, const ADoor* Door,
	TEnumAsByte<EAxis::Type> DoorForwardAxis)
{
	if (!IsValid(Avatar) || !IsValid(Door))
	{
		return EDoorSide::Front;
	}
	const FVector DoorLocation = Door->GetActorLocation();
	const FVector AvatarLocation = Avatar->GetActorLocation();
	const FVector DoorVector = Door->GetActorTransform().GetScaledAxis(DoorForwardAxis);
	const FVector AvatarVector = (AvatarLocation - DoorLocation).GetSafeNormal2D();
	const float Dot = FVector::DotProduct(DoorVector, AvatarVector);
	return Dot >= 0.f ? EDoorSide::Front : EDoorSide::Back;
}

FString UDoorStatics::DoorStateToString(EDoorState State)
{
	switch (State)
	{
	case EDoorState::Closed: return TEXT("Closed");
	case EDoorState::Opening: return TEXT("Opening");
	case EDoorState::Open: return TEXT("Open");
	case EDoorState::Closing: return TEXT("Closing");
	default: return TEXT("Unknown");
	}
}

FString UDoorStatics::DoorDirectionToString(EDoorDirection Direction)
{
	switch (Direction)
	{
	case EDoorDirection::Outward: return TEXT("Outward");
	case EDoorDirection::Inward: return TEXT("Inward");
	default: return TEXT("Unknown");
	}
}

FString UDoorStatics::DoorSideToString(EDoorSide Side)
{
	switch (Side)
	{
	case EDoorSide::Front: return TEXT("Front");
	case EDoorSide::Back: return TEXT("Back");
	default: return TEXT("Unknown");
	}
}

FString UDoorStatics::DoorStateDirectionToString(EDoorState State, EDoorDirection Direction)
{
	switch (State)
	{
	case EDoorState::Closed:
		{
			switch (Direction)
			{
			case EDoorDirection::Outward: return TEXT("Closed-Outward");
			case EDoorDirection::Inward: return TEXT("Closed-Inward");
			}
		}
		break;
	case EDoorState::Opening:
		{
			switch (Direction)
			{
			case EDoorDirection::Outward: return TEXT("Opening-Outward");
			case EDoorDirection::Inward: return TEXT("Opening-Inward");
			}
		}
		break;
	case EDoorState::Open: 
		{
			switch (Direction)
			{
			case EDoorDirection::Outward: return TEXT("Open-Outward");
			case EDoorDirection::Inward: return TEXT("Open-Inward");
			}
		}
		break;
	case EDoorState::Closing: 
		{
			switch (Direction)
			{
			case EDoorDirection::Outward: return TEXT("Closing-Outward");
			case EDoorDirection::Inward: return TEXT("Closing-Inward");
			}
		}
		break;
	default: return TEXT("Unknown");
	}
	
	return TEXT("Unknown");
}

FString UDoorStatics::DoorStateSideToString(EDoorState State, EDoorSide Side)
{ 
	switch (State)
	{
	case EDoorState::Closed:
		{
			switch (Side)
			{
			case EDoorSide::Front: return TEXT("Closed-Front");
			case EDoorSide::Back: return TEXT("Closed-Back");
			}
		}
		break;
	case EDoorState::Opening:
		{
			switch (Side)
			{
			case EDoorSide::Front: return TEXT("Opening-Front");
			case EDoorSide::Back: return TEXT("Opening-Back");
			}
		}
		break;
	case EDoorState::Open: 
		{
			switch (Side)
			{
			case EDoorSide::Front: return TEXT("Open-Front");
			case EDoorSide::Back: return TEXT("Open-Back");
			}
		}
		break;
	case EDoorState::Closing: 
		{
			switch (Side)
			{
			case EDoorSide::Front: return TEXT("Closing-Front");
			case EDoorSide::Back: return TEXT("Closing-Back");
			}
		}
		break;
	default: return TEXT("Unknown");
	}
	
	return TEXT("Unknown");
}
