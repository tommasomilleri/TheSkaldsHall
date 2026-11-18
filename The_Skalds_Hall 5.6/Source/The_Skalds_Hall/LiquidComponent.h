#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LiquidComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UAudioComponent;
class USoundBase;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UVRHingeComponent;

/**
 * Liquido simulato (senza fluidodinamica): contenitori che si riempiono e si svuotano.
 * - Botte: bInfiniteSource = true + un VRHingeComponent (rubinetto) che regola il flusso.
 * - Brocca/boccale: versa quando lo inclini, si riempie se ci versi dentro.
 * Componenti con tag: "Liquid" = mesh del liquido interno, "Spout" = punto da cui esce.
 */
UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API ULiquidComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULiquidComponent();

	UFUNCTION(BlueprintCallable, Category = "Liquid")
	void AddLiquid(float Milliliters);

	UFUNCTION(BlueprintPure, Category = "Liquid")
	float GetFill() const { return Fill; }

	UFUNCTION(BlueprintPure, Category = "Liquid")
	bool IsPouring() const { return CurrentFlow > 0.f; }

	/** 0 vuoto, 1 pieno. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Liquid", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Fill = 0.f;

	UPROPERTY(EditAnywhere, Category = "Liquid", meta = (ClampMin = "1.0"))
	float CapacityMl = 1000.f;

	/** Botte: non si svuota mai, non riceve liquido. */
	UPROPERTY(EditAnywhere, Category = "Liquid")
	bool bInfiniteSource = false;

	UPROPERTY(EditAnywhere, Category = "Liquid", meta = (ClampMin = "1.0"))
	float PourRateMlPerSec = 300.f;

	/** Sotto questa apertura del rubinetto (0-1) non esce nulla. */
	UPROPERTY(EditAnywhere, Category = "Liquid", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinTapOpening = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Liquid")
	FLinearColor LiquidColor = FLinearColor(0.45f, 0.18f, 0.03f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Liquid|FX")
	TObjectPtr<UNiagaraSystem> PourEffect;

	UPROPERTY(EditAnywhere, Category = "Liquid|FX")
	TObjectPtr<USoundBase> PourSound;

	/** Lunghezza massima del getto (cm). */
	UPROPERTY(EditAnywhere, Category = "Liquid|FX")
	float MaxStreamLength = 150.f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> LiquidMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> LiquidMID;

	UPROPERTY()
	TObjectPtr<USceneComponent> Spout;

	UPROPERTY()
	TObjectPtr<UVRHingeComponent> Tap;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> StreamFX;

	UPROPERTY()
	TObjectPtr<UAudioComponent> StreamAudio;

	float CurrentFlow = 0.f;
	bool bStreamOn = false;

	FVector GetSpoutLocation() const;
	float ComputeFlow() const;
	void UpdateVisual();
};
