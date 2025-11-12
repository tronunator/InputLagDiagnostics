#pragma once

#include "Core.h"
#include "Engine.h"

/**
 * Helper class for input lag diagnostics rendering
 * This is a simple C++ class, not a UObject, to avoid any ABI issues with UT HUD inheritance
 */
class FInputLagDiagnostics
{
public:
	FInputLagDiagnostics();
	
	// Toggle for showing input lag diagnostics
	bool bShowInputLagDiagnostics;

	// Toggle for CSV logging
	bool bEnableCSVLogging;

	// Player controller reference for input tracking
	APlayerController* PlayerOwner;

	// Canvas for drawing (borrowed from actual HUD)
	UCanvas* Canvas;

	// Last recorded input timestamp
	double LastInputTimestamp;

	// Frame number when the input was recorded
	uint64 InputFrameNumber;

	// Last tracked input key name
	FKey LastTrackedInputKey;

	// Whether we're waiting to measure input lag
	bool bPendingInputMeasurement;

	// History of input lag measurements (in milliseconds) - circular buffer
	TArray<float> InputLagHistory;

	// Current index in the circular buffer
	int32 HistoryIndex;

	// Smoothed input lag value (exponential moving average)
	float SmoothedInputLag;

	// Raw (unsmoothed) input lag from last measurement
	float RawInputLag;

	// Maximum number of samples to keep
	static const int32 MaxInputLagSamples = 200;
	
	// Tick function for input tracking (called by mutator)
	void Tick(float DeltaTime);

	// Toggle input lag display (called by mutator's Exec command)
	void ShowInputLag();

	// Draw the input lag diagnostics overlay (call with Canvas set)
	void DrawHUD();

	// Draw the input lag diagnostics overlay
	void DrawInputLagDiagnostics();

	// Called at end of frame rendering to finalize input lag measurement
	void FinalizeInputLagMeasurement();

	// Get statistics
	float GetAverageInputLag() const;
	float GetLastInputLag() const;
	float GetMinInputLag() const;
	float GetMaxInputLag() const;
	float Get95thPercentileInputLag() const;

	// CSV logging
	void ToggleCSVLogging();
	void WriteCSVEntry(float InputLag);

private:
	// Input event delegates
	void OnInputKey(FKey Key, EInputEvent EventType);
	void OnInputAxis(FKey Key, float Delta);

	// CSV file handle
	IFileHandle* CSVFileHandle;
	
	// CSV file path
	FString CSVFilePath;
	
	// Number of samples written to CSV
	int32 CSVSampleCount;
};