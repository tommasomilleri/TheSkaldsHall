#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoricaItem.generated.h"
class UStaticMeshComponent;
class USoundBase;
class UNiagaraSystem;
UCLASS()
class THE_SKALDS_HALL_API AStoricaItem : public AActor
{
	GENERATED_BODY()
public:
	AStoricaItem();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storica")
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, Category = "Storica|Feedback")
	USoundBase* ImpactSound;

	UPROPERTY(EditAnywhere, Category = "Storica|Feedback")
	UNiagaraSystem* ImpactVFX;

	UPROPERTY(EditAnywhere, Category = "Storica|Feedback")
	float MinImpulse = 150.f;

	UPROPERTY(EditAnywhere, Category = "Storica|Feedback")
	float MaxImpulse = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Storica|Feedback")
	float ImpactCooldown = 0.25f;
protected:
	virtual void BeginPlay() override;
	UFUNCTION()
	void OnItemHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse,
		const FHitResult& Hit);
private:
	float LastImpactTime = -1.f;
};