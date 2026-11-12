#include "GrabInfoTriggerComponent.h"
#include "GlassCardActor.h"
#include "HandGrabComponent.h"
#include "MotionControllerComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
UGrabInfoTriggerComponent::UGrabInfoTriggerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
}
void UGrabInfoTriggerComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	if (!Owner) return;

	SpawnCardIfNeeded();
	if (LinkedCard && bCardFollowsObject)
	{
		LinkedCard->SetFollowTarget(Owner);
	}

	const USceneComponent* OwnerRoot = Owner->GetRootComponent();
	const bool bMovable = OwnerRoot && OwnerRoot->Mobility == EComponentMobility::Movable;
	if (!bMovable)
	{
		bUseProximity = bProximityWhenNotGrabbable;
		return;
	}

	if (bAutoAddHandGrab
		&& OwnerRoot->IsA<UStaticMeshComponent>()
		&& !Owner->FindComponentByClass<UHandGrabComponent>())
	{
		UHandGrabComponent* Grab = NewObject<UHandGrabComponent>(Owner, TEXT("AutoHandGrab"));
		Owner->AddInstanceComponent(Grab);
		Grab->RegisterComponent();
	}
}
void UGrabInfoTriggerComponent::SpawnCardIfNeeded()
{
	if (LinkedCard || CardTitle.IsEmpty()) return;
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner) return;

	const FTransform SpawnTransform(Owner->GetActorLocation());
	AGlassCardActor* Card = World->SpawnActorDeferred<AGlassCardActor>(
		AGlassCardActor::StaticClass(), SpawnTransform, Owner, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Card) return;

	Card->Title = CardTitle;
	Card->Body = CardBody;
	Card->bStartClosed = true;
	UGameplayStatics::FinishSpawningActor(Card, SpawnTransform);
	LinkedCard = Card;
}
bool UGrabInfoTriggerComponent::IsPlayerNear() const
{
	const AActor* Owner = GetOwner();
	const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Owner || !Cam) return false;
	FVector Origin;
	FVector Extent;
	Owner->GetActorBounds(false, Origin, Extent);

	// Isteresi: una volta aperta, si chiude solo allontanandosi o voltandosi di piu'.
	const float Radius = bWasActive ? ProximityRadius * 1.25f : ProximityRadius;
	const float MinDot = bWasActive ? ProximityMinViewDot - 0.35f : ProximityMinViewDot;

	const FVector ToObject = Origin - Cam->GetCameraLocation();
	if (ToObject.SizeSquared() > FMath::Square(Radius)) return false;

	return FVector::DotProduct(Cam->GetCameraRotation().Vector(), ToObject.GetSafeNormal()) >= MinDot;
}
bool UGrabInfoTriggerComponent::IsHeldNow() const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->GetRootComponent()) return false;
	for (const USceneComponent* P = Owner->GetRootComponent()->GetAttachParent(); P; P = P->GetAttachParent())
	{
		if (P->IsA<UMotionControllerComponent>())
		{
			return true;
		}
	}
	return false;
}
void UGrabInfoTriggerComponent::TickComponent(float DeltaTime,
	ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!LinkedCard) return;

	const bool bActive = bUseProximity ? IsPlayerNear() : IsHeldNow();
	if (bActive)
	{
		InactiveTime = 0.f;
		if (!bWasActive)
		{
			LinkedCard->Open();
			bWasActive = true;
		}
		return;
	}

	if (bWasActive && bCloseOnRelease)
	{
		InactiveTime += DeltaTime;
		if (InactiveTime >= CloseDelay)
		{
			LinkedCard->Close();
			bWasActive = false;
		}
	}
	else if (bWasActive)
	{
		bWasActive = false;
	}
}