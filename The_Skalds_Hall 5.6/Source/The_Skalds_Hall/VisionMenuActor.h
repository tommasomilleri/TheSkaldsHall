#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VisionMenuActor.generated.h"
class UWidgetComponent;
class UWidgetInteractionComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UInputAction;
class UInputMappingContext;
UCLASS()
class THE_SKALDS_HALL_API AVisionMenuActor : public AActor
{
	GENERATED_BODY()
public:
	AVisionMenuActor();
	UPROPERTY(VisibleAnywhere, Category = "Menu")
	TObjectPtr<UWidgetComponent> MenuWidget;

	UPROPERTY(EditAnywhere, Category = "Menu")
	float DistanceFromPlayer = 120.f;

	UPROPERTY(EditAnywhere, Category = "Menu")
	float FollowSpeed = 5.f;

	UPROPERTY(EditAnywhere, Category = "Menu", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float RecenterAngleDeg = 40.f;

	UPROPERTY(EditAnywhere, Category = "Menu|Laser", meta = (ClampMin = "0.05"))
	float LaserThickness = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Menu|Laser")
	FLinearColor LaserColor = FLinearColor(0.85f, 0.75f, 0.45f, 1.f);

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ToggleMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OpenMenu();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void CloseMenu();

	UFUNCTION(BlueprintPure, Category = "Menu")
	bool IsMenuOpen() const { return bOpen; }
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
private:
	UPROPERTY(VisibleAnywhere, Category = "Menu|Laser")
	TObjectPtr<UStaticMeshComponent> LaserLeft;
	UPROPERTY(VisibleAnywhere, Category = "Menu|Laser")
	TObjectPtr<UStaticMeshComponent> LaserRight;

	UPROPERTY() TObjectPtr<UMaterialInterface> LaserBaseMaterial;
	UPROPERTY() TObjectPtr<UInputAction> InteractLeftAction;
	UPROPERTY() TObjectPtr<UInputAction> InteractRightAction;
	UPROPERTY() TObjectPtr<UInputMappingContext> MenuMappingContext;
	UPROPERTY(Transient) TObjectPtr<UWidgetInteractionComponent> PointerLeft;
	UPROPERTY(Transient) TObjectPtr<UWidgetInteractionComponent> PointerRight;

	bool bOpen = false;
	bool bInputBound = false;
	bool bRepositioning = false;

	void SetMenuOpen(bool bNewOpen);
	void BindMenuInput();
	void FindPointers();
	void ReleasePointers();
	void UpdateLaser(const UWidgetInteractionComponent* Pointer, UStaticMeshComponent* Laser) const;

	void OnLeftPressed();
	void OnLeftReleased();
	void OnRightPressed();
	void OnRightReleased();

	bool GetPlayerView(FVector& OutLocation, FRotator& OutRotation) const;
	FVector ComputeTargetLocation() const;
};