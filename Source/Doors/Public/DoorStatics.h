// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "DoorTypes.h"

#include "DoorStatics.generated.h"

struct FGameplayEventData;
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

	/** Pack the door state and door direction and door side into a single uint8 for replication */
	static ETargetDataDoorState PackTargetDataDoorState(EDoorState DoorState, EDoorDirection DoorDirection, EDoorSide DoorSide);
	
	/** Unpack the door state and door direction from a single uint8 from replication */
	static void UnpackTargetDataDoorState(ETargetDataDoorState DoorStatePacked, EDoorState& OutDoorState,
		EDoorDirection& OutDoorDirection, EDoorSide& OutDoorSide);
	
	/** Unpack any data sent from the gameplay ability event data payload */
	UFUNCTION(BlueprintCallable, Category=Door, meta=(ExpandEnumAsExecs="Validate"))
	static void GetDoorFromAbilityActivationTargetData(const FGameplayEventData& EventData, EDoorValid& Validate,
		EDoorState& DoorState, EDoorDirection& DoorDirection, EDoorSide& DoorSide);
	
	/**
	 * Based on the current state of the door, if we interact, then we're requesting it to go into a new state
	 * Determine what the new state is based on the current state, and which side we want the door to open towards
	 * @param Door The door to check
	 * @param DoorState The current state of the door
	 * @param DoorDirection The current direction of the door
	 * @param DoorSide The side of the door that we are standing on
	 * @param NewDoorState The new resulting state of the door
	 * @param NewDoorDirection The new resulting direction of the door
	 * @param Motion The motion we want to use to interact with the door
	 * @param FailReason The reason we failed to progress the door state
	 * @return True if we have a valid state the door can change to
	 */
	UFUNCTION(BlueprintCallable, Category=Door)
	static bool ProgressDoorState(const ADoor* Door, EDoorState DoorState, EDoorDirection DoorDirection,
		EDoorSide DoorSide, EDoorState& NewDoorState, EDoorDirection& NewDoorDirection, EDoorMotion& Motion, FGameplayTag& FailReason);

	/** 
	 * Get the target door state based on the current state of the door, when we want to interact with it
	 * @param FromState The current state of the door
	 * @return The target state of the door
	 */
	UFUNCTION(BlueprintCallable, Category=Door)
	static EDoorInteraction GetDoorInteractionFromState(EDoorState FromState);

	/**
	 * Get the door side based on the avatar's location and the door's location
	 */
	UFUNCTION(BlueprintCallable, Category=Door)
	static EDoorSide GetDoorSide(const AActor* Avatar, const ADoor* Door);

	/** Convenience function for passing an interactable component on the door, to retrieve and cast the door owner */
	UFUNCTION(BlueprintPure, Category=Door)
	static ADoor* GetOwningDoorFromComponent(const USceneComponent* Component);

	UFUNCTION(BlueprintCallable, Category=Door, meta=(ExpandEnumAsExecs="Validate"))
	static ADoor* GetOwningDoorFromComponentChecked(const USceneComponent* Component, EDoorValid& Validate);
	
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

	UFUNCTION(BlueprintPure, Category=Door)
	static FString DoorAccessToString(EDoorAccess Access);

	UFUNCTION(BlueprintPure, Category=Door)
	static FString DoorOpenDirectionToString(EDoorOpenDirection Direction);

	UFUNCTION(BlueprintPure, Category=Door)
	static FString DoorMotionToString(EDoorMotion Motion);

	static FString GetRoleString(const AActor* Actor);
};
