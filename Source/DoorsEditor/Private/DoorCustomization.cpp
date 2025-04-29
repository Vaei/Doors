// Copyright (c) Jared Taylor. All Rights Reserved


#include "DoorCustomization.h"

#include "DetailLayoutBuilder.h"


TSharedRef<IDetailCustomization> FDoorCustomization::MakeInstance()
{
	return MakeShared<FDoorCustomization>();
}

void FDoorCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.EditCategory(TEXT("Door"), FText::GetEmpty(), ECategoryPriority::Important);
}