
#include "VisionMenuActor.h"
#include "VisionMenuWidget.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

AVisionMenuActor::AVisionMenuActor()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	MenuWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("MenuWidget"));
	MenuWidget->SetupAttachment(Root);
	MenuWidget->SetWidgetClass(UVisionMenuWidget::StaticClass());
	MenuWidget->SetDrawSize(FVector2D(760.f, 620.f));
	MenuWidget->SetTwoSided(true);
	MenuWidget->SetWorldScale3D(FVector(0.07f));

	MenuWidget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MenuWidget->SetCollisionProfileName(TEXT("UI"));
	MenuWidget->SetWindowFocusable(true);
	MenuWidget->SetBlendMode(EWidgetBlendMode::Transparent);
	MenuWidget->SetBackgroundColor(FLinearColor::Transparent);
	MenuWidget->SetBlendMode(EWidgetBlendMode::Transparent);
	// Scala mondo: 520px -> ~52 cm di larghezza
}

void AVisionMenuActor::BeginPlay()
{
	Super::BeginPlay();
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	MenuWidget->SetVisibility(false);
}

void AVisionMenuActor::ToggleMenu()
{
	bOpen = !bOpen;
	SetActorHiddenInGame(!bOpen);
	SetActorTickEnabled(bOpen);
	MenuWidget->SetVisibility(bOpen);

	if (bOpen)
	{
		// Riposiziona subito davanti al giocatore
		SetActorLocation(ComputeTargetLocation());
	}
}

void AVisionMenuActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector ViewLoc;
	FRotator ViewRot;
	if (!GetPlayerView(ViewLoc, ViewRot))
	{
		return;
	}

	// Follow morbido
	const FVector Target = ComputeTargetLocation();
	SetActorLocation(FMath::VInterpTo(GetActorLocation(), Target, DeltaTime, FollowSpeed));

	// Sempre rivolto verso il giocatore
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
	if (!const_cast<AVisionMenuActor*>(this)->GetPlayerView(ViewLoc, ViewRot))
	{
		return GetActorLocation();
	}
	FVector Forward = ViewRot.Vector();
	Forward.Z = 0.f;
	Forward.Normalize();
	return ViewLoc + Forward * DistanceFromPlayer - FVector(0.f, 0.f, 10.f);
}
