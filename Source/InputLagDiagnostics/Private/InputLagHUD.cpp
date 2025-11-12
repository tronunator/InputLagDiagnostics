#include "InputLagDiagnostics.h"
#include "InputLagHUD.h"

FInputLagDiagnostics::FInputLagDiagnostics()
	: bShowInputLagDiagnostics(false)
	, bEnableCSVLogging(false)
	, PlayerOwner(nullptr)
	, Canvas(nullptr)
	, LastInputTimestamp(0.0)
	, InputFrameNumber(0)
	, LastTrackedInputKey(EKeys::Invalid)
	, bPendingInputMeasurement(false)
	, HistoryIndex(0)
	, SmoothedInputLag(0.0f)
	, RawInputLag(0.0f)
	, CSVFileHandle(nullptr)
	, CSVSampleCount(0)
{
	// Pre-allocate circular buffer
	InputLagHistory.AddZeroed(MaxInputLagSamples);
}

void FInputLagDiagnostics::OnInputKey(FKey Key, EInputEvent EventType)
{
	if (!bShowInputLagDiagnostics)
	{
		return;
	}

	// Record timestamp for mouse button presses
	if ((EventType == IE_Pressed || EventType == IE_Repeat))
	{
		if (!bPendingInputMeasurement && (Key == EKeys::LeftMouseButton || Key == EKeys::RightMouseButton))
		{
			LastInputTimestamp = FPlatformTime::Seconds();
			InputFrameNumber = GFrameCounter;
			LastTrackedInputKey = Key;
			bPendingInputMeasurement = true;
		}
	}
}

void FInputLagDiagnostics::OnInputAxis(FKey Key, float Delta)
{
	if (!bShowInputLagDiagnostics)
	{
		return;
	}

	// Record timestamp for mouse movement
	if (!bPendingInputMeasurement && (Key == EKeys::MouseX || Key == EKeys::MouseY))
	{
		LastInputTimestamp = FPlatformTime::Seconds();
		InputFrameNumber = GFrameCounter;
		LastTrackedInputKey = Key;
		bPendingInputMeasurement = true;
	}
}

void FInputLagDiagnostics::ShowInputLag()
{
	bShowInputLagDiagnostics = !bShowInputLagDiagnostics;
	
	// Log to output log
	UE_LOG(LogTemp, Warning, TEXT("InputLag: Toggled to %s"), bShowInputLagDiagnostics ? TEXT("ON") : TEXT("OFF"));
	
	if (PlayerOwner)
	{
		if (bShowInputLagDiagnostics)
		{
			PlayerOwner->ClientMessage(TEXT("Input Lag Diagnostics: ON - Check left side of screen"));
		}
		else
		{
			PlayerOwner->ClientMessage(TEXT("Input Lag Diagnostics: OFF"));
		}
	}
}

void FInputLagDiagnostics::Tick(float DeltaTime)
{
	// Track input events each frame by checking PlayerInput state
	if (bShowInputLagDiagnostics && PlayerOwner && PlayerOwner->PlayerInput)
	{
		// Track mouse button presses (check for key down transitions)
		if (PlayerOwner->WasInputKeyJustPressed(EKeys::LeftMouseButton) && !bPendingInputMeasurement)
		{
			OnInputKey(EKeys::LeftMouseButton, IE_Pressed);
		}
		else if (PlayerOwner->WasInputKeyJustPressed(EKeys::RightMouseButton) && !bPendingInputMeasurement)
		{
			OnInputKey(EKeys::RightMouseButton, IE_Pressed);
		}
		
		// Track mouse movement (check for axis delta)
		float MouseX = PlayerOwner->PlayerInput->GetKeyValue(EKeys::MouseX);
		float MouseY = PlayerOwner->PlayerInput->GetKeyValue(EKeys::MouseY);
		
		if (!bPendingInputMeasurement)
		{
			if (FMath::Abs(MouseX) > 0.0f)
			{
				OnInputAxis(EKeys::MouseX, MouseX);
			}
			else if (FMath::Abs(MouseY) > 0.0f)
			{
				OnInputAxis(EKeys::MouseY, MouseY);
			}
		}
	}
}

void FInputLagDiagnostics::DrawHUD()
{
	// Early exit if no canvas
	if (!Canvas)
	{
		if (bShowInputLagDiagnostics && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, TEXT("InputLag DrawHUD: No Canvas"));
		}
		return;
	}

	// Finalize input lag measurement at end of frame rendering
	FinalizeInputLagMeasurement();

	// Draw input lag diagnostics if enabled
	if (bShowInputLagDiagnostics)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, TEXT("InputLag: Drawing!"));
		}
		DrawInputLagDiagnostics();
	}
}

