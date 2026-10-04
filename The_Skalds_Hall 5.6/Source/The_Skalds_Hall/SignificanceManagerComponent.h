#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SignificanceManagerComponent.generated.h"

class USkeletalMeshComponent;


UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API USignificanceManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USignificanceManagerComponent();

	UPROPERTY(EditAnywhere, Category = "Significance", meta = (ClampMin = "100.0"))
	float MaxSignificantDistance = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Significance", meta = (ClampMin = "0"))
	int32 InsignificantLOD = 3;

	UPROPERTY(EditAnywhere, Category = "Significance",
		meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MinViewDot = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> SkelMesh;
	bool bWasSignificant = true;
	void ApplySignificance(bool bSignificant);
};
