// Copyright (c) Jared Taylor


#include "DoorStatics.h"

#include "Door.h"
#include "DoorTags.h"
#include "Abilities/GameplayAbilityTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DoorStatics)


uint8 UDoorStatics::PackDoorState(EDoorState DoorState, EDoorDirection DoorDirection)
{
	return (static_cast<uint8>(DoorState) & 0x3)
		 | ((static_cast<uint8>(DoorDirection) & 0x1) << 2);
}

void UDoorStatics::UnpackDoorState(uint8 Packed, EDoorState& OutState, EDoorDirection& OutDirection)
{
	OutState = static_cast<EDoorState>(Packed & 0x3);
	OutDirection = static_cast<EDoorDirection>((Packed >> 2) & 0x1);
}

uint8 UDoorStatics::PackTargetDataDoorState(EDoorState DoorState, EDoorDirection DoorDirection, EDoorSide DoorSide)
{
	return (static_cast<uint8>(DoorState) & 0x3)
		 | ((static_cast<uint8>(DoorDirection) & 0x1) << 2)
		 | ((static_cast<uint8>(DoorSide) & 0x1) << 3);
}

void UDoorStatics::UnpackTargetDataDoorState(uint8 DoorStatePacked, EDoorState& OutDoorState, EDoorDirection& OutDoorDirection, EDoorSide& OutDoorSide)
{
	OutDoorState = static_cast<EDoorState>(DoorStatePacked & 0x3);
	OutDoorDirection = static_cast<EDoorDirection>((DoorStatePacked >> 2) & 0x1);
	OutDoorSide = static_cast<EDoorSide>((DoorStatePacked >> 3) & 0x1);
}

void UDoorStatics::GetDoorFromAbilityActivationTargetData(
	const FGameplayEventData& EventData, EDoorValid& Validate, EDoorState& DoorState, EDoorDirection& DoorDirection,
	EDoorSide& DoorSide)
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
				UnpackTargetDataDoorState(DoorData->PackedState, DoorState, DoorDirection, DoorSide);
			}
		}
	}
}

