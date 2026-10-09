#pragma once

#include "Modules/ModuleManager.h"

class FPristineWaterPhysicsModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
