#pragma once

#include "Core.h"
#include "Engine.h"
#include "UnrealTournament.h"
#include "UTPlayerController.h"
#include "InputLagPlayerController.generated.h"

/**
 * Custom PlayerController that adds input lag diagnostics
 * Tracks the time from input arrival to execution
 */
UCLASS()
class AInputLagPlayerController : public AUTPlayerController
{
	GENERATED_UCLASS_BODY()

public:
	// Toggle for showing input lag diagnostics
	UPROPERTY(BlueprintReadWrite, Category = "Input Lag")
	bool bShowInputLagDiagnostics;

	// Last recorded input timestamp
	double LastInputTimestamp;

	// Frame number when the input was recorded
	uint64 InputFrameNumber;

	// Last tracked input key name
	FKey LastTrackedInputKey;

	// Whether we're waiting to measure input lag
	bool bPendingInputMeasurement;

	// History of input lag measurements (in milliseconds)
	TArray<float> InputLagHistory;

	// Maximum number of samples to keep
	static const int32 MaxInputLagSamples = 200;

	// Console command to toggle input lag display
	UFUNCTION(Exec)
	void ShowInputLag();

	// Record the timestamp when an input is received
	void RecordInputTimestamp(FKey Key);

	// Record when the input action executes and calculate lag
	void RecordInputExecution(FKey Key);

	// Get the average input lag from history
	UFUNCTION(BlueprintCallable, Category = "Input Lag")
	float GetAverageInputLag() const;

	// Get the last recorded input lag
	UFUNCTION(BlueprintCallable, Category = "Input Lag")
	float GetLastInputLag() const;

	// Override input functions to track timestamps
	virtual bool InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad) override;
	virtual bool InputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad) override;

	// Called at the end of the frame to finalize input lag measurement
	// Public so HUD can call it at end of rendering
	void MeasureInputLagEndOfFrame();
};
