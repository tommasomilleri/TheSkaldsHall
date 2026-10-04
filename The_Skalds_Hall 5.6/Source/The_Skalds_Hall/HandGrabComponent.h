#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HandGrabComponent.generated.h"

class UStaticMeshComponent;

/**
 * Grab fisico in C++ puro: si aggiunge all'actor afferrabile.
 * Non dipende da alcun Blueprint esistente.
 *
 * Risolve i 3 difetti del grip procedurale Blueprint:
 *  1. Fisica garantita (profilo PhysicsActor; richiede collisione SIMPLE).
 *  2. Pivot decentrato: lo snap usa il CENTRO dei bounds, non il pivot,
 *     quindi l'oggetto non "galleggia" lontano dal palmo.
 *  3. TraceFingerToObject mira alla superficie reale: DidHit funziona.
 *
 * Compatibile con GrabInfoTriggerComponent: il grab avviene via attach,
 * quindi IsHeldNow() continua a scattare se Hand è un MotionController
 * o un suo figlio.
 */
UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UHandGrabComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHandGrabComponent();

	/** Afferra: disattiva fisica e attacca alla mano (MotionController o figlio). */
	UFUNCTION(BlueprintCallable, Category = "VR|Grab")
	void Grab(USceneComponent* Hand);

	/** Rilascia: stacca, riattiva fisica e applica le velocità (lancio).
	 *  Passa la velocità del controller per il lancio realistico. */
	UFUNCTION(BlueprintCallable, Category = "VR|Grab")
	void Release(FVector LinearVelocity, FVector AngularVelocity);

	UFUNCTION(BlueprintPure, Category = "VR|Grab")
	bool IsGrabbed() const { return HeldBy != nullptr; }

	/**
	 * Trace di un segmento dito verso la SUPERFICIE dell'oggetto.
	 * Mira a Bounds.Origin (centro reale), non al pivot dell'actor.
	 * Usalo nell'AnimBP delle mani al posto di "Trace Finger Segment".
	 */
	UFUNCTION(BlueprintPure, Category = "VR|Grab")
	static bool TraceFingerToObject(USceneComponent* FingerSegment,
		UPrimitiveComponent* Target, float MaxDist, FHitResult& OutHit);

	/** Velocità di aggancio al palmo (interpolazione). */
	UPROPERTY(EditAnywhere, Category = "VR|Grab", meta = (ClampMin = "1.0"))
	float SnapSpeed = 14.f;

	/** Offset del punto di presa rispetto al palmo (spazio locale mano). */
	UPROPERTY(EditAnywhere, Category = "VR|Grab")
	FVector PalmOffset = FVector(5.f, 0.f, 0.f);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<USceneComponent> HeldBy;
	bool bSnapping = false;
};

