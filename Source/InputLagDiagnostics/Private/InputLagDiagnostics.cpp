#include "InputLagDiagnostics.h"

#define LOCTEXT_NAMESPACE "FInputLagDiagnosticsModule"

void FInputLagDiagnosticsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory
}

void FInputLagDiagnosticsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FInputLagDiagnosticsModule, InputLagDiagnostics)
