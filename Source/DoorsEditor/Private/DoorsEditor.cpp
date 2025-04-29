// Copyright (c) Jared Taylor

#include "DoorsEditor.h"

#include "Door.h"
#include "DoorCustomization.h"

#define LOCTEXT_NAMESPACE "FDoorsEditorModule"

void FDoorsEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	
	PropertyModule.RegisterCustomClassLayout(ADoor::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FDoorCustomization::MakeInstance));
}

void FDoorsEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule* PropertyModule = FModuleManager::Get().GetModulePtr<FPropertyEditorModule>("PropertyEditor");
		PropertyModule->UnregisterCustomClassLayout(ADoor::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FDoorsEditorModule, DoorsEditor)