void FInputLagDiagnostics::FinalizeInputLagMeasurement()
{
	if (!bPendingInputMeasurement)
	{
		return;
	}

	// Only measure if we're in a LATER frame than when input was recorded
	if (GFrameCounter <= InputFrameNumber)
	{
		return;
	}

	// Measure at end of frame rendering
	double CurrentTime = FPlatformTime::Seconds();
	float InputLagMs = (CurrentTime - LastInputTimestamp) * 1000.0f;

	// Sanity check
	if (InputLagMs > 0.0f && InputLagMs < 1000.0f)
	{
		RawInputLag = InputLagMs;
		
		if (SmoothedInputLag == 0.0f)
		{
			SmoothedInputLag = InputLagMs;
		}
		else
		{
			SmoothedInputLag = 0.9f * SmoothedInputLag + 0.1f * InputLagMs;
		}
		
		InputLagHistory[HistoryIndex] = InputLagMs;
		HistoryIndex = (HistoryIndex + 1) % MaxInputLagSamples;

		// Write to CSV if logging is enabled
		WriteCSVEntry(InputLagMs);
	}

	bPendingInputMeasurement = false;
	LastInputTimestamp = 0.0;
	InputFrameNumber = 0;
}

float FInputLagDiagnostics::GetAverageInputLag() const
{
	float Sum = 0.0f;
	int32 ValidSamples = 0;
	
	for (float Lag : InputLagHistory)
	{
		if (Lag > 0.0f)
		{
			Sum += Lag;
			ValidSamples++;
		}
	}
	
	return (ValidSamples > 0) ? (Sum / ValidSamples) : 0.0f;
}

float FInputLagDiagnostics::GetLastInputLag() const
{
	int32 LastIndex = (HistoryIndex - 1 + MaxInputLagSamples) % MaxInputLagSamples;
	return InputLagHistory[LastIndex];
}

float FInputLagDiagnostics::GetMinInputLag() const
{
	float MinLag = FLT_MAX;
	bool bFoundValid = false;
	
	for (float Lag : InputLagHistory)
	{
		if (Lag > 0.0f)
		{
			MinLag = FMath::Min(MinLag, Lag);
			bFoundValid = true;
		}
	}
	
	return bFoundValid ? MinLag : 0.0f;
}

float FInputLagDiagnostics::GetMaxInputLag() const
{
	float MaxLag = 0.0f;
	
	for (float Lag : InputLagHistory)
	{
		if (Lag > 0.0f)
		{
			MaxLag = FMath::Max(MaxLag, Lag);
		}
	}
	
	return MaxLag;
}

float FInputLagDiagnostics::Get95thPercentileInputLag() const
{
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
	
	ValidSamples.Sort();
	
	int32 Index = FMath::CeilToInt(ValidSamples.Num() * 0.95f) - 1;
	Index = FMath::Clamp(Index, 0, ValidSamples.Num() - 1);
	
	return ValidSamples[Index];
}