bool UDoorStatics::ProgressDoorState(const ADoor* Door, EDoorState DoorState, EDoorDirection DoorDirection,
	EDoorSide DoorSide, EDoorState& NewDoorState, EDoorDirection& NewDoorDirection, EDoorMotion& Motion, FGameplayTag& FailReason)
{
	NewDoorState = Door->GetDoorState();
	NewDoorDirection = Door->GetDoorDirection();

	// Door is not valid
	if (!IsValid(Door))
	{
		FailReason = FDoorTags::Door_Fail_DoorNotValid;
		UE_LOG(LogDoors, Verbose, TEXT("UDoorStatics::ProgressDoorState: Door is not valid"));
		return false;
	}
	
	// Door is locked
	if (Door->GetDoorOpenDirection() == EDoorOpenDirection::Locked)
	{
		FailReason = FDoorTags::Door_Fail_Locked;
		UE_LOG(LogDoors, Verbose, TEXT("%s UDoorStatics::ProgressDoorState: Door is locked"), *GetRoleString(Door));
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
			FailReason = FDoorTags::Door_Fail_AlreadyClosed;
			UE_LOG(LogDoors, Verbose, TEXT("%s UDoorStatics::ProgressDoorState: Door is already closed"), *GetRoleString(Door));
			return false;
		}
		break;
	case EDoorInteraction::Open:
		if (Door->IsDoorOpenOrOpening())
		{
			FailReason = FDoorTags::Door_Fail_AlreadyOpen;
			UE_LOG(LogDoors, Verbose, TEXT("%s UDoorStatics::ProgressDoorState: Door is already open"), *GetRoleString(Door));
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
				FailReason = FDoorTags::Door_Fail_NoAccessFromFront;
				UE_LOG(LogDoors, Verbose, TEXT("%s UDoorStatics::ProgressDoorState: Door access is behind, but we're in front"), *GetRoleString(Door));
				return false;
			}
			break;
		case EDoorAccess::Front:
			// We can only interact from the front -- are we in front of the door?
			if (DoorSide != EDoorSide::Front)
			{
				FailReason = FDoorTags::Door_Fail_NoAccessFromBack;
				UE_LOG(LogDoors, Verbose, TEXT("%s UDoorStatics::ProgressDoorState: Door access is front, but we're behind"), *GetRoleString(Door));
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

		UE_LOG(LogDoors, Verbose, TEXT("%s UDoorStatics::ProgressDoorState: Closing door %s from %s with %s"), *GetRoleString(Door),
			*DoorDirectionToString(NewDoorDirection), *DoorSideToString(DoorSide), *DoorMotionToString(Motion));
		
		break;
	case EDoorInteraction::Open:
		// We are opening the door
		NewDoorState = EDoorState::Opening;

		// Let's try to open it the way we'd prefer
		Motion = Door->GetDoorOpenMotion();

		// But what if we can't push or can't pull?
		switch (Door->GetDoorOpenDirection())
		{
		case EDoorOpenDirection::Bidirectional:
			break;  // Nothing needs to change
		case EDoorOpenDirection::Outward:
			// Door only opens outward, if we're in front of the door, we can only pull, and if behind, only push
			Motion = DoorSide == EDoorSide::Front ? EDoorMotion::Pull : EDoorMotion::Push;
			break;
		case EDoorOpenDirection::Inward:
			// Door only opens inward, if we're in front of the door, we can only push, and if behind, only pull
			Motion = DoorSide == EDoorSide::Front ? EDoorMotion::Push : EDoorMotion::Pull;
			break;
		case EDoorOpenDirection::Locked:
			ensure(false);  // Handled
			break;
		}

		// If standing in front of the door, and we're pushing, we want to push it inward
		// If standing behind the door, and we're pulling, we want to pull it inward
		if (DoorSide == EDoorSide::Front)
		{
			NewDoorDirection = Motion == EDoorMotion::Push ? EDoorDirection::Inward : EDoorDirection::Outward;
		}
		else
		{
			NewDoorDirection = Motion == EDoorMotion::Pull ? EDoorDirection::Inward : EDoorDirection::Outward;
		}

		UE_LOG(LogDoors, Verbose, TEXT("%s UDoorStatics::ProgressDoorState: Opening door %s from %s with %s"), *GetRoleString(Door),
			*DoorDirectionToString(NewDoorDirection), *DoorSideToString(DoorSide), *DoorMotionToString(Motion));
		
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

FString UDoorStatics::DoorAccessToString(EDoorAccess Access)
{
	switch (Access)
	{
	case EDoorAccess::Bidirectional: return TEXT("Bidirectional");
	case EDoorAccess::Behind: return TEXT("Behind");
	case EDoorAccess::Front: return TEXT("Front");
	default: return TEXT("Unknown");
	}
}

FString UDoorStatics::DoorOpenDirectionToString(EDoorOpenDirection Direction)
{
	switch (Direction)
	{
	case EDoorOpenDirection::Bidirectional: return TEXT("Bidirectional");
	case EDoorOpenDirection::Inward: return TEXT("Inward");
	case EDoorOpenDirection::Outward: return TEXT("Outward");
	case EDoorOpenDirection::Locked: return TEXT("Locked");
	default: return TEXT("Unknown");
	}
}

FString UDoorStatics::DoorMotionToString(EDoorMotion Motion)
{
	switch (Motion)
	{
	case EDoorMotion::Push: return TEXT("Push");
	case EDoorMotion::Pull: return TEXT("Pull");
	default: return TEXT("Unknown");
	}
}

FString UDoorStatics::GetRoleString(const AActor* Actor)
{
	if (Actor->GetNetMode() == NM_Standalone || Actor->GetNetMode() == NM_MAX)
	{
		return TEXT("");
	}
	return Actor->HasAuthority() ? TEXT("[ Auth ]") : TEXT("[ Client ]");
}