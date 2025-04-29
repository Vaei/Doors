// Copyright (c) Jared Taylor


#include "DoorStatics.h"

#include "Door.h"
#include "Abilities/GameplayAbilityTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DoorStatics)


EReplicatedDoorState UDoorStatics::PackDoorState(EDoorState DoorState, EDoorDirection DoorDirection)
{
	switch (DoorState)
	{
	case EDoorState::Closed:
		return DoorDirection == EDoorDirection::Outward ? EReplicatedDoorState::ClosedOutward : EReplicatedDoorState::ClosedInward;
	case EDoorState::Opening:
		return DoorDirection == EDoorDirection::Outward ? EReplicatedDoorState::OpeningOutward : EReplicatedDoorState::OpeningInward;
	case EDoorState::Open:
		return DoorDirection == EDoorDirection::Outward ? EReplicatedDoorState::OpenOutward : EReplicatedDoorState::OpenInward;
	case EDoorState::Closing:
		return DoorDirection == EDoorDirection::Outward ? EReplicatedDoorState::ClosingOutward : EReplicatedDoorState::ClosingInward;
	default:
		ensure(false);
		return EReplicatedDoorState::ClosedOutward;
	}
}

void UDoorStatics::UnpackDoorState(EReplicatedDoorState DoorStatePacked, EDoorState& OutDoorState, EDoorDirection& OutDoorDirection)
{
	switch (DoorStatePacked)
	{
	case EReplicatedDoorState::ClosedOutward:
		OutDoorState = EDoorState::Closed;
		OutDoorDirection = EDoorDirection::Outward;
		break;
	case EReplicatedDoorState::ClosedInward:
		OutDoorState = EDoorState::Closed;
		OutDoorDirection = EDoorDirection::Inward;
		break;
	case EReplicatedDoorState::OpeningOutward:
		OutDoorState = EDoorState::Opening;
		OutDoorDirection = EDoorDirection::Outward;
		break;
	case EReplicatedDoorState::OpeningInward:
		OutDoorState = EDoorState::Opening;
		OutDoorDirection = EDoorDirection::Inward;
		break;
	case EReplicatedDoorState::OpenOutward:
		OutDoorState = EDoorState::Open;
		OutDoorDirection = EDoorDirection::Outward;
		break;
	case EReplicatedDoorState::OpenInward:
		OutDoorState = EDoorState::Open;
		OutDoorDirection = EDoorDirection::Inward;
		break;
	case EReplicatedDoorState::ClosingOutward:
		OutDoorState = EDoorState::Closing;
		OutDoorDirection = EDoorDirection::Outward;
		break;
	case EReplicatedDoorState::ClosingInward:
		OutDoorState = EDoorState::Closing;
		OutDoorDirection = EDoorDirection::Inward;
		break;
	default:
		ensure(false);
		OutDoorState = EDoorState::Closed;
		OutDoorDirection = EDoorDirection::Outward;
		break;
  }
}

void UDoorStatics::GetDoorFromAbilityActivationTargetData(
	const FGameplayEventData& EventData, EDoorValid& Validate, EDoorState& DoorState, EDoorDirection& DoorDirection, EDoorSide
	& DoorSide)
{
	Validate = EDoorValid::NotValid;
	for (const TSharedPtr<FGameplayAbilityTargetData>& Data : EventData.TargetData.Data)
	{
		if (Data.IsValid())
		{
			if (Data->GetScriptStruct() == FDoorAbilityTargetData::StaticStruct())
			{
				Validate = EDoorValid::Valid;
				const FDoorAbilityTargetData* DoorData = static_cast<FDoorAbilityTargetData*>(Data.Get());
				DoorSide = DoorData->DoorSide;
				UnpackDoorState(DoorData->DoorState, DoorState, DoorDirection);
			}
		}
	}
}

