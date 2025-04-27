// Copyright (c) Jared Taylor

#include "DoorsVisualizer.h"

#include "DoorEditorVisualizer.h"
#include "DoorVisualizer.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"

#define LOCTEXT_NAMESPACE "FDoorsVisualizerModule"

void FDoorsVisualizerModule::StartupModule()
{
	// Load icons for door state sprites
	FDoorVisualizer::ClosedOutwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Closed_Outward.T_Door_Sprite_Closed_Outward"));
	FDoorVisualizer::ClosedInwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Closed_Inward.T_Door_Sprite_Closed_Inward"));
	FDoorVisualizer::OpenOutwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Open_Outward.T_Door_Sprite_Open_Outward"));
	FDoorVisualizer::OpenInwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Open_Inward.T_Door_Sprite_Open_Inward"));
	FDoorVisualizer::OpeningOutwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Opening_Outward.T_Door_Sprite_Opening_Outward"));
	FDoorVisualizer::OpeningInwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Opening_Inward.T_Door_Sprite_Opening_Inward"));
	FDoorVisualizer::ClosingOutwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Closing_Outward.T_Door_Sprite_Closing_Outward"));
	FDoorVisualizer::ClosingInwardSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Doors/Editor/T_Door_Sprite_Closing_Inward.T_Door_Sprite_Closing_Inward"));
	
	GUnrealEd->RegisterComponentVisualizer(UDoorEditorVisualizer::StaticClass()->GetFName(), MakeShareable(new FDoorVisualizer()));
}

void FDoorsVisualizerModule::ShutdownModule()
{
	if (!UObjectInitialized() || IsEngineExitRequested())
	{
		return;
	}
	
	GUnrealEd->UnregisterComponentVisualizer(UDoorEditorVisualizer::StaticClass()->GetFName());
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FDoorsVisualizerModule, DoorsVisualizer)