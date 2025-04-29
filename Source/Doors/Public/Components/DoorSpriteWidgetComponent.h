// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "DoorTypes.h"
#include "Components/WidgetComponent.h"
#include "DoorSpriteWidgetComponent.generated.h"


class UDoorSpriteWidget;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DOORS_API UDoorSpriteWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UDoorSpriteWidget> DoorWidget;
	
	virtual void InitWidget() override;
	
	void OnRepDoorStateChanged(EReplicatedDoorState RepDoorState) const;
};
