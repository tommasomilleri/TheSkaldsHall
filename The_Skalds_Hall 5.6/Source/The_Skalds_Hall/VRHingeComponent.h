#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "VRHingeComponent.generated.h"

class UMaterialInterface;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHingeAngleChanged, float, NormalizedAngle);

/**
 * Perno afferrabile: porta, leva, rubinetto.
 * Tutto cio' che e' attaccato sotto questo componente ruota attorno al suo asse Z locale.
 * La maniglia e' un componente figlio con tag "Handle".
 */
UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UVRHingeComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UVRHingeComponent();

	UFUNCTION(BlueprintCallable, Category = "VR|Hinge")
	void Grab(USceneComponent* Hand);

	UFUNCTION(BlueprintCallable, Category = "VR|Hinge")
	void Release();

	UFUNCTION(BlueprintPure, Category = "VR|Hinge")
	bool IsGrabbed() const { return HeldBy != nullptr; }

	UFUNCTION(BlueprintPure, Category = "VR|Hinge")
	float GetAngle() const { return CurrentAngle; }

	/** 0 = MinAngle, 1 = MaxAngle. */
	UFUNCTION(BlueprintPure, Category = "VR|Hinge")
	float GetNormalizedAngle() const;

	UFUNCTION(BlueprintCallable, Category = "VR|Hinge")
	void SetAngle(float NewAngle);

	FVector GetHandleLocation() const;

	void SetHighlighted(bool bOn, UMaterialInterface* Material);

	UPROPERTY(BlueprintAssignable, Category = "VR|Hinge")
	FOnHingeAngleChanged OnAngleChanged;

	UPROPERTY(EditAnywhere, Category = "VR|Hinge")
	float MinAngle = 0.f;

	UPROPERTY(EditAnywhere, Category = "VR|Hinge")
	float MaxAngle = 100.f;

	/** Distanza mano-maniglia entro cui si puo' afferrare (cm). */
	UPROPERTY(EditAnywhere, Category = "VR|Hinge", meta = (ClampMin = "1.0"))
	float GrabRadius = 15.f;

	/** Se la mano si allontana oltre questa distanza dalla maniglia, la presa si rilascia (cm). */
	UPROPERTY(EditAnywhere, Category = "VR|Hinge", meta = (ClampMin = "5.0"))
	float BreakDistance = 40.f;

	/** Porta: continua a girare per inerzia dopo il rilascio. */
	UPROPERTY(EditAnywhere, Category = "VR|Hinge")
	bool bKeepMomentum = true;

	UPROPERTY(EditAnywhere, Category = "VR|Hinge", meta = (ClampMin = "0.0"))
	float Friction = 3.f;

	/** Leva/rubinetto: torna da solo a RestAngle quando lo lasci. */
	UPROPERTY(EditAnywhere, Category = "VR|Hinge")
	bool bReturnToRest = false;

	UPROPERTY(EditAnywhere, Category = "VR|Hinge")
	float RestAngle = 0.f;

	UPROPERTY(EditAnywhere, Category = "VR|Hinge", meta = (ClampMin = "0.1"))
	float ReturnSpeed = 6.f;

	UPROPERTY(EditAnywhere, Category = "VR|Hinge|Sound")
	TObjectPtr<USoundBase> MoveSound;

	UPROPERTY(EditAnywhere, Category = "VR|Hinge|Sound")
	TObjectPtr<USoundBase> LimitSound;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> HeldBy;

	FQuat BaseRelativeRotation = FQuat::Identity;
	float CurrentAngle = 0.f;
	float AngularVelocity = 0.f;
	float GrabHandAngle = 0.f;
	float GrabStartAngle = 0.f;
	float MovedSinceSound = 0.f;
	bool bAtLimit = false;

	FTransform GetPivotFrame() const;
	float HandAngle(const FVector& WorldPos) const;
	void ApplyAngle(float NewAngle);
};
