// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "DoorTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDoors, Log, All);

/** Current state of the door */
UENUM(BlueprintType)
enum class EDoorState : uint8
{
	Closed		UMETA(ToolTip="Door is fully closed (stationary)"),
	Closing		UMETA(ToolTip="Door is currently closing (in motion)"),
	Opening		UMETA(ToolTip="Door is currently opening (in motion)"),
	Open		UMETA(ToolTip="Door is fully open (stationary)"),
};

/**
 * Position the current state of the door entered from.
 * This can denote either the side of the door, or the side of the door that the
 * avatar was standing on when the door was interacted with.
 */
UENUM(BlueprintType)
enum class EDoorSide : uint8
{
	Front		UMETA(DisplayName="Front"),
	Back		UMETA(DisplayName="Back"),
};

/**
 * Packing and unpacking occurs pre-post-replication
 * This is difficult to visually parse, but we don't want to replicate two uint8 for no reason
 */
UENUM()
enum class EReplicatedDoorState : uint8
{
	ClosedFront,
	ClosedBack,
	OpeningFront,
	OpeningBack,
	OpenFront,
	OpenBack,
	ClosingFront,
	ClosingBack,
};

/**
 * Which way the door can open
 */
UENUM(BlueprintType)
enum class EDoorOpenDirection : uint8
{
	Bidirectional		UMETA(ToolTip="Door opens both ways"),
	Outward				UMETA(ToolTip="Door can only open outwards (to the front of the door)"),
	Inward				UMETA(ToolTip="Door can only open inwards (to the back of the door)"),
	Locked				UMETA(ToolTip="Door is locked and cannot be opened"),
};

/**
 * Which side the door can be opened from
 */
UENUM(BlueprintType)
enum class EDoorAccess : uint8
{
	Bidirectional		UMETA(ToolTip="Door can be opened from either side"),
	Behind				UMETA(ToolTip="Door can only be opened from behind the door"),
	Front				UMETA(ToolTip="Door can only be opened from in front of the door"),
};

/**
 * Whether we want to push the door or pull the door to open it
 */
UENUM(BlueprintType)
enum class EDoorMotion : uint8
{
	Push		UMETA(ToolTip="Push the door"),
	Pull		UMETA(ToolTip="Pull the door"),
};

/**
 * What action we want to take when interacting with the door
 */
UENUM(BlueprintType)
enum class EDoorInteraction : uint8
{
	Close		UMETA(ToolTip="We want to close the door"),
	Open		UMETA(ToolTip="We want to open the door"),
};

/**
 * How to handle changes to door parameters based on the door's current state
 */
UENUM(BlueprintType)
enum class EDoorChangeType : uint8
{
	Disabled			UMETA(ToolTip="Cannot be changed at all"),
	Closed				UMETA(ToolTip="Can only be changed when the door is closed"),
	Wait				UMETA(ToolTip="Can be changed at any time, if the door is not closed, will wait until it closes to change"),
	Immediate			UMETA(ToolTip="Can be changed at any time, regardless of the door's state"),
};

/**
 * We send the door's data to the ability from the client to the client's ability and from the client to the server's ability
 * This allows the client to request specific states rather than a generic interaction, which will fight latency esp. when other players are interacting
 */
USTRUCT(BlueprintType)
struct DOORS_API FDoorAbilityTargetData : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	FDoorAbilityTargetData(const EDoorState& InDoorState = EDoorState::Closed,
		const EDoorSide& InDoorSide = EDoorSide::Front);

	UPROPERTY(BlueprintReadOnly, Category=Character)
	EReplicatedDoorState DoorState;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << DoorState;
		return true;
	}
	
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}
};

template<>
struct TStructOpsTypeTraits<FDoorAbilityTargetData> : TStructOpsTypeTraitsBase2<FDoorAbilityTargetData>
{
	enum
	{
		WithNetSerializer = true	// For now this is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
	};
};