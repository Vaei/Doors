// Copyright (c) Jared Taylor


#include "DoorCustomization.h"

#include "DetailLayoutBuilder.h"


TSharedRef<IDetailCustomization> FDoorCustomization::MakeInstance()
{
	return MakeShared<FDoorCustomization>();
}

void FDoorCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.EditCategory(TEXT("Door"), FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("DoorChange"), FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("Door Change"), FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("DoorProperties"), FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("Door Properties"), FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("DoorAlpha"), FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("Door Alpha"), FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("DoorTime"), FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("Door Time"), FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("DoorPreview"), FText::GetEmpty(), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("Door Preview"), FText::GetEmpty(), ECategoryPriority::Important);
}