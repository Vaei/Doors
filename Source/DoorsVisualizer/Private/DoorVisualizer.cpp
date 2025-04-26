// Copyright (c) Jared Taylor


#include "DoorVisualizer.h"

#include "Door.h"
#include "Materials/MaterialRenderProxy.h"


void FDoorVisualizer::DrawVisualization(const UActorComponent* InComponent, const FSceneView* View,
	FPrimitiveDrawInterface* PDI)
{
	const UPrimitiveComponent* Component = InComponent ? Cast<const UPrimitiveComponent>(InComponent) : nullptr;
	if (!Component || !IsValid(Component->GetOwner()))
	{
		return;
	}

	const FColoredMaterialRenderProxy* Proxy = new FColoredMaterialRenderProxy(
		GEngine->WireframeMaterial ? GEditor->ConstraintLimitMaterialPrismatic->GetRenderProxy() : nullptr,
		FLinearColor(0.0f, 0.0f, 0.0f, 0.0f)
	);

	// Can't draw without a Proxy
	if (!Proxy)
	{
		return;
	}
}
