// Copyright (c) Jared Taylor


#include "Components/DoorEditorBillboard.h"

#include "Door.h"
#include "DoorTypes.h"

#if WITH_EDITORONLY_DATA
UTexture2D* UDoorEditorBillboard::ClosedOutwardSprite = nullptr;
UTexture2D* UDoorEditorBillboard::ClosedInwardSprite = nullptr;
UTexture2D* UDoorEditorBillboard::OpenOutwardSprite = nullptr;
UTexture2D* UDoorEditorBillboard::OpenInwardSprite = nullptr;
UTexture2D* UDoorEditorBillboard::OpeningOutwardSprite = nullptr;
UTexture2D* UDoorEditorBillboard::OpeningInwardSprite = nullptr;
UTexture2D* UDoorEditorBillboard::ClosingOutwardSprite = nullptr;
UTexture2D* UDoorEditorBillboard::ClosingInwardSprite = nullptr;
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(DoorEditorBillboard)

#if WITH_EDITOR
void UDoorEditorBillboard::OnDoorStateChanged(const ADoor* Door, bool bReplicated)
{
	if (!IsValid(Door))
	{
		return;
	}
	SetSprite(GetDoorSprite(Door, bReplicated));
}

UTexture2D* UDoorEditorBillboard::GetDoorSprite(const ADoor* Door, bool bReplicated) const
{
	if (bReplicated)
	{
		switch (Door->GetRepDoorState())
		{
		case EReplicatedDoorState::ClosedOutward: return ClosedOutwardSprite;
		case EReplicatedDoorState::ClosedInward: return ClosedInwardSprite;
		case EReplicatedDoorState::OpeningOutward: return OpeningOutwardSprite;
		case EReplicatedDoorState::OpeningInward: return OpeningInwardSprite;
		case EReplicatedDoorState::OpenOutward: return OpenOutwardSprite;
		case EReplicatedDoorState::OpenInward: return OpenInwardSprite;
		case EReplicatedDoorState::ClosingOutward: return ClosingOutwardSprite;
		case EReplicatedDoorState::ClosingInward: return ClosingInwardSprite;
		}
	}
	
	switch (Door->GetDoorState())
	{
	case EDoorState::Closed: return Door->GetDoorDirection() == EDoorDirection::Outward ? ClosedOutwardSprite : ClosedInwardSprite;
	case EDoorState::Closing: return Door->GetDoorDirection() == EDoorDirection::Outward ? ClosingOutwardSprite : ClosingInwardSprite;
	case EDoorState::Opening: return Door->GetDoorDirection() == EDoorDirection::Outward ? OpeningOutwardSprite : OpeningInwardSprite;
	case EDoorState::Open: return Door->GetDoorDirection() == EDoorDirection::Outward ? OpenOutwardSprite : OpenInwardSprite;
	default: return nullptr;
	}
}
#endif