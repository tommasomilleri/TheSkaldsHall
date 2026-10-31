#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoricaItemComponent.generated.h"

class UPrimitiveComponent;
class USoundBase;
class USoundAttenuation;
class UNiagaraSystem;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UStoricaItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStoricaItemComponent();

	UPROPERTY(EditAnywhere, Category = "Storica|Feedback")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, Category = "Storica|Feedback")
	TObjectPtr<USoundAttenuation> ImpactAttenuation;

	UPROPERTY(EditAnywhere, Category = "Storica|Feedback")
	TObjectPtr<UNiagaraSystem> ImpactVFX;

	UPROPERTY(EditAnywhere, Category = "Storica|Feedback")
	float MinImpulse = 60.f;

	UPROPERTY(EditAnywhere, Category = "Storica|Feedback")
	float MaxImpulse = 400.f;

	UPROPERTY(EditAnywhere, Category = "Storica|Feedback")
	float ImpactCooldown = 0.25f;

	/** Se true forza la fisica sulla mesh dell'owner al BeginPlay. */
	UPROPERTY(EditAnywhere, Category = "Storica|Physics")
	bool bEnablePhysics = true;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnItemHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse,
		const FHitResult& Hit);

private:
	float LastImpactTime = -1.f;
};