bool UDoorStatics::ProgressDoorState(const ADoor* Door, EDoorState DoorState, EDoorDirection DoorDirection,
	EDoorSide DoorSide, EDoorState& NewDoorState, EDoorDirection& NewDoorDirection, EDoorMotion& Motion)
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

	// Do we have the required access to open the door?
	if (Interaction == EDoorInteraction::Open)
	{
		switch (Door->GetDoorAccess())
		{
		case EDoorAccess::Bidirectional:
			// We can interact from either side
			break;
		case EDoorAccess::Behind:
			// We can only interact from the back -- are we behind the door?
			if (DoorSide != EDoorSide::Back)
			{
				return false;
			}
			break;
		case EDoorAccess::Front:
			// We can only interact from the front -- are we in front of the door?
			if (DoorSide != EDoorSide::Front)
			{
				return false;
			}
			break;
		}
	}

	// Determine next door state
	// Determine what motion we use to interact with the door, do we push or pull it
	switch (Interaction)
	{
	case EDoorInteraction::Close:
		// We are closing the door
		NewDoorState = EDoorState::Closing;
		
		// We will push the door if we're in front of it, and pull it if we're behind it
		Motion = DoorSide == EDoorSide::Front ? EDoorMotion::Push : EDoorMotion::Pull;
		
		// If the door is open, and we're closing the door, we close it based on the direction it was open in
		// So we don't do anything here
		// ---NewDoorDirection = NewDoorDirection;

		break;
	case EDoorInteraction::Open:
		// We are opening the door
		NewDoorState = EDoorState::Opening;

		// Let's try to open it the way we'd prefer
		Motion = Door->GetDoorOpenMotion();

		// But what if we can't push or can't pull?

		// We're trying to push, we're at the front of the door, but it can't open inward
		if (Motion == EDoorMotion::Push && DoorSide == EDoorSide::Front && Door->GetDoorOpenDirection() == EDoorOpenDirection::Inward)
		{
			Motion = EDoorMotion::Pull;
		}

		// We're trying to push, we're behind the door, but it can't open outward
		else if (Motion == EDoorMotion::Push && DoorSide == EDoorSide::Back && Door->GetDoorOpenDirection() == EDoorOpenDirection::Outward)
		{
			Motion = EDoorMotion::Pull;
		}

		// We're trying to pull, we're at the front of the door, but it can't open outward
		else if (Motion == EDoorMotion::Pull && DoorSide == EDoorSide::Front && Door->GetDoorOpenDirection() == EDoorOpenDirection::Outward)
		{
			Motion = EDoorMotion::Push;
		}

		// We're trying to pull, we're behind the door, but it can't open inward
		else if (Motion == EDoorMotion::Pull && DoorSide == EDoorSide::Back && Door->GetDoorOpenDirection() == EDoorOpenDirection::Inward)
		{
			Motion = EDoorMotion::Push;
		}

		// If standing in front of the door, and we're pushing, we want to push it inward
		// If standing behind the door, and we're pulling, we want to pull it inward
		NewDoorDirection = (DoorSide == EDoorSide::Front && Motion == EDoorMotion::Push) ||
			(DoorSide == EDoorSide::Back && Motion == EDoorMotion::Pull) ? EDoorDirection::Inward : EDoorDirection::Outward;
		
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

EDoorSide UDoorStatics::GetDoorSide(const AActor* Avatar, const ADoor* Door)
{
	if (!IsValid(Avatar) || !IsValid(Door))
	{
		return EDoorSide::Front;
	}
	const FVector DoorLocation = Door->GetDoorLocation();
	const FVector AvatarLocation = Avatar->GetActorLocation();
	const FVector DoorVector = Door->GetDoorTransform().GetScaledAxis(EAxis::X);
	const FVector AvatarVector = (AvatarLocation - DoorLocation).GetSafeNormal2D();
	const float Dot = FVector::DotProduct(DoorVector, AvatarVector);
	return Dot >= 0.f ? EDoorSide::Front : EDoorSide::Back;
}

ADoor* UDoorStatics::GetOwningDoorFromComponent(const USceneComponent* Component)
{
	if (!IsValid(Component) || !Component->GetOwner())
	{
		return nullptr;
	}

	return Cast<ADoor>(Component->GetOwner());
}

ADoor* UDoorStatics::GetOwningDoorFromComponentChecked(const USceneComponent* Component, EDoorValid& Validate)
{
	Validate = EDoorValid::NotValid;
	if (ADoor* Door = GetOwningDoorFromComponent(Component))
	{
		Validate = EDoorValid::Valid;
		return Door;
	}
	return nullptr;
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
