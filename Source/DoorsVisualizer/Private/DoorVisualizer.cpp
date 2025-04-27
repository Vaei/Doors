// Copyright (c) Jared Taylor


#include "DoorVisualizer.h"

#include "Door.h"
#include "DoorEditorVisualizer.h"
#include "DoorStatics.h"

UTexture2D* FDoorVisualizer::ClosedOutwardSprite = nullptr;
UTexture2D* FDoorVisualizer::ClosedInwardSprite = nullptr;
UTexture2D* FDoorVisualizer::OpenOutwardSprite = nullptr;
UTexture2D* FDoorVisualizer::OpenInwardSprite = nullptr;
UTexture2D* FDoorVisualizer::OpeningOutwardSprite = nullptr;
UTexture2D* FDoorVisualizer::OpeningInwardSprite = nullptr;
UTexture2D* FDoorVisualizer::ClosingOutwardSprite = nullptr;
UTexture2D* FDoorVisualizer::ClosingInwardSprite = nullptr;

UTexture2D* FDoorVisualizer::GetDoorSprite(const ADoor* Door) const
{
	switch (Door->GetDoorState())
	{
	case EDoorState::Closed: return Door->GetDoorDirection() == EDoorDirection::Outward ? ClosedOutwardSprite : ClosedInwardSprite;
	case EDoorState::Closing: return Door->GetDoorDirection() == EDoorDirection::Outward ? ClosingOutwardSprite : ClosingInwardSprite;
	case EDoorState::Opening: return Door->GetDoorDirection() == EDoorDirection::Outward ? OpeningOutwardSprite : OpeningInwardSprite;
	case EDoorState::Open: return Door->GetDoorDirection() == EDoorDirection::Outward ? OpenOutwardSprite : OpenInwardSprite;
	default: return nullptr;
	}
}

