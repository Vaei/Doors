// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

class ADoor;
/**
 * Draws editor visualizers for doors
 * This visualizes the door's capabilities
 */
class DOORSVISUALIZER_API FDoorVisualizer : public FComponentVisualizer
{
public:
	static UTexture2D* ClosedInwardSprite;
	static UTexture2D* ClosedOutwardSprite;
	static UTexture2D* OpenInwardSprite;
	static UTexture2D* OpenOutwardSprite;
	static UTexture2D* OpeningInwardSprite;
	static UTexture2D* OpeningOutwardSprite;
	static UTexture2D* ClosingInwardSprite;
	static UTexture2D* ClosingOutwardSprite;
	
public:
	virtual UTexture2D* GetDoorSprite(const ADoor* Door) const;
	
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View,
		FPrimitiveDrawInterface* PDI) override;
};
