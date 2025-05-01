// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "DoorTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDoors, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDoorCooldownFinished, const ADoor*, Door);

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
 * Direction the door entered it's current state from.
 */
UENUM(BlueprintType)
enum class EDoorDirection : uint8
{
	Outward		UMETA(ToolTip="Outward (to the front of the door / in front of the door)"),
	Inward		UMETA(ToolTip="Inward (to the back of the door / behind the door)"),
};

/**
 * Position of the avatar interacting with the door relative to the door
 */
UENUM(BlueprintType)
enum class EDoorSide : uint8
{
	Front		UMETA(ToolTip="Avatar was standing in front of the door while interacting"),
	Back		UMETA(TooLTip="Avatar was standing behind the door while interacting"),
};

/**
 * Packing and unpacking occurs pre-post-replication
 * This is difficult to visually parse, but we don't want to replicate two uint8 for no reason
 */
UENUM()
enum class EReplicatedDoorState : uint8
{
	ClosedOutward,
	ClosedInward,
	OpeningOutward,
	OpeningInward,
	OpenOutward,
	OpenInward,
	ClosingOutward,
	ClosingInward,
};

/**
 * Complete packing for target data including the door sides
 */
UENUM()
enum class ETargetDataDoorState : uint8
{
	ClosedOutwardFront,
	ClosedOutwardBack,
	ClosedInwardFront,
	ClosedInwardBack,
	OpeningOutwardFront,
	OpeningOutwardBack,
	OpeningInwardFront,
	OpeningInwardBack,
	OpenOutwardFront,
	OpenOutwardBack,
	OpenInwardFront,
	OpenInwardBack,
	ClosingOutwardFront,
	ClosingOutwardBack,
	ClosingInwardFront,
	ClosingInwardBack,
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
 * Determine how alpha updates based on the door state
 */
UENUM(BlueprintType)
enum class EAlphaMode : uint8
{
	Time				UMETA(ToolTip="Alpha is updated on tick over time"),
	InterpConstant		UMETA(ToolTip="Alpha is interpolated on tick to the target value at a constant rate"),
	InterpTo			UMETA(ToolTip="Alpha is interpolated on tick to the target value based on distance -- WARNING: Framerate dependent do not use if doors can collide with player characters!"),
	Disabled			UMETA(ToolTip="Alpha will not update on tick and must be handled manually. Door will not tick."),
};

UENUM(BlueprintType)
enum class EDoorValid : uint8
{
	Valid,
	NotValid,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnDoorStateChanged, const ADoor*, Door, const EDoorState&, OldState,
	const EDoorState&, NewState, const EDoorDirection&, OldDoorDirection, const EDoorDirection&, NewDoorDirection);

/**
 * We send the door's data to the ability from the client to the client's ability and from the client to the server's ability
 * This allows the client to request specific states rather than a generic interaction, which will fight latency esp. when other players are interacting
 */
USTRUCT(BlueprintType)
struct DOORS_API FDoorAbilityTargetData : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	FDoorAbilityTargetData()
		: PackedState(ETargetDataDoorState::ClosedOutwardFront)
	{}

	FDoorAbilityTargetData(const EDoorState& InDoorState, const EDoorDirection& InDoorDirection,
		const EDoorSide& InDoorSide);

	UPROPERTY(BlueprintReadOnly, Category=Character)
	ETargetDataDoorState PackedState;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << PackedState;
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