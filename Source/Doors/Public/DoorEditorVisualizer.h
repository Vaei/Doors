// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DoorEditorVisualizer.generated.h"


/**
 * Door editor component
 * Registered for DoorVisualizer
 */
UCLASS(ClassGroup=(Custom))
class DOORS_API UDoorEditorVisualizer : public UActorComponent
{
	GENERATED_BODY()

public:
	UDoorEditorVisualizer()
	{
		bIsEditorOnly = true;
	}
};
