#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InstrumentComponent.generated.h"

class UAudioComponent;
class USoundBase;

/** Strumento musicale: quando lo prendi in mano suona il suo brano, quando lo lasci sfuma. */
UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UInstrumentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInstrumentComponent();

	UPROPERTY(EditAnywhere, Category = "Instrument")
	TObjectPtr<USoundBase> Music;

	UPROPERTY(EditAnywhere, Category = "Instrument", meta = (ClampMin = "0.0"))
	float Volume = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Instrument", meta = (ClampMin = "0.0"))
	float FadeInTime = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Instrument", meta = (ClampMin = "0.0"))
	float FadeOutTime = 1.2f;

	/** Riprende dal punto in cui era rimasto invece che ricominciare. */
	UPROPERTY(EditAnywhere, Category = "Instrument")
	bool bResume = true;

	UFUNCTION(BlueprintCallable, Category = "Instrument")
	void Play();

	UFUNCTION(BlueprintCallable, Category = "Instrument")
	void Stop();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TObjectPtr<UAudioComponent> Audio;

	bool bPlaying = false;
	float PlayStartTime = 0.f;
	float Elapsed = 0.f;

	bool IsHeld() const;
};
