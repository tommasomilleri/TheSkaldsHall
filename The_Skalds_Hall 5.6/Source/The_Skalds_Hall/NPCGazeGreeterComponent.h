#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPCGazeGreeterComponent.generated.h"

class AGlassCardActor;
class USoundBase;

/**
 * Attaccalo a un NPC: se il player resta nel suo campo visivo
 * per GazeSecondsRequired secondi (con linea di vista libera),
 * riproduce una battuta audio e/o apre una scheda.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UNPCGazeGreeterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPCGazeGreeterComponent();

	/** Raggio di vista in cm (600 = 6 metri). */
	UPROPERTY(EditAnywhere, Category = "Greeter")
	float SightRadius = 600.f;

	/** Semiampiezza del cono visivo (45 = cono di 90 gradi totali). */
	UPROPERTY(EditAnywhere, Category = "Greeter")
	float SightHalfAngleDeg = 45.f;

	/** Secondi di "contatto visivo" prima di attivarsi. */
	UPROPERTY(EditAnywhere, Category = "Greeter")
	float GazeSecondsRequired = 2.5f;

	/** Battuta audio (asset suono, opzionale). */
	UPROPERTY(EditAnywhere, Category = "Greeter")
	TObjectPtr<USoundBase> VoiceLine;

	/** Scheda da aprire (opzionale). */
	UPROPERTY(EditInstanceOnly, Category = "Greeter")
	TObjectPtr<AGlassCardActor> LinkedCard;

	/** true = si attiva una volta sola. */
	UPROPERTY(EditAnywhere, Category = "Greeter")
	bool bTriggerOnce = true;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	float GazeTime = 0.f;
	bool bTriggered = false;
	bool CanSeePlayer() const;
	void Trigger();
};