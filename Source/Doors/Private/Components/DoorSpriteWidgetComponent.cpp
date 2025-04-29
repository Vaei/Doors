// Copyright (c) Jared Taylor


#include "Components/DoorSpriteWidgetComponent.h"

#include "Door.h"
#include "Widgets/DoorSpriteWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DoorSpriteWidgetComponent)


void UDoorSpriteWidgetComponent::InitWidget()
{
	if (IsRunningDedicatedServer())
	{
		SetTickMode(ETickMode::Disabled);
		return;
	}
	
	Super::InitWidget();

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}

	DoorWidget = IsValid(GetWidget()) ? Cast<UDoorSpriteWidget>(GetWidget()) : nullptr;
	if (DoorWidget.IsValid())
	{
		if (ADoor* Door = Cast<ADoor>(GetOwner()))
		{
			DoorWidget->OnDoorInitialized(Door);
		}
	}
}

void UDoorSpriteWidgetComponent::OnRepDoorStateChanged(EReplicatedDoorState RepDoorState) const
{
	if (DoorWidget.IsValid())
	{
		DoorWidget->OnRepDoorStateChanged(RepDoorState);
	}
}
