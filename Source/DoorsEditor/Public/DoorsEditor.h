// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FDoorsEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
