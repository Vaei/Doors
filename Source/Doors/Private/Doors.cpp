// Copyright Epic Games, Inc. All Rights Reserved.

#include "Doors.h"

#if WITH_EDITORONLY_DATA
#include "Components/DoorEditorBillboard.h"
#endif

#define LOCTEXT_NAMESPACE "FDoorsModule"

void FDoorsModule::StartupModule()
{
#if WITH_EDITORONLY_DATA
	// Load icons for door state sprites
	UDoorEditorBillboard::ClosedOutwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Closed_Outward.T_Door_Sprite_Closed_Outward"));
	UDoorEditorBillboard::ClosedInwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Closed_Inward.T_Door_Sprite_Closed_Inward"));
	UDoorEditorBillboard::OpenOutwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Open_Outward.T_Door_Sprite_Open_Outward"));
	UDoorEditorBillboard::OpenInwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Open_Inward.T_Door_Sprite_Open_Inward"));
	UDoorEditorBillboard::OpeningOutwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Opening_Outward.T_Door_Sprite_Opening_Outward"));
	UDoorEditorBillboard::OpeningInwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Opening_Inward.T_Door_Sprite_Opening_Inward"));
	UDoorEditorBillboard::ClosingOutwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Closing_Outward.T_Door_Sprite_Closing_Outward"));
	UDoorEditorBillboard::ClosingInwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Closing_Inward.T_Door_Sprite_Closing_Inward"));
#endif
}

void FDoorsModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDoorsModule, Doors)