void FInputLagDiagnostics::DrawInputLagDiagnostics()
{
	if (!Canvas)
	{
		return;
	}

	// Get statistics
	float SmoothedLag = SmoothedInputLag;
	float RawLag = RawInputLag;
	float AverageLag = GetAverageInputLag();
	float MinLag = GetMinInputLag();
	float MaxLag = GetMaxInputLag();
	float Percentile95 = Get95thPercentileInputLag();

	// Position on middle-left of screen
	float XPos = 20.0f; // Left edge with small margin
	float YPos = (Canvas->ClipY / 2.0f) - 80.0f; // Vertically centered (accounting for box height)
	float LineHeight = 22.0f;
	
	// Label column and value column X positions
	float LabelX = XPos;
	float ValueX = XPos + 180.0f;

	// Draw background
	FCanvasTileItem BackgroundItem(FVector2D(XPos - 10.0f, YPos - 10.0f), 
		FVector2D(440.0f, LineHeight * 7.0f + 20.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
	BackgroundItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BackgroundItem);

	UFont* Font = GEngine->GetSmallFont();
	
	// Helper lambda for color-coded display (like stat unit does)
	auto GetLagColor = [](float Lag) -> FLinearColor
	{
		if (Lag < 10.0f) return FLinearColor::Green;   // Excellent
		if (Lag < 20.0f) return FLinearColor::Yellow;  // Good
		if (Lag < 33.0f) return FLinearColor(1.0f, 0.5f, 0.0f, 1.0f); // Orange - OK
		return FLinearColor::Red;  // Poor
	};
	
	// Helper lambda to draw shadowed text
	auto DrawShadowedText = [&](float X, float Y, const FString& Text, const FLinearColor& Color)
	{
		// Shadow
		Canvas->SetLinearDrawColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
		Canvas->DrawText(Font, Text, X + 1.0f, Y + 1.0f, 1.0f, 1.0f);
		// Main text
		Canvas->SetLinearDrawColor(Color);
		Canvas->DrawText(Font, Text, X, Y, 1.0f, 1.0f);
	};

	// Title
	DrawShadowedText(LabelX, YPos, TEXT("Input Lag Diagnostics"), FLinearColor(1.0f, 1.0f, 0.0f, 1.0f));
	YPos += LineHeight;

	// Smoothed (primary display - like stat unit shows smoothed values)
	DrawShadowedText(LabelX, YPos, TEXT("Smoothed:"), FLinearColor::White);
	DrawShadowedText(ValueX, YPos, FString::Printf(TEXT("%5.2f ms"), SmoothedLag), GetLagColor(SmoothedLag));
	YPos += LineHeight;

	// Raw (current frame - like stat unit's raw mode)
	DrawShadowedText(LabelX, YPos, TEXT("Raw:"), FLinearColor::White);
	DrawShadowedText(ValueX, YPos, FString::Printf(TEXT("%5.2f ms"), RawLag), GetLagColor(RawLag));
	YPos += LineHeight;

	// Average
	DrawShadowedText(LabelX, YPos, TEXT("Average:"), FLinearColor::White);
	DrawShadowedText(ValueX, YPos, FString::Printf(TEXT("%5.2f ms"), AverageLag), FLinearColor::White);
	YPos += LineHeight;

	// Min/Max (like stat unitmax)
	DrawShadowedText(LabelX, YPos, TEXT("Min / Max:"), FLinearColor::White);
	DrawShadowedText(ValueX, YPos, FString::Printf(TEXT("%5.2f / %5.2f ms"), MinLag, MaxLag), FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));
	YPos += LineHeight;

	// 95th Percentile (useful metric - 95% of frames are below this)
	DrawShadowedText(LabelX, YPos, TEXT("95th Percentile:"), FLinearColor::White);
	DrawShadowedText(ValueX, YPos, FString::Printf(TEXT("%5.2f ms"), Percentile95), FLinearColor(0.8f, 0.8f, 1.0f, 1.0f));
	YPos += LineHeight;

	// Last tracked input key
	FString KeyName = LastTrackedInputKey.ToString();
	DrawShadowedText(LabelX, YPos, TEXT("Tracking:"), FLinearColor::White);
	DrawShadowedText(ValueX, YPos, KeyName, FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
	YPos += LineHeight;

	// CSV logging status
	if (bEnableCSVLogging)
	{
		DrawShadowedText(LabelX, YPos, TEXT("CSV Logging:"), FLinearColor::White);
		DrawShadowedText(ValueX, YPos, FString::Printf(TEXT("ON (%d samples)"), CSVSampleCount), FLinearColor::Green);
	}
}

void FInputLagDiagnostics::ToggleCSVLogging()
{
	bEnableCSVLogging = !bEnableCSVLogging;

	if (bEnableCSVLogging)
	{
		// Create CSV file in Logs directory
		FString LogsDir = FPaths::GameSavedDir() + TEXT("Logs/");
		FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
		CSVFilePath = LogsDir + TEXT("InputLagLog_") + Timestamp + TEXT(".csv");

		// Create file
		CSVFileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*CSVFilePath);

		if (CSVFileHandle)
		{
			// Write CSV header
			FString Header = TEXT("Timestamp,FrameNumber,InputLag_ms,InputKey\n");
			CSVFileHandle->Write((const uint8*)TCHAR_TO_ANSI(*Header), Header.Len());
			CSVSampleCount = 0;

			if (PlayerOwner)
			{
				PlayerOwner->ClientMessage(FString::Printf(TEXT("CSV logging started: %s"), *CSVFilePath));
			}
		}
		else
		{
			bEnableCSVLogging = false;
			if (PlayerOwner)
			{
				PlayerOwner->ClientMessage(TEXT("Failed to create CSV file!"));
			}
		}
	}
	else
	{
		// Close CSV file
		if (CSVFileHandle)
		{
			delete CSVFileHandle;
			CSVFileHandle = nullptr;

			if (PlayerOwner)
			{
				PlayerOwner->ClientMessage(FString::Printf(TEXT("CSV logging stopped. %d samples written to: %s"), CSVSampleCount, *CSVFilePath));
			}
		}
	}
}

void FInputLagDiagnostics::WriteCSVEntry(float InputLag)
{
	if (!bEnableCSVLogging || !CSVFileHandle)
	{
		return;
	}

	// Write CSV row: Timestamp, FrameNumber, InputLag_ms, InputKey
	FString Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S.%s"));
	FString Row = FString::Printf(TEXT("%s,%llu,%.3f,%s\n"), 
		*Timestamp,
		GFrameCounter,
		InputLag,
		*LastTrackedInputKey.ToString()
	);

	CSVFileHandle->Write((const uint8*)TCHAR_TO_ANSI(*Row), Row.Len());
	// Note: No Flush() in UE4 4.15 IFileHandle, data is written on delete
	CSVSampleCount++;
}
