#include "InputLagDiagnostics.h"
#include "InputLagHUD.h"
#include "InputLagPlayerController.h"

AInputLagHUD::AInputLagHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AInputLagHUD::DrawHUD()
{
	Super::DrawHUD();

	// Finalize input lag measurement at end of frame rendering
	FinalizeInputLagMeasurement();

	// Draw input lag diagnostics if enabled
	AInputLagPlayerController* InputLagPC = Cast<AInputLagPlayerController>(UTPlayerOwner);
	if (InputLagPC && InputLagPC->bShowInputLagDiagnostics)
	{
		DrawInputLagDiagnostics();
	}
}

void AInputLagHUD::FinalizeInputLagMeasurement()
{
	AInputLagPlayerController* InputLagPC = Cast<AInputLagPlayerController>(UTPlayerOwner);
	if (InputLagPC && InputLagPC->bPendingInputMeasurement)
	{
		// Measure at the end of frame rendering (DrawHUD is one of the last calls)
		InputLagPC->MeasureInputLagEndOfFrame();
	}
}

void AInputLagHUD::DrawInputLagDiagnostics()
{
	AInputLagPlayerController* InputLagPC = Cast<AInputLagPlayerController>(UTPlayerOwner);
	if (!InputLagPC || !Canvas)
	{
		return;
	}

	// Get statistics
	float SmoothedLag = InputLagPC->GetSmoothedInputLag();
	float RawLag = InputLagPC->GetRawInputLag();
	float AverageLag = InputLagPC->GetAverageInputLag();
	float MinLag = InputLagPC->GetMinInputLag();
	float MaxLag = InputLagPC->GetMaxInputLag();
	float Percentile95 = InputLagPC->Get95thPercentileInputLag();

	// Position in top-right corner (similar to stat unit)
	float XPos = Canvas->ClipX - 450.0f;
	float YPos = 50.0f;
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
	FString KeyName = InputLagPC->LastTrackedInputKey.ToString();
	DrawShadowedText(LabelX, YPos, TEXT("Tracking:"), FLinearColor::White);
	DrawShadowedText(ValueX, YPos, KeyName, FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
}
