#include "GlassCardActor.h"
#include "SGlassCardWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace GlassCard
{
	// Smorzamento critico: insegue senza oscillare e senza scatti, indipendente dagli fps.
	FVector SmoothDamp(const FVector& Current, const FVector& Target, FVector& Velocity, float SmoothTime, float DeltaTime)
	{
		const float Omega = 2.f / FMath::Max(SmoothTime, 0.0001f);
		const float X = Omega * DeltaTime;
		const float Exp = 1.f / (1.f + X + 0.48f * X * X + 0.235f * X * X * X);
		const FVector Change = Current - Target;
		const FVector Temp = (Velocity + Omega * Change) * DeltaTime;
		Velocity = (Velocity - Omega * Temp) * Exp;
		return Target + (Change + Temp) * Exp;
	}
}

AGlassCardActor::AGlassCardActor()
{
	PrimaryActorTick.bCanEverTick = true;
	// Dopo il movimento di oggetti e mani: niente ritardo di un frame.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	CardWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("CardWidget"));
	CardWidget->SetupAttachment(Root);
	CardWidget->SetWidgetSpace(EWidgetSpace::World);
	CardWidget->SetDrawSize(FVector2D(900.f, 300.f));
	CardWidget->SetDrawAtDesiredSize(true);
	// Bordo inferiore sull'origine: la scheda cresce verso l'alto, mai dentro l'oggetto.
	CardWidget->SetPivot(FVector2D(0.5f, 1.f));
	CardWidget->SetTwoSided(true);
	CardWidget->SetBlendMode(EWidgetBlendMode::Transparent);
	CardWidget->SetBackgroundColor(FLinearColor::Transparent);
	CardWidget->SetRelativeScale3D(FVector(0.04f));
	CardWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CardWidget->SetCastShadow(false);
	CardWidget->SetTranslucentSortPriority(10);

	GlassPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GlassPanel"));
	GlassPanel->SetupAttachment(Root);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		GlassPanel->SetStaticMesh(PlaneMesh.Object);
	}
	GlassPanel->SetRelativeRotation(FRotator(0.f, 90.f, -90.f));
	GlassPanel->SetRelativeLocation(FVector(-0.5f, 0.f, 0.f));
	GlassPanel->SetRelativeScale3D(FVector(0.36f, 0.12f, 1.f));
	GlassPanel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GlassPanel->SetCastShadow(false);
}

void AGlassCardActor::BeginPlay()
{
	Super::BeginPlay();

	BuildWidget();

	if (GlassMaterial)
	{
		GlassPanel->SetMaterial(0, GlassMaterial);
	}
	else
	{
		GlassPanel->SetVisibility(false);
	}

	if (FollowTarget)
	{
		SetFollowTarget(FollowTarget);
	}

	BaseZ = GetActorLocation().Z;
	CurrentScale = TargetScale = bStartClosed ? 0.f : 1.f;
	Root->SetRelativeScale3D(FVector(FMath::Max(CurrentScale, 0.001f)));
	if (bStartClosed)
	{
		SetActorHiddenInGame(true);
	}
	SetActorTickEnabled(!bStartClosed);
}

void AGlassCardActor::BuildWidget()
{
	const float PixelWidth = FMath::Max(CardPixelWidth, 300.f);
	TSharedRef<SWidget> Card = SNew(SGlassCardWidget)
		.Title(Title)
		.Body(Body)
		.Width(PixelWidth)
		.TitleSize(TitleFontSize)
		.BodySize(BodyFontSize)
		.BackgroundOpacity(BackgroundOpacity);
	Card->SetVisibility(EVisibility::HitTestInvisible);
	CardWidget->SetSlateWidget(Card);
	CardWidget->SetRelativeScale3D(FVector(CardWorldWidth / PixelWidth));
	CardWidget->SetRedrawTime(0.5f);
	CardWidget->RequestRenderUpdate();
}

void AGlassCardActor::SetTexts(const FText& NewTitle, const FText& NewBody)
{
	Title = NewTitle;
	Body = NewBody;
	if (HasActorBegunPlay())
	{
		BuildWidget();
	}
}

void AGlassCardActor::SetFollowTarget(AActor* NewTarget)
{
	if (FollowTarget && FollowTarget != NewTarget)
	{
		RemoveTickPrerequisiteActor(FollowTarget);
	}
	FollowTarget = NewTarget;
	if (FollowTarget)
	{
		AddTickPrerequisiteActor(FollowTarget);
		SnapToTarget();
	}
}

void AGlassCardActor::SnapToTarget()
{
	FVector Anchor;
	if (ComputeFollowAnchor(Anchor, 0.f))
	{
		FollowPosition = Anchor;
		SetActorLocation(Anchor);
		FollowVelocity = FVector::ZeroVector;
		bHasFollowPosition = true;
	}
}

void AGlassCardActor::Open()
{
	const bool bWasHidden = IsHidden();
	SetActorTickEnabled(true);
	SetActorHiddenInGame(false);
	TargetScale = 1.f;
	CardWidget->RequestRenderUpdate();

	if (bWasHidden && FollowTarget)
	{
		// Parte dal centro dell'oggetto ed "esce" verso la sua posizione.
		FVector Origin;
		FVector Extent;
		FollowTarget->GetActorBounds(false, Origin, Extent);
		FollowPosition = Origin;
		SetActorLocation(Origin);
		FollowVelocity = FVector::ZeroVector;
		HeldBlend = IsTargetHeld() ? 1.f : 0.f;
		bHasFollowPosition = true;
	}
}

void AGlassCardActor::Close()
{
	SetActorTickEnabled(true);
	TargetScale = 0.f;
}

