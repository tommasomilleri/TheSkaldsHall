#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ObjectPoolComponent.generated.h"

/**
 * Pool di attori pre-istanziati (cibo, corni, frecce, monete).
 * Elimina gli spike di Garbage Collection da SpawnActor/DestroyActor,
 * che in VR causano judder percepibile (frame persi = nausea).
 *
 * USO: Acquire() al posto di SpawnActor, Release() al posto di Destroy.
 * Gli attori restituiti NON vanno mai distrutti manualmente.
 */
UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UObjectPoolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Classe degli attori nel pool. */
	UPROPERTY(EditAnywhere, Category = "Pool")
	TSubclassOf<AActor> PooledClass;

	/** Numero di attori pre-istanziati a BeginPlay. */
	UPROPERTY(EditAnywhere, Category = "Pool", meta = (ClampMin = "1", ClampMax = "256"))
	int32 PoolSize = 24;

	/** Preleva un attore dal pool (nullptr se esaurito: MAI spawn extra). */
	UFUNCTION(BlueprintCallable, Category = "Pool")
	AActor* Acquire(const FTransform& SpawnTransform);

	/** Restituisce un attore al pool (lo disattiva e lo nasconde). */
	UFUNCTION(BlueprintCallable, Category = "Pool")
	void Release(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "Pool")
	int32 NumAvailable() const { return Available.Num(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY() TArray<TObjectPtr<AActor>> Available;
	UPROPERTY() TArray<TObjectPtr<AActor>> All; // per il cleanup finale
	void SetActive(AActor* Actor, bool bActive) const;
};
