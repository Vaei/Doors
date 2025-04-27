// Copyright (c) Jared Taylor

#include "DoorsVisualizer.h"

#include "DoorEditorVisualizer.h"
#include "DoorVisualizer.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"

#define LOCTEXT_NAMESPACE "FDoorsVisualizerModule"

void FDoorsVisualizerModule::StartupModule()
{
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