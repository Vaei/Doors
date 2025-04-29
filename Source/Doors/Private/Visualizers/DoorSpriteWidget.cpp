// Copyright (c) Jared Taylor


#include "Visualizers/DoorSpriteWidget.h"

#include "Door.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DoorSpriteWidget)


void UDoorSpriteWidget::OnDoorInitialized(ADoor* Door)
{
	if (IsValid(Door))
	{
		Door->OnDoorStateChangedDelegate.AddDynamic(this, &ThisClass::OnDoorStateChanged);
		OnDoorStateChanged(Door, Door->GetDoorState(), Door->GetDoorState(),
			Door->GetDoorDirection(), Door->GetDoorDirection());
	}
}