// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

/**
 * Draws editor visualizers for doors
 * This visualizes the door's capabilities
 */
class DOORSVISUALIZER_API FDoorVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View,
		FPrimitiveDrawInterface* PDI) override;
};
