#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrabInfoTriggerComponent.generated.h"
class AGlassCardActor;
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UGrabInfoTriggerComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UGrabInfoTriggerComponent();
	/** Scheda gia' nel livello. Se vuota, ne viene creata una con Card Title / Card Body. */
	UPROPERTY(EditInstanceOnly, Category = "Info Card")
	TObjectPtr<AGlassCardActor> LinkedCard;

	UPROPERTY(EditAnywhere, Category = "Info Card")
	FText CardTitle;

	UPROPERTY(EditAnywhere, Category = "Info Card", meta = (MultiLine = true))
	FText CardBody;

	/** La scheda segue l'oggetto e si posiziona da sola. */
	UPROPERTY(EditAnywhere, Category = "Info Card")
	bool bCardFollowsObject = true;

	UPROPERTY(EditAnywhere, Category = "Info Card")
	bool bCloseOnRelease = true;

	/** Attesa prima di chiudere dopo il rilascio (s). */
	UPROPERTY(EditAnywhere, Category = "Info Card", meta = (ClampMin = "0.0"))
	float CloseDelay = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Info Card|Grab")
	bool bAutoAddHandGrab = true;

	UPROPERTY(EditAnywhere, Category = "Info Card|Proximity")
	bool bProximityWhenNotGrabbable = true;

	UPROPERTY(EditAnywhere, Category = "Info Card|Proximity", meta = (ClampMin = "10.0"))
	float ProximityRadius = 170.f;

	UPROPERTY(EditAnywhere, Category = "Info Card|Proximity", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float ProximityMinViewDot = 0.6f;

	/** Toccare l'oggetto con la mano apre la scheda (anche se fisso). */
	UPROPERTY(EditAnywhere, Category = "Info Card|Touch")
	bool bOpenOnTouch = true;

	/** Per quanti secondi resta aperta dopo l'ultimo tocco. */
	UPROPERTY(EditAnywhere, Category = "Info Card|Touch", meta = (ClampMin = "0.1"))
	float TouchHoldTime = 4.f;

	/** Chiamata da VRHandComponent quando la mano tocca l'oggetto. */
	UFUNCTION(BlueprintCallable, Category = "Info Card")
	void NotifyTouched();

	UFUNCTION(BlueprintPure, Category = "Info Card")
	AGlassCardActor* GetCard() const { return LinkedCard; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
protected:
	virtual void BeginPlay() override;
private:
	bool bWasActive = false;
	bool bUseProximity = false;
	float InactiveTime = 0.f;
	float LastTouchTime = -100.f;
	bool IsHeldNow() const;
	bool IsPlayerNear() const;
	void SpawnCardIfNeeded();
};
