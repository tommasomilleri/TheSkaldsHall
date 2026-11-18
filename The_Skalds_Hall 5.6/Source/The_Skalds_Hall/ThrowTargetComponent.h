#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ThrowTargetComponent.generated.h"

class UTextRenderComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTargetScored, int32, Points, int32, Total);

/** Bersaglio a cerchi: calcola i punti in base alla distanza dal centro. */
UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UThrowTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UThrowTargetComponent();

	UFUNCTION(BlueprintCallable, Category = "Target")
	int32 RegisterHit(const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Target")
	void ResetScore();

	UPROPERTY(BlueprintAssignable, Category = "Target")
	FOnTargetScored OnScored;

	/** Raggi dei cerchi dal centro verso l'esterno (cm). */
	UPROPERTY(EditAnywhere, Category = "Target")
	TArray<float> RingRadii = { 6.f, 14.f, 24.f, 36.f };

	/** Punti per ogni cerchio (stesso ordine dei raggi). */
	UPROPERTY(EditAnywhere, Category = "Target")
	TArray<int32> RingPoints = { 10, 5, 3, 1 };

	/** Numero di lanci per partita (0 = infinito). */
	UPROPERTY(EditAnywhere, Category = "Target")
	int32 ThrowsPerRound = 5;

	UPROPERTY(EditAnywhere, Category = "Target")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, Category = "Target")
	TObjectPtr<USoundBase> BullseyeSound;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Target")
	int32 TotalScore = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Target")
	int32 Throws = 0;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UTextRenderComponent> ScoreText;

	UPROPERTY()
	TObjectPtr<USceneComponent> Center;

	void UpdateText(int32 LastPoints);
};
