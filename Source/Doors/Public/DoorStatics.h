// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "DoorTypes.h"

#include "DoorStatics.generated.h"

class ADoor;

/**
 * Helper functions for Doors
 */
UCLASS()
class DOORS_API UDoorStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Pack the door state and door direction into a single uint8 for replication */
	static EReplicatedDoorState PackDoorState(EDoorState DoorState, EDoorDirection DoorDirection);
	
	/** Unpack the door state and door direction from a single uint8 from replication */
	static void UnpackDoorState(EReplicatedDoorState DoorStatePacked, EDoorState& OutDoorState, EDoorDirection& OutDoorDirection);

	/**
	 * Based on the current state of the door, if we interact, then we're requesting it to go into a new state
	 * Determine what the new state is based on the current state, and which side we want the door to open towards
	 * @param Door The door to check
	 * @param DoorState The current state of the door
	 * @param DoorDirection The current direction of the door
	 * @param AvatarDoorSide The side of the door that we are standing on
	 * @param NewDoorState The new resulting state of the door
	 * @param NewDoorDirection The new resulting direction of the door
	 * @param Motion The motion we want to use to interact with the door
	 * @return True if we have a valid state the door can change to
	 */
	UFUNCTION(BlueprintCallable, Category=Door)
	static bool ProgressDoorState(const ADoor* Door, EDoorState DoorState, EDoorDirection DoorDirection,
		EDoorSide AvatarDoorSide, EDoorState& NewDoorState, EDoorDirection& NewDoorDirection, EDoorMotion& Motion);

	/** 
	 * Get the target door state based on the current state of the door, when we want to interact with it
	 * @param FromState The current state of the door
	 * @return The target state of the door
	 */
	UFUNCTION(BlueprintCallable, Category=Door)
	static EDoorInteraction GetDoorInteractionFromState(EDoorState FromState);

	/**
	 * Get the door side based on the avatar's location and the door's location
	 * If the door is rotated -90 yaw to point down Unreal forward axis use EAxis::X (get forward vector)
	 * Defaulting to EAxis::Y because designers typically don't bother rotating things (get right vector)
	 */
	UFUNCTION(BlueprintCallable, Category=Door)
	static EDoorSide GetDoorSide(const AActor* Avatar, const ADoor* Door, TEnumAsByte<EAxis::Type> DoorForwardAxis = EAxis::Y);

	UFUNCTION(BlueprintPure, Category=Door)
	static FString DoorStateToString(EDoorState State);

	UFUNCTION(BlueprintPure, Category=Door)
	static FString DoorDirectionToString(EDoorDirection Direction);
	
	UFUNCTION(BlueprintPure, Category=Door)
	static FString DoorSideToString(EDoorSide Side);

	UFUNCTION(BlueprintPure, Category=Door)
	static FString DoorStateDirectionToString(EDoorState State, EDoorDirection Direction);

	UFUNCTION(BlueprintPure, Category=Door)
	static FString DoorStateSideToString(EDoorState State, EDoorSide Side);
};
