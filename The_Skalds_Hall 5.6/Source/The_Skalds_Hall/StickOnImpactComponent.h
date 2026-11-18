#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickOnImpactComponent.generated.h"

class UStaticMeshComponent;
class USoundBase;

/** Ascia/coltello: se colpisce un bersaglio abbastanza forte e di lama, resta conficcato. */
UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UStickOnImpactComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStickOnImpactComponent();

	/** Tag che deve avere il bersaglio (sull'attore o sul componente colpito). */
	UPROPERTY(EditAnywhere, Category = "Stick")
	FName TargetTag = TEXT("Target");

	UPROPERTY(EditAnywhere, Category = "Stick")
	float MinStickSpeed = 350.f;

	UPROPERTY(EditAnywhere, Category = "Stick")
	float PenetrationDepth = 3.f;

	/** Direzione della lama nello spazio locale della mesh. */
	UPROPERTY(EditAnywhere, Category = "Stick")
	FVector BladeAxis = FVector(1.f, 0.f, 0.f);

	/** -1 = qualsiasi orientamento, 0.3 = la lama deve puntare piu' o meno verso il bersaglio. */
	UPROPERTY(EditAnywhere, Category = "Stick", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MinBladeDot = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Stick")
	TObjectPtr<USoundBase> StickSound;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;

	FVector LastVelocity = FVector::ZeroVector;

	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);
};
