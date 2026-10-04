#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlassCardActor.generated.h"

class UWidgetComponent;

/**
 * Scheda "fisica" nel mondo: fluttua, guarda il player,
 * si apre/chiude con animazione di scala.
 */
UCLASS()
class THE_SKALDS_HALL_API AGlassCardActor : public AActor
{
	GENERATED_BODY()

public:
	AGlassCardActor();

	// --- Modificabili nel pannello Details dell'editor ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card", meta = (MultiLine = true))
	FText Body;

	/** Ampiezza dell'ondeggiamento in cm. 0 = fermo. */
	UPROPERTY(EditAnywhere, Category = "Card|Motion")
	float FloatAmplitude = 4.f;

	UPROPERTY(EditAnywhere, Category = "Card|Motion")
	float FloatSpeed = 1.2f;

	/** Ruota per guardare sempre il player. */
	UPROPERTY(EditAnywhere, Category = "Card|Motion")
	bool bFacePlayer = true;

	/** Parte invisibile: si apre solo chiamando Open(). */
	UPROPERTY(EditAnywhere, Category = "Card")
	bool bStartClosed = false;

	// --- Chiamabili da codice (o Blueprint se servisse) ---

	UFUNCTION(BlueprintCallable, Category = "Card")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "Card")
	void Close();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWidgetComponent> CardWidget;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> GlassPanel;

	/** Materiale M_LiquidGlass (vedi note in fondo al file). */
	UPROPERTY(EditAnywhere, Category = "Card|Style")
	TObjectPtr<UMaterialInterface> GlassMaterial;

private:
	float BaseZ = 0.f;         // quota di partenza per l'ondeggiamento
	float FloatTime = 0.f;     // accumulatore per il seno
	bool bWasHeldLastTick = false;
	float TargetScale = 1.f;   // scala verso cui animiamo
	float CurrentScale = 1.f;  // scala attuale
	bool IsHeld() const;       // è in mano al player?
};