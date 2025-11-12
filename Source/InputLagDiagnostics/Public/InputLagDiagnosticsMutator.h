#pragma once

#include "Core.h"
#include "Engine.h"
#include "UnrealTournament.h"
#include "UTMutator.h"
#include "InputLagDiagnosticsMutator.generated.h"

// Forward declaration
class FInputLagDiagnostics;

UCLASS(Blueprintable, Meta = (ChildCanTick))
class AInputLagDiagnosticsMutator : public AUTMutator
{
	GENERATED_UCLASS_BODY()

public:
	/** Auto-enable diagnostics for all players when mutator is added */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Lag")
	bool bAutoEnableForAllPlayers;

	// Reference to our InputLag diagnostics helper (plain C++ class)
	FInputLagDiagnostics* InputLagDiagnostics;

	virtual void Init_Implementation(const FString& Options) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void ModifyPlayer_Implementation(APawn* Other, bool bIsNewSpawn) override;
	
	// Mutate command handler (called via "mutate showinputlag")
	virtual void Mutate_Implementation(const FString& MutateString, APlayerController* Sender) override;
	
	// PostRenderFor callback for HUD drawing (called by AHUD::DrawActorOverlays)
	virtual void PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir) override;
};
