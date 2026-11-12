#include "VisionMenuActor.h"
#include "VisionMenuWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/LocalPlayer.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MotionControllerComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AVisionMenuActor::AVisionMenuActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	MenuWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("MenuWidget"));
	MenuWidget->SetupAttachment(Root);
	MenuWidget->SetWidgetClass(UVisionMenuWidget::StaticClass());
	MenuWidget->SetDrawSize(FVector2D(760.f, 700.f));
	MenuWidget->SetTwoSided(true);
	MenuWidget->SetWorldScale3D(FVector(0.07f));
	MenuWidget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MenuWidget->SetCollisionProfileName(TEXT("UI"));
	// I WidgetInteraction del VRPawn tracciano su ECC_GameTraceChannel1 (3DWidget del VR Template).
	MenuWidget->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	MenuWidget->SetWindowFocusable(true);
	MenuWidget->SetBlendMode(EWidgetBlendMode::Transparent);
	MenuWidget->SetBackgroundColor(FLinearColor::Transparent);

	LaserLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserLeft"));
	LaserRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserRight"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMat.Succeeded()) LaserBaseMaterial = BaseMat.Object;

	for (UStaticMeshComponent* Laser : { LaserLeft.Get(), LaserRight.Get() })
	{
		Laser->SetupAttachment(Root);
		if (CylinderMesh.Succeeded()) Laser->SetStaticMesh(CylinderMesh.Object);
		Laser->SetUsingAbsoluteLocation(true);
		Laser->SetUsingAbsoluteRotation(true);
		Laser->SetUsingAbsoluteScale(true);
		Laser->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Laser->SetCastShadow(false);
		Laser->SetGenerateOverlapEvents(false);
		Laser->SetVisibility(false);
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MenuIMC(TEXT("/Game/VRTemplate/Input/IMC_Menu.IMC_Menu"));
	static ConstructorHelpers::FObjectFinder<UInputAction> InteractLeft(TEXT("/Game/VRTemplate/Input/Actions/IA_Menu_Interact_Left.IA_Menu_Interact_Left"));
	static ConstructorHelpers::FObjectFinder<UInputAction> InteractRight(TEXT("/Game/VRTemplate/Input/Actions/IA_Menu_Interact_Right.IA_Menu_Interact_Right"));
	if (MenuIMC.Succeeded()) MenuMappingContext = MenuIMC.Object;
	if (InteractLeft.Succeeded()) InteractLeftAction = InteractLeft.Object;
	if (InteractRight.Succeeded()) InteractRightAction = InteractRight.Object;
}

void AVisionMenuActor::BeginPlay()
{
	Super::BeginPlay();

	for (UStaticMeshComponent* Laser : { LaserLeft.Get(), LaserRight.Get() })
	{
		if (!LaserBaseMaterial) break;
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(LaserBaseMaterial, Laser))
		{
			MID->SetVectorParameterValue(TEXT("Color"), LaserColor);
			Laser->SetMaterial(0, MID);
		}
	}

	bOpen = true;
	SetMenuOpen(false);
}

void AVisionMenuActor::ToggleMenu()
{
	SetMenuOpen(!bOpen);
}

void AVisionMenuActor::OpenMenu()
{
	SetMenuOpen(true);
}

void AVisionMenuActor::CloseMenu()
{
	SetMenuOpen(false);
}

void AVisionMenuActor::SetMenuOpen(bool bNewOpen)
{
	if (bOpen == bNewOpen) return;
	bOpen = bNewOpen;

	SetActorHiddenInGame(!bOpen);
	SetActorTickEnabled(bOpen);
	// Da chiuso non deve bloccare i trace (laser, sguardo degli NPC).
	SetActorEnableCollision(bOpen);
	MenuWidget->SetVisibility(bOpen);

	if (bOpen)
	{
		bRepositioning = false;
		SetActorLocation(ComputeTargetLocation());
		FVector ViewLoc;
		FRotator ViewRot;
		if (GetPlayerView(ViewLoc, ViewRot))
		{
			FRotator LookAt = (ViewLoc - GetActorLocation()).Rotation();
			LookAt.Pitch = 0.f;
			LookAt.Roll = 0.f;
			SetActorRotation(LookAt);
		}
		FindPointers();
		BindMenuInput();
	}
	else
	{
		ReleasePointers();
		LaserLeft->SetVisibility(false);
		LaserRight->SetVisibility(false);
	}
}

void AVisionMenuActor::FindPointers()
{
	PointerLeft = nullptr;
	PointerRight = nullptr;

	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn) return;

	TInlineComponentArray<UWidgetInteractionComponent*> Pointers(Pawn);
	for (UWidgetInteractionComponent* Pointer : Pointers)
	{
		FString HandName = Pointer->GetName();
		for (const USceneComponent* Parent = Pointer->GetAttachParent(); Parent; Parent = Parent->GetAttachParent())
		{
			if (const UMotionControllerComponent* Hand = Cast<UMotionControllerComponent>(Parent))
			{
				HandName = Hand->MotionSource.ToString();
				break;
			}
		}

		if (HandName.Contains(TEXT("Left")))
		{
			if (!PointerLeft) PointerLeft = Pointer;
		}
		else if (!PointerRight)
		{
			PointerRight = Pointer;
		}
	}

	if (!PointerLeft && !PointerRight)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VisionMenu] Il pawn %s non ha WidgetInteractionComponent: il menu non e' cliccabile."),
			*GetNameSafe(Pawn));
	}
}

