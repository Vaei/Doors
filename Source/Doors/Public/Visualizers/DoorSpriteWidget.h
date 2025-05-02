// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "DoorTypes.h"
#include "Blueprint/UserWidget.h"
#include "DoorSpriteWidget.generated.h"

class ADoor;

/**
 * 
 */
UCLASS()
class DOORS_API UDoorSpriteWidget : public UUserWidget
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
	void OnDoorInitialized(ADoor* Door);

	UFUNCTION(BlueprintImplementableEvent, Category=Door, meta=(DisplayName="On Door Initialized"))
	void K2_OnDoorInitialized(ADoor* Door, bool bShowReplicated);

	UFUNCTION(BlueprintImplementableEvent, Category=Door)
	void OnDoorStateChanged(const ADoor* Door, const EDoorState& OldDoorState, const EDoorState& NewDoorState,
		const EDoorDirection& OldDoorDirection, const EDoorDirection& NewDoorDirection);

	UFUNCTION(BlueprintImplementableEvent, Category=Door)
	void OnRepDoorStateChanged(EDoorState NewDoorState, EDoorDirection NewDoorDirection);
#endif
};
