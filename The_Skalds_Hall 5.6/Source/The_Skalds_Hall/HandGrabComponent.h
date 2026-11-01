#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HandGrabComponent.generated.h"

class UStaticMeshComponent;
class UPrimitiveComponent;

/**
 * Grab fisico in C++ puro per oggetti afferrabili in VR.
 */
UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UHandGrabComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHandGrabComponent();

	UFUNCTION(BlueprintCallable, Category = "VR|Grab")
	void Grab(USceneComponent* Hand);

	UFUNCTION(BlueprintCallable, Category = "VR|Grab")
	void Release(FVector LinearVelocity, FVector AngularVelocity);

	UFUNCTION(BlueprintPure, Category = "VR|Grab")
	bool IsGrabbed() const { return HeldBy != nullptr; }

	UFUNCTION(BlueprintPure, Category = "VR|Grab")
	static bool TraceFingerToObject(USceneComponent* FingerSegment,
		UPrimitiveComponent* Target, float MaxDist, FHitResult& OutHit);

	UPROPERTY(EditAnywhere, Category = "VR|Grab", meta = (ClampMin = "1.0"))
	float SnapSpeed = 14.f;

	UPROPERTY(EditAnywhere, Category = "VR|Grab")
	FVector PalmOffset = FVector(5.f, 0.f, 0.f);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<USceneComponent> HeldBy;

	bool bSnapping = false;
};