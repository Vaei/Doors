// Copyright (c) Jared Taylor


#include "Filtering/DoorFilter_DoorState.h"

#include "Door.h"
#include "GraspComponent.h"
#include "GraspStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DoorFilter_DoorState)


UDoorFilter_DoorState::UDoorFilter_DoorState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

bool UDoorFilter_DoorState::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(DoorFilter_DoorState::ShouldFilterTarget);

	// Find the source actor
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext || !IsValid(SourceContext->SourceActor))
	{
		return true;
	}
	
	const UPrimitiveComponent* TargetComponent = TargetData.HitResult.GetComponent();
	
	const ADoor* Door = TargetComponent ? Cast<ADoor>(TargetComponent->GetOwner()) : nullptr;
	if (!Door)
	{
		return bFilterIfNotDoor;
	}

	if (IsValueFiltered(DoorStateFilterType, Door->GetDoorState(), DoorStates))
	{
		return true;
	}

	if (IsValueFiltered(DoorDirectionFilterType, Door->GetDoorDirection(), DoorDirections))
	{
		return true;
	}

	if (IsValueFiltered(DoorAccessFilterType, Door->GetDoorAccess(), DoorAccesses))
	{
		return true;
	}

	if (IsValueFiltered(DoorOpenDirectionFilterType, Door->GetDoorOpenDirection(), DoorOpenDirections))
	{
		return true;
	}

	return false;
}
