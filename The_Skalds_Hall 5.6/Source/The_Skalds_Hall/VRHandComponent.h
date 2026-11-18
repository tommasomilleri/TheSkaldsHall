#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "VRHandComponent.generated.h"

class UHandGrabComponent;
class UVRHingeComponent;
class UGrabInfoTriggerComponent;
class UMaterialInterface;
class USkeletalMeshComponent;

/**
 * Va messo come figlio di ogni MotionController del VRPawn.
 * Gestisce: presa (oggetti e cerniere), evidenziazione, vibrazione,
 * mano che non entra negli oggetti, chiusura dita, tocco sulle schede.
 */
UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UVRHandComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UVRHandComponent();

	/** Da chiamare quando premi il grip. Ritorna true se ha preso qualcosa. */
	UFUNCTION(BlueprintCallable, Category = "VR|Hand")
	bool TryGrab();

	/** Da chiamare quando rilasci il grip. */
	UFUNCTION(BlueprintCallable, Category = "VR|Hand")
	void Release();

	UFUNCTION(BlueprintPure, Category = "VR|Hand")
	bool IsHolding() const;

	UFUNCTION(BlueprintPure, Category = "VR|Hand")
	float GetGripCurl() const { return CurrentCurl; }

	/** Mesh della mano. Se vuoto usa la prima Skeletal Mesh figlia del MotionController. */
	UPROPERTY(EditAnywhere, Category = "VR|Hand")
	FName HandMeshName;

	/** Distanza massima per afferrare (cm). */
	UPROPERTY(EditAnywhere, Category = "VR|Hand", meta = (ClampMin = "1.0"))
	float GrabRadius = 12.f;

	/** Distanza a cui l'oggetto si illumina (cm). */
	UPROPERTY(EditAnywhere, Category = "VR|Hand", meta = (ClampMin = "1.0"))
	float HighlightRadius = 16.f;

	/** Punto del palmo rispetto al MotionController. */
	UPROPERTY(EditAnywhere, Category = "VR|Hand")
	FVector PalmOffset = FVector(5.f, 0.f, 0.f);

	/** Materiale overlay per indicare che si puo' afferrare (es. M_GrabHighlight). */
	UPROPERTY(EditAnywhere, Category = "VR|Hand")
	TObjectPtr<UMaterialInterface> HighlightMaterial;

	UPROPERTY(EditAnywhere, Category = "VR|Hand")
	bool bHapticsOnHover = true;

	/** La mesh della mano si ferma sulle superfici invece di entrarci. */
	UPROPERTY(EditAnywhere, Category = "VR|Hand")
	bool bBlockHandMesh = true;

	UPROPERTY(EditAnywhere, Category = "VR|Hand", meta = (ClampMin = "0.5"))
	float HandCollisionRadius = 3.5f;

	/** Oltre questa distanza la mano visiva torna di colpo sul controller (cm). */
	UPROPERTY(EditAnywhere, Category = "VR|Hand")
	float MaxHandSeparation = 25.f;

	/** Variabile float nell'Anim Blueprint della mano che riceve la chiusura dita. */
	UPROPERTY(EditAnywhere, Category = "VR|Hand")
	FName GripCurlVariable = TEXT("GripOverride");

	/** Tocco su oggetti fissi con scheda: la apre. */
	UPROPERTY(EditAnywhere, Category = "VR|Hand")
	bool bTouchOpensCards = true;

	UPROPERTY(EditAnywhere, Category = "VR|Hand")
	float TouchRadius = 6.f;

	UPROPERTY(EditAnywhere, Category = "VR|Hand")
	float ThrowVelocityScale = 1.2f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Controller;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> HandMesh;

	UPROPERTY()
	TObjectPtr<UHandGrabComponent> HeldObject;

	UPROPERTY()
	TObjectPtr<UVRHingeComponent> HeldHinge;

	TWeakObjectPtr<UHandGrabComponent> HoverObject;
	TWeakObjectPtr<UVRHingeComponent> HoverHinge;

	FTransform HandMeshRelative;
	FVector VisualHandLocation = FVector::ZeroVector;
	bool bHasVisualHand = false;
	float CurrentCurl = 0.f;

	static constexpr int32 HistorySize = 6;
	FVector PosHistory[HistorySize];
	FQuat RotHistory[HistorySize];
	float TimeHistory[HistorySize];
	int32 HistoryIndex = 0;
	int32 HistoryCount = 0;

	FTimerHandle HapticTimer;

	FVector GetPalm() const;
	void FindCandidates(UHandGrabComponent*& OutObject, UVRHingeComponent*& OutHinge, float Radius) const;
	void UpdateHover();
	void UpdateHandMesh();
	void UpdateCurl(float DeltaTime);
	void UpdateTouch();
	void RecordHistory();
	void ComputeVelocity(FVector& OutLinear, FVector& OutAngular) const;
	void Pulse(float Amplitude, float Duration);
	void StopPulse();
};