bool AGlassCardActor::IsTargetHeld() const
{
	if (!FollowTarget || !FollowTarget->GetRootComponent()) return false;
	for (const USceneComponent* P = FollowTarget->GetRootComponent()->GetAttachParent(); P; P = P->GetAttachParent())
	{
		if (P->IsA<UMotionControllerComponent>())
		{
			return true;
		}
	}
	return false;
}

bool AGlassCardActor::ComputeFollowAnchor(FVector& OutAnchor, float DeltaTime)
{
	if (!IsValid(FollowTarget)) return false;

	FVector Origin;
	FVector Extent;
	FollowTarget->GetActorBounds(false, Origin, Extent);

	FVector CamLoc = Origin - FollowTarget->GetActorForwardVector() * 100.f;
	FRotator CamRot = FRotator::ZeroRotator;
	if (const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		CamLoc = Cam->GetCameraLocation();
		CamRot = Cam->GetCameraRotation();
	}

	FVector ToPlayer = CamLoc - Origin;
	ToPlayer.Z = 0.f;
	ToPlayer = ToPlayer.GetSafeNormal();

	// A riposo: sopra l'oggetto, un po' verso il giocatore.
	const FVector RestAnchor = Origin
		+ FVector(0.f, 0.f, Extent.Z + HeightAboveObject)
		+ ToPlayer * TowardPlayerOffset;

	// In mano: di lato, verso il centro della vista.
	const FVector CamRight = FRotationMatrix(FRotator(0.f, CamRot.Yaw, 0.f)).GetUnitAxis(EAxis::Y);
	const float SideSign = FVector::DotProduct(Origin - CamLoc, CamRight) >= 0.f ? -1.f : 1.f;
	HeldSide = DeltaTime > 0.f ? FMath::FInterpTo(HeldSide, SideSign, DeltaTime, 4.f) : SideSign;
	const float HalfWidth = CardWorldWidth * 0.5f;
	const FVector HeldAnchor = Origin
		+ CamRight * HeldSide * (SideOffsetWhenHeld + HalfWidth)
		+ FVector(0.f, 0.f, HeightWhenHeld)
		+ ToPlayer * 4.f;

	const float HeldTarget = IsTargetHeld() ? 1.f : 0.f;
	HeldBlend = DeltaTime > 0.f ? FMath::FInterpTo(HeldBlend, HeldTarget, DeltaTime, 6.f) : HeldTarget;
	const float Alpha = FMath::SmoothStep(0.f, 1.f, HeldBlend);
	OutAnchor = FMath::Lerp(RestAnchor, HeldAnchor, Alpha);
	return true;
}

void AGlassCardActor::UpdatePanelSize()
{
	if (!GlassPanel->IsVisible()) return;

	// Il Plane engine e' 100x100 cm: lo si adatta alla dimensione reale del widget.
	const FVector2D DrawSize = CardWidget->GetCurrentDrawSize();
	const float Scale = CardWidget->GetRelativeScale3D().X;
	const float WidthCm = static_cast<float>(DrawSize.X) * Scale;
	const float HeightCm = static_cast<float>(DrawSize.Y) * Scale;
	if (WidthCm <= 0.f || HeightCm <= 0.f) return;

	GlassPanel->SetRelativeScale3D(FVector(WidthCm / 100.f * 1.02f, HeightCm / 100.f * 1.04f, 1.f));
	GlassPanel->SetRelativeLocation(FVector(-0.5f, 0.f, HeightCm * 0.5f));
}

void AGlassCardActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Apertura morbida (ease-out), chiusura piu' rapida.
	const float ScaleSpeed = TargetScale > CurrentScale ? 9.f : 14.f;
	CurrentScale = FMath::FInterpTo(CurrentScale, TargetScale, DeltaTime, ScaleSpeed);
	const float Shown = TargetScale > 0.f ? FMath::InterpEaseOut(0.f, 1.f, CurrentScale, 2.5f) : CurrentScale;
	Root->SetRelativeScale3D(FVector(FMath::Max(Shown, 0.001f)));
	if (TargetScale <= 0.f && CurrentScale < 0.01f)
	{
		SetActorHiddenInGame(true);
		SetActorTickEnabled(false);
		return;
	}

	UpdatePanelSize();

	FloatTime += DeltaTime * FloatSpeed;
	const float Bob = FloatAmplitude * (1.f - HeldBlend) * FMath::Sin(FloatTime);

	FVector Shown3D = GetActorLocation();
	FVector Anchor;
	if (ComputeFollowAnchor(Anchor, DeltaTime))
	{
		if (!bHasFollowPosition || FVector::DistSquared(FollowPosition, Anchor) > FMath::Square(SnapDistance))
		{
			FollowPosition = Anchor;
			FollowVelocity = FVector::ZeroVector;
			bHasFollowPosition = true;
		}
		else
		{
			FollowPosition = GlassCard::SmoothDamp(FollowPosition, Anchor, FollowVelocity, FollowSmoothTime, DeltaTime);
		}
		// L'ondeggiamento si somma solo alla posizione mostrata, non allo stato del follow.
		Shown3D = FollowPosition + FVector(0.f, 0.f, Bob);
	}
	else if (FloatAmplitude > 0.f)
	{
		Shown3D.Z = BaseZ + Bob;
	}
	SetActorLocation(Shown3D);

	if (bFacePlayer)
	{
		if (const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			FRotator Look = (Cam->GetCameraLocation() - Shown3D).Rotation();
			Look.Pitch = FMath::Clamp(Look.Pitch, -MaxPitch, MaxPitch);
			Look.Roll = 0.f;
			const FQuat Smoothed = FQuat::Slerp(GetActorQuat(), Look.Quaternion(),
				FMath::Clamp(DeltaTime * 8.f, 0.f, 1.f));
			SetActorRotation(Smoothed);
		}
	}
}