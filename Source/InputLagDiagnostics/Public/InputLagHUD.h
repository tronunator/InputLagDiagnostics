#pragma once

#include "Core.h"
#include "Engine.h"
#include "UnrealTournament.h"
#include "UTHUD.h"
#include "InputLagHUD.generated.h"

/**
 * Custom HUD that displays input lag diagnostics
 */
UCLASS()
class AInputLagHUD : public AUTHUD
{
	GENERATED_UCLASS_BODY()

public:
	virtual void DrawHUD() override;

	// Draw the input lag diagnostics overlay
	void DrawInputLagDiagnostics();

	// Called at end of frame rendering to finalize input lag measurement
	void FinalizeInputLagMeasurement();
};
