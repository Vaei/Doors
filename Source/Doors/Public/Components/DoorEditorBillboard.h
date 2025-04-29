// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Components/BillboardComponent.h"
#include "DoorEditorBillboard.generated.h"


/**
 * Door editor component
 * Used to draw debug sprites during PIE in editor
 */
UCLASS(ClassGroup=(Custom))
class DOORS_API UDoorEditorBillboard : public UBillboardComponent
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
public:
	static UTexture2D* ClosedInwardSprite;
	static UTexture2D* ClosedOutwardSprite;
	static UTexture2D* OpenInwardSprite;
	static UTexture2D* OpenOutwardSprite;
	static UTexture2D* OpeningInwardSprite;
	static UTexture2D* OpeningOutwardSprite;
	static UTexture2D* ClosingInwardSprite;
	static UTexture2D* ClosingOutwardSprite;
#endif
	
public:
	UDoorEditorBillboard()
	{
		bIsEditorOnly = true;
	}

#if WITH_EDITORONLY_DATA
	virtual UTexture2D* GetDoorSprite(const ADoor* Door) const;
#endif
};
