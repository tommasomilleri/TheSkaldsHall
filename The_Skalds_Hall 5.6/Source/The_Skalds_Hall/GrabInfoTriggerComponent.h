#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrabInfoTriggerComponent.generated.h"

class AGlassCardActor;

/**
 * Apre la GlassCard collegata quando l'oggetto viene afferrato da un MotionController.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UGrabInfoTriggerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrabInfoTriggerComponent();

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