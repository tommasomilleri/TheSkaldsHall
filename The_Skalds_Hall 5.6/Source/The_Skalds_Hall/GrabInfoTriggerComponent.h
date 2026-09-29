#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrabInfoTriggerComponent.generated.h"

class AGlassCardActor;

/**
 * Attaccalo a un oggetto afferrabile: quando l'oggetto finisce
 * in mano al player (attaccato a un MotionController), apre
 * la scheda collegata. Al rilascio la richiude (opzionale).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UGrabInfoTriggerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrabInfoTriggerComponent();

	/** La scheda da aprire: assegnala nel Details (picker con la pipetta). */
	UPROPERTY(EditInstanceOnly, Category = "Info Card")
	TObjectPtr<AGlassCardActor> LinkedCard;

	UPROPERTY(EditAnywhere, Category = "Info Card")
	bool bCloseOnRelease = true;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool bWasHeld = false;
	bool IsHeldNow() const;
};