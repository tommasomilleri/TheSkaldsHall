#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlassCardActor.generated.h"
class UWidgetComponent;
class UStaticMeshComponent;
class UMaterialInterface;
UCLASS()
class THE_SKALDS_HALL_API AGlassCardActor : public AActor
{
	GENERATED_BODY()
public:
	AGlassCardActor();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card", meta = (MultiLine = true))
	FText Body;

	UPROPERTY(EditAnywhere, Category = "Card")
	bool bStartClosed = false;

	/** Oggetto da seguire. Lo imposta da solo il Grab Info Trigger che usa questa scheda. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Card|Follow")
	TObjectPtr<AActor> FollowTarget;

	UPROPERTY(EditAnywhere, Category = "Card|Follow")
	float HeightAboveObject = 12.f;

	UPROPERTY(EditAnywhere, Category = "Card|Follow")
	float TowardPlayerOffset = 10.f;

	UPROPERTY(EditAnywhere, Category = "Card|Follow")
	float SideOffsetWhenHeld = 22.f;

	UPROPERTY(EditAnywhere, Category = "Card|Follow")
	float HeightWhenHeld = -6.f;

	/** Piu' basso = piu' reattiva, piu' alto = piu' morbida (secondi). */
	UPROPERTY(EditAnywhere, Category = "Card|Follow", meta = (ClampMin = "0.02", ClampMax = "1.0"))
	float FollowSmoothTime = 0.12f;

	/** Oltre questa distanza (teleport, lancio lungo) la scheda salta subito sul posto (cm). */
	UPROPERTY(EditAnywhere, Category = "Card|Follow")
	float SnapDistance = 400.f;

	UPROPERTY(EditAnywhere, Category = "Card|Style", meta = (ClampMin = "10.0"))
	float CardWorldWidth = 36.f;

	/** Pixel di larghezza: piu' alto = testo piu' nitido. */
	UPROPERTY(EditAnywhere, Category = "Card|Style", meta = (ClampMin = "300.0"))
	float CardPixelWidth = 900.f;

	UPROPERTY(EditAnywhere, Category = "Card|Style", meta = (ClampMin = "8.0"))
	float TitleFontSize = 34.f;

	UPROPERTY(EditAnywhere, Category = "Card|Style", meta = (ClampMin = "8.0"))
	float BodyFontSize = 23.f;

	UPROPERTY(EditAnywhere, Category = "Card|Style", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BackgroundOpacity = 0.72f;

	/** Ondeggiamento (cm). Disattivato quando l'oggetto e' in mano. */
	UPROPERTY(EditAnywhere, Category = "Card|Motion")
	float FloatAmplitude = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Card|Motion")
	float FloatSpeed = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Card|Motion")
	bool bFacePlayer = true;

	/** Inclinazione massima verso gli occhi (gradi). */
	UPROPERTY(EditAnywhere, Category = "Card|Motion", meta = (ClampMin = "0.0", ClampMax = "60.0"))
	float MaxPitch = 30.f;

	UFUNCTION(BlueprintCallable, Category = "Card")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "Card")
	void Close();

	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetFollowTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetTexts(const FText& NewTitle, const FText& NewBody);

	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWidgetComponent> CardWidget;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> GlassPanel;

	UPROPERTY(EditAnywhere, Category = "Card|Style")
	TObjectPtr<UMaterialInterface> GlassMaterial;
private:
	float BaseZ = 0.f;
	float FloatTime = 0.f;
	float TargetScale = 1.f;
	float CurrentScale = 1.f;
	float HeldBlend = 0.f;
	float HeldSide = 1.f;
	FVector FollowVelocity = FVector::ZeroVector;
	FVector FollowPosition = FVector::ZeroVector;
	bool bHasFollowPosition = false;
	void BuildWidget();
	void UpdatePanelSize();
	bool IsTargetHeld() const;
	bool ComputeFollowAnchor(FVector& OutAnchor, float DeltaTime);
	void SnapToTarget();
};