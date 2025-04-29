// Copyright (c) Jared Taylor


#include "Visualizers/DoorSpriteWidget.h"

#include "Door.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DoorSpriteWidget)


#if WITH_EDITORONLY_DATA
void UDoorSpriteWidget::OnDoorInitialized(ADoor* Door)
{
	if (IsValid(Door))
	{
		if (Door->OnDoorStateChangedDelegate.IsAlreadyBound(this, &ThisClass::OnDoorStateChanged))
		{
			Door->OnDoorStateChangedDelegate.RemoveDynamic(this, &ThisClass::OnDoorStateChanged);
		}
		
		Door->OnDoorStateChangedDelegate.AddDynamic(this, &ThisClass::OnDoorStateChanged);
		OnDoorStateChanged(Door, Door->GetDoorState(), Door->GetDoorState(),
			Door->GetDoorDirection(), Door->GetDoorDirection());

		K2_OnDoorInitialized(Door, Door->GetNetMode() == NM_Client);
	}
}
#endif