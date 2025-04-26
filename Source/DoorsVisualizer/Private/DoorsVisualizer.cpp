// Copyright (c) Jared Taylor

#include "DoorsVisualizer.h"

#include "Door.h"
#include "DoorVisualizer.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"

#define LOCTEXT_NAMESPACE "FDoorsVisualizerModule"

void FDoorsVisualizerModule::StartupModule()
{
	GUnrealEd->RegisterComponentVisualizer(ADoor::StaticClass()->GetFName(), MakeShareable(new FDoorVisualizer()));
}

void FDoorsVisualizerModule::ShutdownModule()
{
	GUnrealEd->UnregisterComponentVisualizer(ADoor::StaticClass()->GetFName());
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FDoorsVisualizerModule, DoorsVisualizer)