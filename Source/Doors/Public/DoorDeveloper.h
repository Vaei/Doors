// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "DoorDeveloper.generated.h"

/**
 * 
 */
UCLASS(Config=EditorPerProjectUserSettings, meta=(DisplayName="Door Developer Settings"))
class DOORS_API UDoorDeveloper : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	/** If true, draw billboard sprites showing the door state during PIE */
	UPROPERTY(Config, EditAnywhere, Category=Door, meta=(ConsoleVariable="p.Door.ShowDoorStateDuringPIE",
		DisplayName="Show Door State During PIE", ToolTip="If true, draw billboard sprites showing the door state during PIE"))
	bool bShowDoorStateDuringPIE = true;
#endif
};
