#include "InputLagDiagnostics.h"
#include "InputLagPlayerController.h"

AInputLagPlayerController::AInputLagPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShowInputLagDiagnostics = false;
	LastInputTimestamp = 0.0;
	InputFrameNumber = 0;
	LastTrackedInputKey = EKeys::Invalid;
	bPendingInputMeasurement = false;
	InputLagHistory.Reserve(MaxInputLagSamples);
}

void AInputLagPlayerController::ShowInputLag()
{
	bShowInputLagDiagnostics = !bShowInputLagDiagnostics;
	if (bShowInputLagDiagnostics)
	{
		ClientMessage(TEXT("Input Lag Diagnostics: ON"));
	}
	else
	{
		ClientMessage(TEXT("Input Lag Diagnostics: OFF"));
	}
}

void AInputLagPlayerController::RecordInputTimestamp(FKey Key)
{
	if (!bShowInputLagDiagnostics)
	{
		return;
	}

	// Only record new input if we're not already waiting for measurement
	if (!bPendingInputMeasurement)
	{
		// Record the timestamp when input arrives from Windows
		LastInputTimestamp = FPlatformTime::Seconds();
		InputFrameNumber = GFrameCounter;
		LastTrackedInputKey = Key;
		bPendingInputMeasurement = true;
	}
}

void AInputLagPlayerController::RecordInputExecution(FKey Key)
{
	if (!bShowInputLagDiagnostics || LastInputTimestamp == 0.0)
	{
		return;
	}

	// Only record if this execution matches the last tracked input
	if (Key == LastTrackedInputKey)
	{
		double ExecutionTime = FPlatformTime::Seconds();
		float InputLagMs = (ExecutionTime - LastInputTimestamp) * 1000.0f;

		// Add to history
		if (InputLagHistory.Num() >= MaxInputLagSamples)
		{
			InputLagHistory.RemoveAt(0);
		}
		InputLagHistory.Add(InputLagMs);

		// Reset for next input
		LastInputTimestamp = 0.0;
	}
}

float AInputLagPlayerController::GetAverageInputLag() const
{
	if (InputLagHistory.Num() == 0)
	{
		return 0.0f;
	}

	float Sum = 0.0f;
	for (float Lag : InputLagHistory)
	{
		Sum += Lag;
	}
	return Sum / InputLagHistory.Num();
}

float AInputLagPlayerController::GetLastInputLag() const
{
	if (InputLagHistory.Num() == 0)
	{
		return 0.0f;
	}
	return InputLagHistory.Last();
}

bool AInputLagPlayerController::InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	// Record timestamp for mouse button presses
	if (bShowInputLagDiagnostics && (EventType == IE_Pressed || EventType == IE_Repeat))
	{
		if (Key == EKeys::LeftMouseButton || Key == EKeys::RightMouseButton)
		{
			RecordInputTimestamp(Key);
		}
	}

	return Super::InputKey(Key, EventType, AmountDepressed, bGamepad);
}

bool AInputLagPlayerController::InputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	// Record timestamp for mouse movement
	if (bShowInputLagDiagnostics)
	{
		if (Key == EKeys::MouseX || Key == EKeys::MouseY)
		{
			RecordInputTimestamp(Key);
		}
	}

	return Super::InputAxis(Key, Delta, DeltaTime, NumSamples, bGamepad);
}

void AInputLagPlayerController::MeasureInputLagEndOfFrame()
{
	if (!bPendingInputMeasurement)
	{
		return;
	}

	// Only measure if we're in a LATER frame than when input was recorded
	// This ensures we're measuring "input frame N -> visual result appears in frame N+1"
	if (GFrameCounter <= InputFrameNumber)
	{
		return; // Still in same frame, wait for next frame
	}

	// Measure at end of frame rendering (called from HUD's DrawHUD)
	// This captures TRUE perceived input lag:
	// - Input recorded in frame N
	// - Game processes input in frame N
	// - Frame N rendering completes
	// - Measurement happens in frame N+1 DrawHUD
	// - This gives us the time from "input arrives" to "frame with input's effect is ready"
	double CurrentTime = FPlatformTime::Seconds();
	float InputLagMs = (CurrentTime - LastInputTimestamp) * 1000.0f;

	// Sanity check - if lag is impossibly high (> 1 second), skip it
	if (InputLagMs > 0.0f && InputLagMs < 1000.0f)
	{
		// Add to history
		if (InputLagHistory.Num() >= MaxInputLagSamples)
		{
			InputLagHistory.RemoveAt(0);
		}
		InputLagHistory.Add(InputLagMs);
	}

	// Reset
	bPendingInputMeasurement = false;
	LastInputTimestamp = 0.0;
	InputFrameNumber = 0;
}
