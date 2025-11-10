#include "InputLagDiagnostics.h"
#include "InputLagDiagnosticsMutator.h"
#include "InputLagPlayerController.h"
#include "InputLagHUD.h"
#include "UTGameMode.h"

AInputLagDiagnosticsMutator::AInputLagDiagnosticsMutator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoEnableForAllPlayers = true;
	DisplayName = NSLOCTEXT("InputLagDiagnostics", "InputLagDiagnostics", "Input Lag Diagnostics");
	Description = NSLOCTEXT("InputLagDiagnostics", "InputLagDiagnosticsDesc", "Enables real-time input lag measurement for mouse inputs");
}

void AInputLagDiagnosticsMutator::Init_Implementation(const FString& Options)
{
	Super::Init_Implementation(Options);

	// Replace PlayerController and HUD classes in the game mode
	AUTGameMode* UTGameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
	if (UTGameMode)
	{
		UTGameMode->PlayerControllerClass = AInputLagPlayerController::StaticClass();
		UTGameMode->HUDClass = AInputLagHUD::StaticClass();
	}
}

void AInputLagDiagnosticsMutator::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoEnableForAllPlayers)
	{
		// Enable for all current players
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			AInputLagPlayerController* PC = Cast<AInputLagPlayerController>(*Iterator);
			if (PC)
			{
				PC->bShowInputLagDiagnostics = true;
				if (PC->IsLocalPlayerController())
				{
					PC->ClientMessage(TEXT("Input Lag Diagnostics: Auto-enabled. Use 'ShowInputLag' command to toggle."));
				}
			}
		}
	}
}
