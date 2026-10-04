#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VRTeleportComponent.generated.h"

/**
 * Teleport ad arco parabolico con "blink" alla Half-Life: Alyx
 * (fade nero rapidissimo durante lo spostamento: la tecnica
 * anti-motion-sickness più efficace esistente).
 *
 * Da aggiungere al Pawn del giocatore. Richiede:
 *  - "NavigationSystem" nel Build.cs
 *  - un NavMeshBoundsVolume nel livello (altrimenti nessuna
 *    destinazione risulterà mai valida).
 */
UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UVRTeleportComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVRTeleportComponent();

	/** Inizia a mirare (chiamalo quando spingi lo stick avanti). */
	UFUNCTION(BlueprintCallable, Category = "VR|Teleport")
	void StartAiming(USceneComponent* AimSource);

	/** Conferma: blink + teleport. SnapYaw = rotazione snap pre-teleport
	 *  (es. direzione del thumbstick al rilascio). */
	UFUNCTION(BlueprintCallable, Category = "VR|Teleport")
	void ConfirmTeleport(float SnapYaw = 0.f);

	/** Annulla la mira senza teleportare. */
	UFUNCTION(BlueprintCallable, Category = "VR|Teleport")
	void CancelAiming();

	UFUNCTION(BlueprintPure, Category = "VR|Teleport")
	bool HasValidDestination() const { return bHasValidDest; }

	/** Velocità iniziale dell'arco (cm/s). Più alta = arco più lungo. */
	UPROPERTY(EditAnywhere, Category = "VR|Teleport", meta = (ClampMin = "100.0"))
	float LaunchSpeed = 900.f;

	/** Durata fade a nero (blink out). Alyx usa ~0.1s. */
	UPROPERTY(EditAnywhere, Category = "VR|Teleport", meta = (ClampMin = "0.01"))
	float FadeOutTime = 0.1f;

	/** Durata ritorno dal nero (blink in). */
	UPROPERTY(EditAnywhere, Category = "VR|Teleport", meta = (ClampMin = "0.01"))
	float FadeInTime = 0.15f;

	/** Tolleranza di proiezione sulla NavMesh (cm). */
	UPROPERTY(EditAnywhere, Category = "VR|Teleport")
	FVector NavProjectionExtent = FVector(50.f, 50.f, 100.f);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY() TObjectPtr<USceneComponent> Source;
	FVector ValidDestination = FVector::ZeroVector;
	bool bHasValidDest = false;
	bool bTeleportInProgress = false;

	void DoTeleport(float SnapYaw);
	FTimerHandle FadeTimer;
};

