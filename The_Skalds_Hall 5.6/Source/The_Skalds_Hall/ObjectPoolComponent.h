#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ObjectPoolComponent.generated.h"

/**
 * Pool di attori pre-istanziati per evitare spike di Garbage Collection in VR.
 */
UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UObjectPoolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Pool")
	TSubclassOf<AActor> PooledClass;

	UPROPERTY(EditAnywhere, Category = "Pool", meta = (ClampMin = "1", ClampMax = "256"))
	int32 PoolSize = 24;

	UFUNCTION(BlueprintCallable, Category = "Pool")
	AActor* Acquire(const FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable, Category = "Pool")
	void Release(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "Pool")
	int32 NumAvailable() const { return Available.Num(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TArray<TObjectPtr<AActor>> Available;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> All;

	void SetActive(AActor* Actor, bool bActive) const;
};