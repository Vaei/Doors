// Copyright (c) Jared Taylor


#include "DoorVisualizer.h"

#include "Door.h"
#include "DoorEditorVisualizer.h"
#include "Materials/MaterialRenderProxy.h"


void FDoorVisualizer::DrawVisualization(const UActorComponent* InComponent, const FSceneView* View,
	FPrimitiveDrawInterface* PDI)
{
	const UDoorEditorVisualizer* Component = InComponent ? Cast<const UDoorEditorVisualizer>(InComponent) : nullptr;
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

	const ADoor* Door = Cast<ADoor>(Component->GetOwner());
	const FVector& Forward = Door->GetActorForwardVector();

	// Get the door's bounds
	const FBox Box = Door->GetComponentsBoundingBox();
	const FVector Extents = Box.GetExtent();
	const FVector Origin = Box.GetCenter();

	// DrawBox(PDI, Door->GetTransform().ToMatrixWithScale(), Extents, Proxy, 1);
	
	// Transform extents into actor space
	const FVector LocalX = Door->GetActorForwardVector();
	const FVector LocalY = Door->GetActorRightVector();
	const FVector LocalZ = Door->GetActorUpVector();

	const bool bFrontLocked = Door->GetDoorOpenDirection() != EDoorOpenDirection::Bidirectional && 
		Door->GetDoorOpenDirection() != EDoorOpenDirection::Outward;

	const bool bBackLocked = Door->GetDoorOpenDirection() != EDoorOpenDirection::Bidirectional && 
		Door->GetDoorOpenDirection() != EDoorOpenDirection::Inward;

	constexpr float Extension = 100.f;
	constexpr float LineThickness = 1.f;

	const FLinearColor FrontColor = bFrontLocked ? FColor::Orange.ReinterpretAsLinear() : FColor::Emerald.ReinterpretAsLinear();
	const FLinearColor BackColor = bBackLocked ? FColor::Orange.ReinterpretAsLinear() : FColor::Emerald.ReinterpretAsLinear();
	
	// Find the floor
	const FVector Floor = Origin - LocalZ * Extents.Z;

	// PDI->DrawPoint(FloorFront, FColor::Red, 30.f, SDPG_World);

	// Draw an arrow showing which way the door is oriented
	const FMatrix Matrix = FTranslationMatrix(Origin) * FRotationMatrix(Door->GetActorRotation());
	DrawFlatArrow(PDI, Floor, Matrix.GetScaledAxis(EAxis::X), Matrix.GetScaledAxis(EAxis::Y),
		FColor::Black, Extension * 1.f, Extension * 0.5f, Proxy, SDPG_World, 1.f);	

	// Determine the points in front of the door
	const FVector FloorFront = Floor + LocalX * Extents.X;
	const FVector FloorFrontLeft = FloorFront - LocalY * Extents.Y;
	const FVector FloorFrontRight = FloorFront + LocalY * Extents.Y;
	const FVector FloorExtendFrontLeft = FloorFrontLeft + Forward * Extension;
	const FVector FloorExtendFrontRight = FloorFrontRight + Forward * Extension;
	
	// Draw a box in front of the door
	PDI->DrawLine(FloorFrontLeft, FloorFrontRight, FrontColor, SDPG_World, LineThickness);
	PDI->DrawLine(FloorFrontLeft, FloorExtendFrontLeft, FrontColor, SDPG_World, LineThickness);
	PDI->DrawLine(FloorFrontRight, FloorExtendFrontRight, FrontColor, SDPG_World, LineThickness);
	PDI->DrawLine(FloorExtendFrontLeft, FloorExtendFrontRight, FrontColor, SDPG_World, LineThickness);

	// Draw segments horizontally through the box we just drew above
	constexpr int32 Segments = 5;
	for (int32 i = 1; i < Segments; ++i)
	{
		// The box above is on the ground, these must be on the ground too, as subdivisions of the box, we need to draw them 100cm to match the extension
		const FVector FloorLeft = FloorFrontLeft + LocalX * (Extension / (float)Segments) * i;
		const FVector FloorRight = FloorFrontRight + LocalX * (Extension / (float)Segments) * i;
		PDI->DrawLine(FloorLeft, FloorRight, FrontColor, SDPG_World, LineThickness);
	}

	// Determine the points behind the door
	const FVector FloorBack = Floor - LocalX * Extents.X;
	const FVector FloorBackLeft = FloorBack - LocalY * Extents.Y;
	const FVector FloorBackRight = FloorBack + LocalY * Extents.Y;
	const FVector FloorExtendBackLeft = FloorBackLeft + -Forward * Extension;
	const FVector FloorExtendBackRight = FloorBackRight + -Forward * Extension;

	// Draw a box at the back of the door floor
	PDI->DrawLine(FloorBackLeft, FloorBackRight, BackColor, SDPG_World, LineThickness);
	PDI->DrawLine(FloorBackLeft, FloorExtendBackLeft, BackColor, SDPG_World, LineThickness);
	PDI->DrawLine(FloorBackRight, FloorExtendBackRight, BackColor, SDPG_World, LineThickness);
	PDI->DrawLine(FloorExtendBackLeft, FloorExtendBackRight, BackColor, SDPG_World, LineThickness);

	// Draw segments horizontally through the box we just drew above
	for (int32 i = 1; i < Segments; ++i)
	{
		// The box above is on the ground, these must be on the ground too, as subdivisions of the box, we need to draw them 100cm to match the extension
		const FVector FloorLeft = FloorBackLeft + -LocalX * (Extension / (float)Segments) * i;
		const FVector FloorRight = FloorBackRight + -LocalX * (Extension / (float)Segments) * i;
		PDI->DrawLine(FloorLeft, FloorRight, BackColor, SDPG_World, LineThickness);
	}
}
