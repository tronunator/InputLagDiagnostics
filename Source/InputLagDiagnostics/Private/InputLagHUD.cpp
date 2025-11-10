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

	float CurrentLag = InputLagPC->GetLastInputLag();
	float AverageLag = InputLagPC->GetAverageInputLag();
	int32 SampleCount = InputLagPC->InputLagHistory.Num();

	// Position in top-right corner
	float XPos = Canvas->ClipX - 350.0f;
	float YPos = 50.0f;
	float LineHeight = 25.0f;

	// Draw background
	FCanvasTileItem BackgroundItem(FVector2D(XPos - 10.0f, YPos - 10.0f), 
		FVector2D(340.0f, LineHeight * 3.0f + 20.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));
	BackgroundItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BackgroundItem);

	// Choose color based on lag severity
	FLinearColor LagColor;
	if (CurrentLag < 10.0f)
	{
		LagColor = FLinearColor::Green;
	}
	else if (CurrentLag < 20.0f)
	{
		LagColor = FLinearColor::Yellow;
	}
	else
	{
		LagColor = FLinearColor::Red;
	}

	// Draw text
	FString CurrentText = FString::Printf(TEXT("Current Input Lag: %.2f ms"), CurrentLag);
	FString AverageText = FString::Printf(TEXT("Average Input Lag: %.2f ms"), AverageLag);
	FString SamplesText = FString::Printf(TEXT("Samples: %d"), SampleCount);

	// Draw shadow then colored text
	Canvas->SetLinearDrawColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	Canvas->DrawText(GEngine->GetSmallFont(), CurrentText, XPos, YPos, 1.0f, 1.0f);
	Canvas->SetLinearDrawColor(LagColor);
	Canvas->DrawText(GEngine->GetSmallFont(), CurrentText, XPos - 1.0f, YPos - 1.0f, 1.0f, 1.0f);

	Canvas->SetLinearDrawColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	Canvas->DrawText(GEngine->GetSmallFont(), AverageText, XPos, YPos + LineHeight, 1.0f, 1.0f);
	Canvas->SetLinearDrawColor(FLinearColor::White);
	Canvas->DrawText(GEngine->GetSmallFont(), AverageText, XPos - 1.0f, YPos + LineHeight - 1.0f, 1.0f, 1.0f);

	Canvas->SetLinearDrawColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	Canvas->DrawText(GEngine->GetSmallFont(), SamplesText, XPos, YPos + LineHeight * 2.0f, 1.0f, 1.0f);
	Canvas->SetLinearDrawColor(FLinearColor::Gray);
	Canvas->DrawText(GEngine->GetSmallFont(), SamplesText, XPos - 1.0f, YPos + LineHeight * 2.0f - 1.0f, 1.0f, 1.0f);
}
