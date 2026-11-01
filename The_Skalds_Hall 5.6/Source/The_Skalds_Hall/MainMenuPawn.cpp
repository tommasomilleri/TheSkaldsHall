#include "MainMenuPawn.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* RightClickKeys[] = {
		TEXT("OculusTouch_Right_Trigger_Click"), TEXT("ValveIndex_Right_Trigger_Click"),
		TEXT("Vive_Right_Trigger_Click"), TEXT("MixedReality_Right_Trigger_Click"),
		TEXT("LeftMouseButton")
	};
	const TCHAR* LeftClickKeys[] = {
		TEXT("OculusTouch_Left_Trigger_Click"), TEXT("ValveIndex_Left_Trigger_Click"),
		TEXT("Vive_Left_Trigger_Click"), TEXT("MixedReality_Left_Trigger_Click")
	};
}

AMainMenuPawn::AMainMenuPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;

	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
	SetRootComponent(VROrigin);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VROrigin);
	Camera->bLockToHmd = true;

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VROrigin);
	LeftController->SetTrackingMotionSource(FName("LeftAim"));

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VROrigin);
	RightController->SetTrackingMotionSource(FName("RightAim"));

	LeftInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("LeftInteraction"));
	RightInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("RightInteraction"));
	LeftLaser = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftLaser"));
	RightLaser = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightLaser"));
	LeftCursor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftCursor"));
	RightCursor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightCursor"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	if (CylinderMesh.Succeeded())
	{
		LeftLaser->SetStaticMesh(CylinderMesh.Object);
		RightLaser->SetStaticMesh(CylinderMesh.Object);
	}
	if (SphereMesh.Succeeded())
	{
		LeftCursor->SetStaticMesh(SphereMesh.Object);
		RightCursor->SetStaticMesh(SphereMesh.Object);
	}
	if (BaseMat.Succeeded())
	{
		LaserBaseMaterial = BaseMat.Object;
	}

	SetupHand(RightController, RightInteraction, RightLaser, RightCursor, 0);
	SetupHand(LeftController, LeftInteraction, LeftLaser, LeftCursor, 1);

	ClickRightAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_MenuClickRight"));
	ClickRightAction->ValueType = EInputActionValueType::Boolean;

	ClickLeftAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_MenuClickLeft"));
	ClickLeftAction->ValueType = EInputActionValueType::Boolean;

	MenuMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("IMC_MainMenu"));
	for (const TCHAR* KeyName : RightClickKeys)
	{
		MenuMappingContext->MapKey(ClickRightAction, FKey(KeyName));
	}
	for (const TCHAR* KeyName : LeftClickKeys)
	{
		MenuMappingContext->MapKey(ClickLeftAction, FKey(KeyName));
	}
}

void AMainMenuPawn::SetupHand(UMotionControllerComponent* Controller, UWidgetInteractionComponent* Interaction,
	UStaticMeshComponent* Laser, UStaticMeshComponent* Cursor, int32 PointerIndex)
{
	Interaction->SetupAttachment(Controller);
	Interaction->InteractionDistance = InteractionDistance;
	Interaction->InteractionSource = EWidgetInteractionSource::World;
	Interaction->PointerIndex = PointerIndex;
	Interaction->VirtualUserIndex = PointerIndex;
	Interaction->bShowDebug = false;

	for (UStaticMeshComponent* M : { Laser, Cursor })
	{
		M->SetupAttachment(Interaction);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetCastShadow(false);
		M->SetGenerateOverlapEvents(false);
	}

	Laser->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Cursor->SetRelativeScale3D(FVector(0.03f));
	Cursor->SetVisibility(false);
}

void AMainMenuPawn::BeginPlay()
{
	Super::BeginPlay();
	bHMDActive = UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();

	if (bHMDActive)
	{
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Stage);
		ApplyLaserMaterial(LeftLaser);
		ApplyLaserMaterial(RightLaser);
		ApplyLaserMaterial(LeftCursor);
		ApplyLaserMaterial(RightCursor);
	}
	else
	{
		Camera->bLockToHmd = false;
		Camera->SetRelativeLocation(FVector(0.f, 0.f, DesktopEyeHeight));
		RightInteraction->InteractionSource = EWidgetInteractionSource::Mouse;
		LeftInteraction->SetActive(false);
		LeftInteraction->bEnableHitTesting = false;
		for (UStaticMeshComponent* M : { LeftLaser, RightLaser, LeftCursor, RightCursor })
		{
			M->SetVisibility(false);
		}
	}
}

void AMainMenuPawn::ApplyLaserMaterial(UStaticMeshComponent* Mesh) const
{
	if (!Mesh || !LaserBaseMaterial) return;
	if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(LaserBaseMaterial, const_cast<AMainMenuPawn*>(this)))
	{
		MID->SetVectorParameterValue(TEXT("Color"), LaserColor);
		Mesh->SetMaterial(0, MID);
	}
}

void AMainMenuPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(ClickRightAction, ETriggerEvent::Started, this, &AMainMenuPawn::OnClickRightPressed);
		EIC->BindAction(ClickRightAction, ETriggerEvent::Completed, this, &AMainMenuPawn::OnClickRightReleased);
		EIC->BindAction(ClickLeftAction, ETriggerEvent::Started, this, &AMainMenuPawn::OnClickLeftPressed);
		EIC->BindAction(ClickLeftAction, ETriggerEvent::Completed, this, &AMainMenuPawn::OnClickLeftReleased);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenuPawn] DefaultInputComponentClass non e' EnhancedInputComponent: il click non funzionera'."));
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	if (ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->AddMappingContext(MenuMappingContext, 100);
		}
	}

	if (!bHMDActive)
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
}

void AMainMenuPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bHMDActive) return;

	UpdateLaser(RightInteraction, RightLaser, RightCursor);
	UpdateLaser(LeftInteraction, LeftLaser, LeftCursor);
}

void AMainMenuPawn::UpdateLaser(UWidgetInteractionComponent* Interaction, UStaticMeshComponent* Laser, UStaticMeshComponent* Cursor) const
{
	float Length = InteractionDistance;
	const FHitResult& Hit = Interaction->GetLastHitResult();
	const bool bOverWidget = Interaction->IsOverHitTestVisibleWidget();

	if (Hit.bBlockingHit)
	{
		const float HitDist = static_cast<float>(FVector::Dist(Interaction->GetComponentLocation(), Hit.ImpactPoint));
		Length = FMath::Clamp(HitDist, 1.f, InteractionDistance);
	}

	Laser->SetRelativeLocation(FVector(Length * 0.5f, 0.f, 0.f));
	Laser->SetRelativeScale3D(FVector(LaserThickness / 100.f, LaserThickness / 100.f, Length / 100.f));
	Cursor->SetVisibility(bOverWidget);
	if (bOverWidget)
	{
		Cursor->SetRelativeLocation(FVector(Length, 0.f, 0.f));
	}
}

void AMainMenuPawn::OnClickRightPressed() { RightInteraction->PressPointerKey(EKeys::LeftMouseButton); }
void AMainMenuPawn::OnClickRightReleased() { RightInteraction->ReleasePointerKey(EKeys::LeftMouseButton); }
void AMainMenuPawn::OnClickLeftPressed() { if (bHMDActive) LeftInteraction->PressPointerKey(EKeys::LeftMouseButton); }
void AMainMenuPawn::OnClickLeftReleased() { if (bHMDActive) LeftInteraction->ReleasePointerKey(EKeys::LeftMouseButton); }