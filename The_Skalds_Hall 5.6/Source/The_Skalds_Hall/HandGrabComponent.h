#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HandGrabComponent.generated.h"

class UStaticMeshComponent;
class UPrimitiveComponent;
class UMaterialInterface;
class USoundBase;

UENUM(BlueprintType)
enum class EHandGrabMode : uint8
{
	KeepOffset	UMETA(DisplayName = "Keep Offset"),
	GripPoint	UMETA(DisplayName = "Grip Point"),
	SnapCenter	UMETA(DisplayName = "Snap Center")
};

UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UHandGrabComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHandGrabComponent();

	UFUNCTION(BlueprintCallable, Category = "VR|Grab")
	void Grab(USceneComponent* Hand);

	UFUNCTION(BlueprintCallable, Category = "VR|Grab")
	void Release(FVector LinearVelocity, FVector AngularVelocity);

	UFUNCTION(BlueprintPure, Category = "VR|Grab")
	bool IsGrabbed() const { return HeldBy != nullptr; }

	/** Quanto chiudere le dita (0 aperta, 1 pugno) per la presa attuale. */
	UFUNCTION(BlueprintPure, Category = "VR|Grab")
	float GetGripCurl() const { return ActiveGripCurl; }

	UFUNCTION(BlueprintCallable, Category = "VR|Grab")
	void SetHighlighted(bool bOn, UMaterialInterface* Material);

	/** Distanza dalla superficie di collisione (cm). -1 se non calcolabile. */
	float GetDistanceToSurface(const FVector& Point) const;

	UStaticMeshComponent* GetMesh() const { return Mesh; }

	/** Mesh principale dell'attore: la root se e' una Static Mesh, altrimenti la prima senza tag "Liquid". */
	static UStaticMeshComponent* FindMainMesh(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "VR|Grab")
	static bool TraceFingerToObject(USceneComponent* FingerSegment,
		UPrimitiveComponent* Target, float MaxDist, FHitResult& OutHit);

	// ---------- Presa ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Grab")
	EHandGrabMode GrabMode;

	UPROPERTY(EditAnywhere, Category = "VR|Grab")
	bool bAlignGripRotation = true;

	UPROPERTY(EditAnywhere, Category = "VR|Grab", meta = (ClampMin = "0.0"))
	float SurfaceGap = 2.5f;

	UPROPERTY(EditAnywhere, Category = "VR|Grab")
	bool bIgnorePawnWhileHeld = true;

	UPROPERTY(EditAnywhere, Category = "VR|Grab", meta = (ClampMin = "1.0"))
	float SnapSpeed = 14.f;

	UPROPERTY(EditAnywhere, Category = "VR|Grab")
	FVector PalmOffset = FVector(5.f, 0.f, 0.f);

	/** Chiusura dita se il Grip Point non ha il tag "Curl=x". */
	UPROPERTY(EditAnywhere, Category = "VR|Grab", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DefaultGripCurl = 0.85f;

	// ---------- Stabilita' fisica (anti-esplosione) ----------
	UPROPERTY(EditAnywhere, Category = "VR|Physics")
	bool bStartAsleep = true;

	UPROPERTY(EditAnywhere, Category = "VR|Physics", meta = (ClampMin = "0.01"))
	float MinMassKg = 0.3f;

	/** Velocita' massima con cui il motore separa oggetti compenetrati (cm/s). */
	UPROPERTY(EditAnywhere, Category = "VR|Physics", meta = (ClampMin = "0.0"))
	float MaxDepenetrationVelocity = 40.f;

	UPROPERTY(EditAnywhere, Category = "VR|Physics")
	bool bUseCCD = true;

	UPROPERTY(EditAnywhere, Category = "VR|Physics")
	float LinearDamping = 0.1f;

	UPROPERTY(EditAnywhere, Category = "VR|Physics")
	float AngularDamping = 0.6f;

	// ---------- Ritorno alla posizione originale ----------
	UPROPERTY(EditAnywhere, Category = "VR|Return")
	bool bReturnHome = false;

	UPROPERTY(EditAnywhere, Category = "VR|Return", meta = (ClampMin = "0.0"))
	float ReturnDelay = 3.f;

	UPROPERTY(EditAnywhere, Category = "VR|Return", meta = (ClampMin = "0.05"))
	float ReturnTime = 0.6f;

	// ---------- Suoni ----------
	UPROPERTY(EditAnywhere, Category = "VR|Sound")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, Category = "VR|Sound")
	TObjectPtr<USoundBase> PickupSound;

	UPROPERTY(EditAnywhere, Category = "VR|Sound")
	float MinImpactSpeed = 40.f;

	UPROPERTY(EditAnywhere, Category = "VR|Sound")
	float FullVolumeSpeed = 450.f;

	UPROPERTY(EditAnywhere, Category = "VR|Sound")
	float ImpactCooldown = 0.12f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<USceneComponent> HeldBy;

	bool bSnapping = false;
	bool bReturning = false;
	float ReturnAlpha = 0.f;
	float ActiveGripCurl = 0.85f;
	float LastImpactTime = -10.f;
	int32 HighlightCount = 0;
	FTransform TargetRelative;
	FTransform HomeTransform;
	FTransform ReturnStart;
	FTimerHandle ReturnTimer;
	TEnumAsByte<ECollisionResponse> SavedPawnResponse = ECR_Block;

	USceneComponent* FindClosestGrip(const FVector& Palm) const;
	FTransform ComputeTargetWorld(const FVector& Palm, const FQuat& HandRot, USceneComponent*& OutGrip) const;
	void StartReturn();

	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);
};
