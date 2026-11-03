#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MainMenuPawn.generated.h"
class UCameraComponent;
class UMotionControllerComponent;
class UWidgetInteractionComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class UMaterialInterface;
/**
- Pawn del menu principale (livello MenuPrincipale).
- Camera HMD + due motion controller (pose "Aim") con WidgetInteractionComponent
- e laser; il grilletto clicca il widget 3D. Senza visore usa il mouse.
- Input costruito in C++ (Enhanced Input): nessun asset richiesto.
   */
UCLASS()
class THE_SKALDS_HALL_API AMainMenuPawn : public APawn
{
	GENERATED_BODY()
public:
	AMainMenuPawn();
	UPROPERTY(VisibleAnywhere, Category = "VR") TObjectPtr<USceneComponent> VROrigin;
	UPROPERTY(VisibleAnywhere, Category = "VR") TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, Category = "VR") TObjectPtr<UMotionControllerComponent> LeftController;
	UPROPERTY(VisibleAnywhere, Category = "VR") TObjectPtr<UMotionControllerComponent> RightController;
	UPROPERTY(VisibleAnywhere, Category = "VR") TObjectPtr<UWidgetInteractionComponent> LeftInteraction;
	UPROPERTY(VisibleAnywhere, Category = "VR") TObjectPtr<UWidgetInteractionComponent> RightInteraction;
	UPROPERTY(VisibleAnywhere, Category = "VR") TObjectPtr<UStaticMeshComponent> LeftLaser;
	UPROPERTY(VisibleAnywhere, Category = "VR") TObjectPtr<UStaticMeshComponent> RightLaser;
	UPROPERTY(VisibleAnywhere, Category = "VR") TObjectPtr<UStaticMeshComponent> LeftCursor;
	UPROPERTY(VisibleAnywhere, Category = "VR") TObjectPtr<UStaticMeshComponent> RightCursor;

	/** Portata del puntatore in cm. */
	UPROPERTY(EditAnywhere, Category = "Menu")
	float InteractionDistance = 1000.f;

	/** Spessore del laser in cm. */
	UPROPERTY(EditAnywhere, Category = "Menu", meta = (ClampMin = "0.05"))
	float LaserThickness = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Menu")
	FLinearColor LaserColor = FLinearColor(0.85f, 0.75f, 0.45f, 1.f);

	/** Altezza camera quando NON c'e' un visore (test desktop). */
	UPROPERTY(EditAnywhere, Category = "Menu")
	float DesktopEyeHeight = 160.f;

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
protected:
	virtual void BeginPlay() override;
private:
	UPROPERTY() TObjectPtr<UInputAction> ClickRightAction;
	UPROPERTY() TObjectPtr<UInputAction> ClickLeftAction;
	UPROPERTY() TObjectPtr<UInputMappingContext> MenuMappingContext;
	UPROPERTY() TObjectPtr<UMaterialInterface> LaserBaseMaterial;
	bool bHMDActive = false;

	void OnClickRightPressed();
	void OnClickRightReleased();
	void OnClickLeftPressed();
	void OnClickLeftReleased();

	// "HandController" e non "Controller": quel nome e' gia' un membro di APawn (errore C4458).
	void SetupHand(UMotionControllerComponent* HandController, UWidgetInteractionComponent* Interaction,
		UStaticMeshComponent* Laser, UStaticMeshComponent* Cursor, int32 PointerIndex);
	void UpdateLaser(UWidgetInteractionComponent* Interaction, UStaticMeshComponent* Laser, UStaticMeshComponent* Cursor) const;
	void ApplyLaserMaterial(UStaticMeshComponent* Mesh) const;
};