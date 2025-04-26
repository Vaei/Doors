// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FDoorsVisualizerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
