// Copyright (c) Jared Taylor


#include "DoorTypes.h"

#include "DoorStatics.h"


DEFINE_LOG_CATEGORY(LogDoors);

#include UE_INLINE_GENERATED_CPP_BY_NAME(DoorTypes)

FDoorAbilityTargetData::FDoorAbilityTargetData(const EDoorState& InDoorState, const EDoorDirection& InDoorDirection)
	: DoorState(UDoorStatics::PackDoorState(InDoorState, InDoorDirection))
{}
