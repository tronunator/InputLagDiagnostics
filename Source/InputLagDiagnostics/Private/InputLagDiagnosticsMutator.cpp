#include "InputLagDiagnostics.h"
#include "InputLagDiagnosticsMutator.h"
#include "InputLagPlayerController.h"
#include "InputLagHUD.h"
#include "InputLagHUDHelper.h"
#include "UTGameMode.h"
#include "UTHUD.h"
#include "Engine/Canvas.h"

AInputLagDiagnosticsMutator::AInputLagDiagnosticsMutator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InputLagDiagnostics(nullptr)
{
	bAutoEnableForAllPlayers = true;
	DisplayName = NSLOCTEXT("InputLagDiagnostics", "InputLagDiagnostics", "Input Lag Diagnostics");
	Description = NSLOCTEXT("InputLagDiagnostics", "InputLagDiagnosticsDesc", "Enables real-time input lag measurement for mouse inputs");
	
	// Enable ticking for this mutator
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
}

void AInputLagDiagnosticsMutator::Init_Implementation(const FString& Options)
{
	Super::Init_Implementation(Options);

	// Create our input lag diagnostics helper (plain C++ class, no inheritance issues)
	InputLagDiagnostics = new FInputLagDiagnostics();
}

void AInputLagDiagnosticsMutator::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("InputLag Mutator: BeginPlay called"));

	if (bAutoEnableForAllPlayers && InputLagDiagnostics)
	{
		// Find local player and enable diagnostics
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PC = Iterator->Get();
			if (PC && PC->IsLocalPlayerController())
			{
				InputLagDiagnostics->PlayerOwner = PC;
				InputLagDiagnostics->bShowInputLagDiagnostics = true;
				
				// Register this mutator for PostRender callbacks (HUD drawing)
				AHUD* HUD = PC->GetHUD();
				if (HUD)
				{
					HUD->AddPostRenderedActor(this);
					UE_LOG(LogTemp, Warning, TEXT("InputLag Mutator: Added to PostRenderedActors for HUD drawing"));
				}
				
				PC->ClientMessage(TEXT("Input Lag Diagnostics: Auto-enabled. Type 'mutate showinputlag' to toggle."));
				UE_LOG(LogTemp, Warning, TEXT("InputLag Mutator: Auto-enabled for player"));
				break;
			}
		}
	}
}

void AInputLagDiagnosticsMutator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!InputLagDiagnostics)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TEXT("InputLag: No diagnostics object!"));
		}
		return;
	}

	// Find player owner if we don't have one yet
	if (!InputLagDiagnostics->PlayerOwner)
	{
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PC = Iterator->Get();
			if (PC && PC->IsLocalPlayerController())
			{
				InputLagDiagnostics->PlayerOwner = PC;
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("InputLag: Found player owner!"));
				}
				break;
			}
		}
	}
	
	// Update input tracking
	if (InputLagDiagnostics->PlayerOwner && InputLagDiagnostics->PlayerOwner->PlayerInput)
	{
		InputLagDiagnostics->Tick(DeltaTime);
		
		// NOTE: We do NOT finalize here - we wait until PostRenderFor (after rendering)
		// to get accurate input-to-display latency
	}
	
}

void AInputLagDiagnosticsMutator::PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir)
{
	if (!InputLagDiagnostics || !Canvas)
	{
		return;
	}

	// Finalize input lag measurement NOW - at the point when the frame is actually being rendered
	// This gives us true input-to-display latency (from input event to pixels being drawn)
	InputLagDiagnostics->FinalizeInputLagMeasurement();

	// Only draw if diagnostics are enabled
	if (!InputLagDiagnostics->bShowInputLagDiagnostics)
	{
		return;
	}

	// Get statistics
	float SmoothedLag = InputLagDiagnostics->SmoothedInputLag;
	float RawLag = InputLagDiagnostics->RawInputLag;
	float AvgLag = InputLagDiagnostics->GetAverageInputLag();
	float MinLag = InputLagDiagnostics->GetMinInputLag();
	float MaxLag = InputLagDiagnostics->GetMaxInputLag();
	float P95Lag = InputLagDiagnostics->Get95thPercentileInputLag();
	
	// Position on left side of screen
	float X = 20.0f;
	float Y = Canvas->ClipY / 2.0f - 80.0f;
	
	// Font and colors
	UFont* Font = GEngine->GetMediumFont();
	FLinearColor TitleColor = FLinearColor::Yellow;
	FLinearColor ValueColor = FLinearColor::White;
	
	// Draw title
	Canvas->SetDrawColor(FColor(255, 255, 0, 255));
	Canvas->DrawText(Font, TEXT("=== Input Lag Diagnostics ==="), X, Y);
	Y += 20.0f;
	
	// Draw smoothed lag
	FLinearColor Color = SmoothedLag < 20.0f ? FLinearColor::Green : (SmoothedLag < 40.0f ? FLinearColor::Yellow : FLinearColor::Red);
	Canvas->SetDrawColor(FColor(Color.R * 255, Color.G * 255, Color.B * 255, 255));
	Canvas->DrawText(Font, FString::Printf(TEXT("Smoothed: %.2f ms"), SmoothedLag), X, Y);
	Y += 18.0f;
	
	// Draw raw lag
	Canvas->SetDrawColor(FColor(255, 255, 255, 255));
	Canvas->DrawText(Font, FString::Printf(TEXT("Raw: %.2f ms"), RawLag), X, Y);
	Y += 18.0f;
	
	// Draw average
	Canvas->DrawText(Font, FString::Printf(TEXT("Average: %.2f ms"), AvgLag), X, Y);
	Y += 18.0f;
	
	// Draw min/max
	Canvas->DrawText(Font, FString::Printf(TEXT("Min: %.2f ms  Max: %.2f ms"), MinLag, MaxLag), X, Y);
	Y += 18.0f;
	
	// Draw 95th percentile
	Canvas->DrawText(Font, FString::Printf(TEXT("95th Percentile: %.2f ms"), P95Lag), X, Y);
	Y += 18.0f;
	
	// Draw last tracked key
	Canvas->SetDrawColor(FColor(200, 200, 200, 255));
	Canvas->DrawText(Font, FString::Printf(TEXT("Last Input: %s"), *InputLagDiagnostics->LastTrackedInputKey.ToString()), X, Y);
}

void AInputLagDiagnosticsMutator::ModifyPlayer_Implementation(APawn* Other, bool bIsNewSpawn)
{
	Super::ModifyPlayer_Implementation(Other, bIsNewSpawn);
	
	// Ensure our diagnostics helper has reference to the player controller
	if (InputLagDiagnostics && Other && Other->GetController())
	{
		APlayerController* PC = Cast<APlayerController>(Other->GetController());
		if (PC && PC->IsLocalPlayerController())
		{
			InputLagDiagnostics->PlayerOwner = PC;
		}
	}
}

void AInputLagDiagnosticsMutator::Mutate_Implementation(const FString& MutateString, APlayerController* Sender)
{
	if (MutateString.Equals(TEXT("showinputlag"), ESearchCase::IgnoreCase))
	{
		if (InputLagDiagnostics)
		{
			InputLagDiagnostics->ShowInputLag();
		}
	}
	else if (MutateString.Equals(TEXT("loginputlag"), ESearchCase::IgnoreCase))
	{
		if (InputLagDiagnostics)
		{
			InputLagDiagnostics->ToggleCSVLogging();
		}
	}
	else
	{
		Super::Mutate_Implementation(MutateString, Sender);
	}
}
