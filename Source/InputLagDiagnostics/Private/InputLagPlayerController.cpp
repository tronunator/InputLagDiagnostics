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
	HistoryIndex = 0;
	SmoothedInputLag = 0.0f;
	RawInputLag = 0.0f;
	
	// Pre-allocate circular buffer
	InputLagHistory.AddZeroed(MaxInputLagSamples);
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
	float Sum = 0.0f;
	int32 ValidSamples = 0;
	
	for (float Lag : InputLagHistory)
	{
		if (Lag > 0.0f) // Only count valid samples (buffer may not be full yet)
		{
			Sum += Lag;
			ValidSamples++;
		}
	}
	
	return (ValidSamples > 0) ? (Sum / ValidSamples) : 0.0f;
}

float AInputLagPlayerController::GetLastInputLag() const
{
	// Get the most recent sample (before current index)
	int32 LastIndex = (HistoryIndex - 1 + MaxInputLagSamples) % MaxInputLagSamples;
	return InputLagHistory[LastIndex];
}

float AInputLagPlayerController::GetMinInputLag() const
{
	float MinLag = FLT_MAX;
	bool bFoundValid = false;
	
	for (float Lag : InputLagHistory)
	{
		if (Lag > 0.0f) // Only consider valid samples
		{
			MinLag = FMath::Min(MinLag, Lag);
			bFoundValid = true;
		}
	}
	
	return bFoundValid ? MinLag : 0.0f;
}

float AInputLagPlayerController::GetMaxInputLag() const
{
	float MaxLag = 0.0f;
	
	for (float Lag : InputLagHistory)
	{
		if (Lag > 0.0f) // Only consider valid samples
		{
			MaxLag = FMath::Max(MaxLag, Lag);
		}
	}
	
	return MaxLag;
}

float AInputLagPlayerController::Get95thPercentileInputLag() const
{
	// Collect valid samples
	TArray<float> ValidSamples;
	ValidSamples.Reserve(MaxInputLagSamples);
	
	for (float Lag : InputLagHistory)
	{
		if (Lag > 0.0f)
		{
			ValidSamples.Add(Lag);
		}
	}
	
	if (ValidSamples.Num() == 0)
	{
		return 0.0f;
	}
	
	// Sort samples
	ValidSamples.Sort();
	
	// Get 95th percentile (95% of samples are below this value)
	int32 Index = FMath::CeilToInt(ValidSamples.Num() * 0.95f) - 1;
	Index = FMath::Clamp(Index, 0, ValidSamples.Num() - 1);
	
	return ValidSamples[Index];
}

float AInputLagPlayerController::GetSmoothedInputLag() const
{
	return SmoothedInputLag;
}

float AInputLagPlayerController::GetRawInputLag() const
{
	return RawInputLag;
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
		// Store raw value
		RawInputLag = InputLagMs;
		
		// Calculate smoothed value (exponential moving average: 90% old + 10% new)
		// Similar to stat unit's smoothing
		if (SmoothedInputLag == 0.0f)
		{
			SmoothedInputLag = InputLagMs; // First sample
		}
		else
		{
			SmoothedInputLag = 0.9f * SmoothedInputLag + 0.1f * InputLagMs;
		}
		
		// Add to circular buffer (no array shifting needed!)
		InputLagHistory[HistoryIndex] = InputLagMs;
		HistoryIndex = (HistoryIndex + 1) % MaxInputLagSamples;
	}

	// Reset
	bPendingInputMeasurement = false;
	LastInputTimestamp = 0.0;
	InputFrameNumber = 0;
}
