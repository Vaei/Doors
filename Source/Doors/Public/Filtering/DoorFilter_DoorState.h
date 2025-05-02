// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "DoorTypes.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "DoorFilter_DoorState.generated.h"

UENUM(BlueprintType)
enum class EDoorFilterType : uint8
{
	Ignore,
	Blacklist,
	Whitelist
};

/**
 * Filter targets based on their being a door and in a specific state
 */
UCLASS(Blueprintable, DisplayName="Door Filter (Door State)")
class DOORS_API UDoorFilter_DoorState : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

public:
	/** If its not a door at all, will be filtered out if true */
	UPROPERTY(EditAnywhere, Category="Door Filter")
	bool bFilterIfNotDoor = false;
	
	/** Whether to filter in or out */
	UPROPERTY(EditAnywhere, Category="Door Filter")
	EDoorFilterType DoorStateFilterType = EDoorFilterType::Ignore;

	/** Whether to filter in or out */
	UPROPERTY(EditAnywhere, Category="Door Filter")
	EDoorFilterType DoorDirectionFilterType = EDoorFilterType::Ignore;
	
	/** Whether to filter in or out */
	UPROPERTY(EditAnywhere, Category="Door Filter")
	EDoorFilterType DoorAccessFilterType = EDoorFilterType::Ignore;
		
	/** Whether to filter in or out */
	UPROPERTY(EditAnywhere, Category="Door Filter")
	EDoorFilterType DoorOpenDirectionFilterType = EDoorFilterType::Ignore;
	
	/** Door states to filter in or out */
	UPROPERTY(EditAnywhere, Category="Door Filter", meta=(EditCondition="DoorStateFilterType!=EDoorFilterType::Ignore", EditConditionHides))
	TArray<EDoorState> DoorStates = { EDoorState::Closed, EDoorState::Closing, EDoorState::Open, EDoorState::Opening };

	/** Door directions to filter in or out */
	UPROPERTY(EditAnywhere, Category="Door Filter", meta=(EditCondition="DoorDirectionFilterType!=EDoorFilterType::Ignore", EditConditionHides))
	TArray<EDoorDirection> DoorDirections = { EDoorDirection::Inward, EDoorDirection::Outward };

	/** Door access to filter in or out */
	UPROPERTY(EditAnywhere, Category="Door Filter", meta=(EditCondition="DoorAccessFilterType!=EDoorFilterType::Ignore", EditConditionHides))
	TArray<EDoorAccess> DoorAccesses = { EDoorAccess::Bidirectional, EDoorAccess::Behind, EDoorAccess::Front };

	/** Door access to filter in or out */
	UPROPERTY(EditAnywhere, Category="Door Filter", meta=(EditCondition="DoorOpenDirectionFilterType!=EDoorFilterType::Ignore", EditConditionHides))
	TArray<EDoorOpenDirection> DoorOpenDirections = { EDoorOpenDirection::Bidirectional, EDoorOpenDirection::Inward, EDoorOpenDirection::Outward, EDoorOpenDirection::Locked };
	
public:
	UDoorFilter_DoorState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Called against every target data to determine if the target should be filtered out */
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

protected:
	// Helper for filtering values based on filter type
	template<typename T>
	bool IsValueFiltered(EDoorFilterType FilterType, T Value, const TArray<T>& FilterList) const;
};

template <typename T>
bool UDoorFilter_DoorState::IsValueFiltered(EDoorFilterType FilterType, T Value, const TArray<T>& FilterList) const
{
	switch (FilterType)
	{
	case EDoorFilterType::Whitelist:
		return !FilterList.Contains(Value);
	case EDoorFilterType::Blacklist:
		return FilterList.Contains(Value);
	case EDoorFilterType::Ignore:
	default:
		return false;
	}
}
