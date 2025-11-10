#pragma once

#include "Core.h"
#include "Engine.h"
#include "UnrealTournament.h"
#include "UTMutator.h"
#include "InputLagDiagnosticsMutator.generated.h"

UCLASS(Blueprintable, Meta = (ChildCanTick))
class AInputLagDiagnosticsMutator : public AUTMutator
{
	GENERATED_UCLASS_BODY()

public:
	/** Auto-enable diagnostics for all players when mutator is added */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Lag")
	bool bAutoEnableForAllPlayers;

	virtual void Init_Implementation(const FString& Options) override;
	virtual void BeginPlay() override;
};
