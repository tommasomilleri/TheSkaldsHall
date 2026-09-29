#include "GlassCardActor.h"
#include "SGlassCardWidget.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"

AGlassCardActor::AGlassCardActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Il WidgetComponent renderizza un widget Slate/UMG dentro il mondo 3D
	CardWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("CardWidget"));
	CardWidget->SetupAttachment(Root);
	CardWidget->SetWidgetSpace(EWidgetSpace::World);     // nel mondo, non a schermo
	CardWidget->SetDrawSize(FVector2D(500.f, 170.f));    // risoluzione in pixel
	CardWidget->SetTwoSided(true);                       // visibile da dietro
	CardWidget->SetBlendMode(EWidgetBlendMode::Transparent); // serve per la trasparenza!
	CardWidget->SetWorldScale3D(FVector(0.1f));          // 500px -> ~50cm reali
}

void AGlassCardActor::BeginPlay()
{
	Super::BeginPlay();

	// Colleghiamo il nostro widget Slate al componente
	CardWidget->SetSlateWidget(
		SNew(SGlassCardWidget).Title(Title).Body(Body));

	BaseZ = GetActorLocation().Z;
	CurrentScale = TargetScale = bStartClosed ? 0.f : 1.f;
	Root->SetRelativeScale3D(FVector(FMath::Max(CurrentScale, 0.001f)));
	if (bStartClosed) { SetActorHiddenInGame(true); }
}

void AGlassCardActor::Open()
{
	SetActorHiddenInGame(false);
	TargetScale = 1.f;  // il Tick anima la scala verso 1
}

void AGlassCardActor::Close()
{
	TargetScale = 0.f;  // il Tick anima verso 0 e poi nasconde
}

bool AGlassCardActor::IsHeld() const
{
	// "In mano" = attaccato (anche indirettamente) a un MotionController.
	// È così che il grab del VR Template attacca gli oggetti: quindi
	// funzioniamo col suo sistema SENZA modificare i suoi Blueprint.
	for (USceneComponent* P = Root->GetAttachParent(); P; P = P->GetAttachParent())
	{
		if (P->IsA<UMotionControllerComponent>()) return true;
	}
	return false;
}

void AGlassCardActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1) Animazione apertura/chiusura (interpolazione morbida)
	CurrentScale = FMath::FInterpTo(CurrentScale, TargetScale, DeltaTime, 10.f);
	Root->SetRelativeScale3D(FVector(FMath::Max(CurrentScale, 0.001f)));
	if (TargetScale <= 0.f && CurrentScale < 0.01f) { SetActorHiddenInGame(true); }

	// 2) Se il player la tiene in mano, niente fluttuazione/rotazione
	if (IsHeld()) return;

	// 3) Fluttuazione: seno sulla Z
	if (FloatAmplitude > 0.f)
	{
		FloatTime += DeltaTime * FloatSpeed;
		FVector Loc = GetActorLocation();
		Loc.Z = BaseZ + FMath::Sin(FloatTime) * FloatAmplitude;
		SetActorLocation(Loc);
	}

	// 4) Ruota dolcemente verso la camera del player (solo yaw)
	if (bFacePlayer)
	{
		if (const APlayerCameraManager* Cam =
			UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			FRotator Look = (Cam->GetCameraLocation() - GetActorLocation()).Rotation();
			Look.Pitch = 0.f; Look.Roll = 0.f;
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), Look, DeltaTime, 4.f));
		}
	}
}