void FDoorVisualizer::DrawVisualization(const UActorComponent* InComponent, const FSceneView* View,
	FPrimitiveDrawInterface* PDI)
{
	const UDoorEditorVisualizer* Component = InComponent ? Cast<const UDoorEditorVisualizer>(InComponent) : nullptr;
	if (!Component || !IsValid(Component->GetOwner()))
	{
		return;
	}

	if (!PDI || !PDI->View)
	{
		return;
	}

	// const FColoredMaterialRenderProxy* Proxy = new FColoredMaterialRenderProxy(
	// 	GEngine->WireframeMaterial ? GEditor->ConstraintLimitMaterialPrismatic->GetRenderProxy() : nullptr,
	// 	FLinearColor(0.0f, 0.0f, 0.0f, 0.0f)
	// );
	//
	// // Can't draw without a Proxy
	// if (!Proxy)
	// {
	// 	return;
	// }

	const ADoor* Door = Cast<ADoor>(Component->GetOwner());
	const FVector& Forward = Door->GetActorForwardVector();
	const FTransform Transform = Door->GetTransform();
	const FTransform TransformBackward = { (-Forward).ToOrientationQuat(), Transform.GetLocation(), Transform.GetScale3D() };

	// Get the door's bounds
	const FBox Box = Door->GetComponentsBoundingBox();
	const FVector Extents = Box.GetExtent();
	const FVector Origin = Box.GetCenter();
	
	// DrawBox(PDI, Door->GetTransform().ToMatrixWithScale(), Extents, Proxy, 1);
	
	// Transform extents into actor space
	const FVector LocalX = Door->GetActorForwardVector();
	const FVector LocalY = Door->GetActorRightVector();
	const FVector LocalZ = Door->GetActorUpVector();

	// Properties	
	constexpr float Extension = 100.f;
	constexpr float LineThickness = 1.f;

	// Find the floor
	const FVector Floor = Origin - LocalZ * Extents.Z;
	
	// Determine the points in front of the door
	const FVector FloorFront = Floor + LocalX * Extents.X;
	const FVector FloorExtendFront = FloorFront + Forward * Extension;
	const FVector FloorFrontLeft = FloorFront - LocalY * Extents.Y;
	const FVector FloorFrontRight = FloorFront + LocalY * Extents.Y;
	const FVector FloorExtendFrontLeft = FloorFrontLeft + Forward * Extension;
	const FVector FloorExtendFrontRight = FloorFrontRight + Forward * Extension;
	
	// Determine the points behind the door
	const FVector FloorBack = Floor - LocalX * Extents.X;
	const FVector FloorExtendBack = FloorBack - Forward * Extension;
	const FVector FloorBackLeft = FloorBack - LocalY * Extents.Y;
	const FVector FloorBackRight = FloorBack + LocalY * Extents.Y;
	const FVector FloorExtendBackLeft = FloorBackLeft + -Forward * Extension;
	const FVector FloorExtendBackRight = FloorBackRight + -Forward * Extension;

	// PDI->DrawPoint(FloorFront, FColor::Red, 30.f, SDPG_World);

	// Draw an arrow showing which way the door is oriented
	// const FMatrix Matrix = FTranslationMatrix(Origin) * FRotationMatrix(Door->GetActorRotation());
	// DrawFlatArrow(PDI, Floor, Matrix.GetScaledAxis(EAxis::X), Matrix.GetScaledAxis(EAxis::Y),
	// 	FColor::Yellow, Extension * 1.f, Extension * 0.5f, Proxy, SDPG_World, 1.f);	

	// Draw the door state
	const FString DoorStateInfo = UDoorStatics::DoorStateDirectionToString(Door->GetDoorState(), Door->GetDoorDirection());
	const FVector SpriteLocation = Origin + (LocalX * Extents.X * 2.f) + (LocalZ * Extents.Z * 0.25f);

	if (UTexture2D* DoorSprite = GetDoorSprite(Door))
	{
		if (DoorSprite->IsValidLowLevel() && DoorSprite->GetResource())
		{
			constexpr float SpriteSize = 12.8f;
			PDI->DrawSprite(
				SpriteLocation,
				SpriteSize,
				SpriteSize,
				DoorSprite->GetResource(),
				FLinearColor::White,
				SDPG_Foreground,
				0.f, 0.f, 0.f, 0.f
			);
		}
	}

	// Draw arrow showing the direction we prefer to open the door in

	// Front of door
	{
		const bool bMotionForward = Door->GetDoorOpenMotion() == EDoorMotion::Pull;
		const FVector MotionOrigin = Origin + LocalX * Extents.X * (2.5f + (bMotionForward ? 0.f : 1.f));
		const FTransform ExtentForwardTransform = FTransform(Transform.Rotator(), MotionOrigin);
		const FTransform ExtentBackwardTransform = FTransform(TransformBackward.Rotator(), MotionOrigin);
		const FMatrix ForwardArrowMatrix = bMotionForward ? ExtentForwardTransform.ToMatrixNoScale() : ExtentBackwardTransform.ToMatrixNoScale();
		DrawDirectionalArrow(PDI, ForwardArrowMatrix, FColor::Green, 15.f, 5.f, SDPG_Foreground, 1.f);
		DrawWireSphere(PDI, ForwardArrowMatrix.GetOrigin(), FColor::Green, 1.f, 16, SDPG_Foreground, 2.f);
	}

	// Back of door
	{
		const bool bMotionBackward = Door->GetDoorOpenMotion() != EDoorMotion::Pull;
		const FVector MotionOrigin = Origin - LocalX * Extents.X * (2.5f + (bMotionBackward ? 1.f : 0.f));
		const FTransform ExtentForwardTransform = FTransform(Transform.Rotator(), MotionOrigin);
		const FTransform ExtentBackwardTransform = FTransform(TransformBackward.Rotator(), MotionOrigin);
		const FMatrix BackwardArrowMatrix = bMotionBackward ? ExtentForwardTransform.ToMatrixNoScale() : ExtentBackwardTransform.ToMatrixNoScale();
		DrawDirectionalArrow(PDI, BackwardArrowMatrix, FColor::Green, 15.f, 5.f, SDPG_Foreground, 1.f);
		DrawWireSphere(PDI, BackwardArrowMatrix.GetOrigin(), FColor::Green, 1.f, 16, SDPG_Foreground, 2.f);
	}
	
	// Draw circles representing the door access, orange circle we can't open from that side, green circle we can
	const bool bFrontAccess = Door->GetDoorAccess() == EDoorAccess::Bidirectional || 
		Door->GetDoorAccess() == EDoorAccess::Front;

	const bool bBackAccess = Door->GetDoorAccess() == EDoorAccess::Bidirectional || 
		Door->GetDoorAccess() == EDoorAccess::Behind;

	const FLinearColor FrontAccessColor = bFrontAccess ? FColor::Green.ReinterpretAsLinear() : FColor::Orange.ReinterpretAsLinear();
	const FLinearColor BackAccessColor = bBackAccess ? FColor::Green.ReinterpretAsLinear() : FColor::Orange.ReinterpretAsLinear();

	DrawCircle(PDI, FMath::Lerp<FVector>(FloorFront, FloorExtendFront, 0.5f), 
		LocalX, LocalY, FrontAccessColor, Extents.Y * .2f, 3, SDPG_Foreground, LineThickness);

	DrawCircle(PDI, FMath::Lerp<FVector>(FloorBack, FloorExtendBack, 0.5f), 
		LocalX, LocalY, BackAccessColor, Extents.Y * .2f, 3, SDPG_Foreground, LineThickness);

	// Door direction locking
	const bool bFrontLocked = Door->GetDoorOpenDirection() != EDoorOpenDirection::Bidirectional && 
		Door->GetDoorOpenDirection() != EDoorOpenDirection::Outward;

	const bool bBackLocked = Door->GetDoorOpenDirection() != EDoorOpenDirection::Bidirectional && 
		Door->GetDoorOpenDirection() != EDoorOpenDirection::Inward;

	const FLinearColor FrontLockColor = bFrontLocked ? FColor::Orange.ReinterpretAsLinear() : FColor::Emerald.ReinterpretAsLinear();
	const FLinearColor BackLockColor = bBackLocked ? FColor::Orange.ReinterpretAsLinear() : FColor::Emerald.ReinterpretAsLinear();

	// Draw a box in front of the door
	PDI->DrawLine(FloorFrontLeft, FloorFrontRight, FrontLockColor, SDPG_World, LineThickness);
	PDI->DrawLine(FloorFrontLeft, FloorExtendFrontLeft, FrontLockColor, SDPG_World, LineThickness);
	PDI->DrawLine(FloorFrontRight, FloorExtendFrontRight, FrontLockColor, SDPG_World, LineThickness);
	PDI->DrawLine(FloorExtendFrontLeft, FloorExtendFrontRight, FrontLockColor, SDPG_World, LineThickness);

	// Draw segments horizontally through the box we just drew above
	constexpr int32 Segments = 5;
	for (int32 i = 1; i < Segments; ++i)
	{
		// The box above is on the ground, these must be on the ground too, as subdivisions of the box, we need to draw them 100cm to match the extension
		const FVector FloorLeft = FloorFrontLeft + LocalX * (Extension / (float)Segments) * i;
		const FVector FloorRight = FloorFrontRight + LocalX * (Extension / (float)Segments) * i;
		PDI->DrawLine(FloorLeft, FloorRight, FrontLockColor, SDPG_World, LineThickness);
	}

	// Draw a box at the back of the door floor
	PDI->DrawLine(FloorBackLeft, FloorBackRight, BackLockColor, SDPG_World, LineThickness);
	PDI->DrawLine(FloorBackLeft, FloorExtendBackLeft, BackLockColor, SDPG_World, LineThickness);
	PDI->DrawLine(FloorBackRight, FloorExtendBackRight, BackLockColor, SDPG_World, LineThickness);
	PDI->DrawLine(FloorExtendBackLeft, FloorExtendBackRight, BackLockColor, SDPG_World, LineThickness);

	// Draw segments horizontally through the box we just drew above
	for (int32 i = 1; i < Segments; ++i)
	{
		// The box above is on the ground, these must be on the ground too, as subdivisions of the box, we need to draw them 100cm to match the extension
		const FVector FloorLeft = FloorBackLeft + -LocalX * (Extension / (float)Segments) * i;
		const FVector FloorRight = FloorBackRight + -LocalX * (Extension / (float)Segments) * i;
		PDI->DrawLine(FloorLeft, FloorRight, BackLockColor, SDPG_World, LineThickness);
	}
}