void AVisionMenuActor::BindMenuInput()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	if (const ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (MenuMappingContext)
			{
				Sub->AddMappingContext(MenuMappingContext, 0);
			}
		}
	}

	if (bInputBound) return;

	EnableInput(PC);
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC || !InteractLeftAction || !InteractRightAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VisionMenu] Enhanced Input o IA_Menu_Interact_Left/Right mancanti: i grilletti non cliccheranno."));
		return;
	}

	EIC->BindAction(InteractLeftAction, ETriggerEvent::Started, this, &AVisionMenuActor::OnLeftPressed);
	EIC->BindAction(InteractLeftAction, ETriggerEvent::Completed, this, &AVisionMenuActor::OnLeftReleased);
	EIC->BindAction(InteractLeftAction, ETriggerEvent::Canceled, this, &AVisionMenuActor::OnLeftReleased);
	EIC->BindAction(InteractRightAction, ETriggerEvent::Started, this, &AVisionMenuActor::OnRightPressed);
	EIC->BindAction(InteractRightAction, ETriggerEvent::Completed, this, &AVisionMenuActor::OnRightReleased);
	EIC->BindAction(InteractRightAction, ETriggerEvent::Canceled, this, &AVisionMenuActor::OnRightReleased);
	bInputBound = true;
}

void AVisionMenuActor::ReleasePointers()
{
	for (UWidgetInteractionComponent* Pointer : { PointerLeft.Get(), PointerRight.Get() })
	{
		if (Pointer)
		{
			Pointer->ReleasePointerKey(EKeys::LeftMouseButton);
		}
	}
}

void AVisionMenuActor::OnLeftPressed()
{
	if (bOpen && PointerLeft) PointerLeft->PressPointerKey(EKeys::LeftMouseButton);
}

void AVisionMenuActor::OnLeftReleased()
{
	if (PointerLeft) PointerLeft->ReleasePointerKey(EKeys::LeftMouseButton);
}

void AVisionMenuActor::OnRightPressed()
{
	if (bOpen && PointerRight) PointerRight->PressPointerKey(EKeys::LeftMouseButton);
}

void AVisionMenuActor::OnRightReleased()
{
	if (PointerRight) PointerRight->ReleasePointerKey(EKeys::LeftMouseButton);
}

void AVisionMenuActor::UpdateLaser(const UWidgetInteractionComponent* Pointer, UStaticMeshComponent* Laser) const
{
	if (!Pointer)
	{
		Laser->SetVisibility(false);
		return;
	}

	const FVector Start = Pointer->GetComponentLocation();
	const FVector Direction = Pointer->GetForwardVector();
	float Length = FMath::Min(Pointer->InteractionDistance, 150.f);
	const FHitResult& Hit = Pointer->GetLastHitResult();
	if (Hit.bBlockingHit)
	{
		Length = FMath::Max(static_cast<float>(FVector::Dist(Start, Hit.ImpactPoint)), 1.f);
	}

	// Il cilindro engine e' alto 100 cm lungo Z.
	Laser->SetWorldLocationAndRotation(Start + Direction * (Length * 0.5f), FRotationMatrix::MakeFromZ(Direction).Rotator());
	Laser->SetWorldScale3D(FVector(LaserThickness / 100.f, LaserThickness / 100.f, Length / 100.f));
	Laser->SetVisibility(true);
}

void AVisionMenuActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateLaser(PointerLeft, LaserLeft);
	UpdateLaser(PointerRight, LaserRight);

	FVector ViewLoc;
	FRotator ViewRot;
	if (!GetPlayerView(ViewLoc, ViewRot))
	{
		return;
	}

	const FVector Target = ComputeTargetLocation();
	if (RecenterAngleDeg <= 0.f)
	{
		bRepositioning = true;
	}
	else if (!bRepositioning)
	{
		FVector ToMenu = GetActorLocation() - ViewLoc;
		ToMenu.Z = 0.f;
		FVector Forward = ViewRot.Vector();
		Forward.Z = 0.f;
		const float CosAngle = FVector::DotProduct(ToMenu.GetSafeNormal(), Forward.GetSafeNormal());
		const bool bOutOfView = CosAngle < FMath::Cos(FMath::DegreesToRadians(RecenterAngleDeg));
		const bool bTooFar = FVector::DistSquared(GetActorLocation(), Target) > FMath::Square(DistanceFromPlayer);
		bRepositioning = bOutOfView || bTooFar;
	}

	if (bRepositioning)
	{
		SetActorLocation(FMath::VInterpTo(GetActorLocation(), Target, DeltaTime, FollowSpeed));
		if (RecenterAngleDeg > 0.f && FVector::DistSquared(GetActorLocation(), Target) < FMath::Square(2.f))
		{
			bRepositioning = false;
		}
	}

	FRotator LookAt = (ViewLoc - GetActorLocation()).Rotation();
	LookAt.Pitch = 0.f;
	LookAt.Roll = 0.f;
	SetActorRotation(LookAt);
}

bool AVisionMenuActor::GetPlayerView(FVector& OutLocation, FRotator& OutRotation) const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return false;
	}
	PC->GetPlayerViewPoint(OutLocation, OutRotation);
	return true;
}

FVector AVisionMenuActor::ComputeTargetLocation() const
{
	FVector ViewLoc;
	FRotator ViewRot;
	if (!GetPlayerView(ViewLoc, ViewRot))
	{
		return GetActorLocation();
	}
	FVector Forward = ViewRot.Vector();
	Forward.Z = 0.f;
	Forward.Normalize();
	return ViewLoc + Forward * DistanceFromPlayer - FVector(0.f, 0.f, 10.f);
}