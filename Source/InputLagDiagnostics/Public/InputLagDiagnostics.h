#pragma once

#include "ModuleManager.h"

class FInputLagDiagnosticsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
