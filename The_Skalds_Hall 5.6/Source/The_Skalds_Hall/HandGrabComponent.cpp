#include "HandGrabComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/BodySetup.h"

UHandGrabComponent::UHandGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UHandGrabComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* Owner = GetOwner())
	{
		Mesh = Owner->FindComponentByClass<UStaticMeshComponent>();
	}
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[HandGrab] %s: nessuna StaticMeshComponent trovata."),
			*GetNameSafe(GetOwner()));
		return;
	}

	Mesh->SetSimulatePhysics(true);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));

	if (Mesh->GetBodySetup() &&
		Mesh->GetBodySetup()->CollisionTraceFlag == CTF_UseComplexAsSimple)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[HandGrab] %s usa Complex-As-Simple: imposta 'Project Default' e aggiungi collisione simple."),
			*GetNameSafe(GetOwner()));
	}
}

void UHandGrabComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HeldBy && Mesh)
	{
		Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
	HeldBy = nullptr;
	bSnapping = false;
	Super::EndPlay(EndPlayReason);
}

void UHandGrabComponent::Grab(USceneComponent* Hand)
{
	if (!Mesh || !Hand || HeldBy) return;

	HeldBy = Hand;
	Mesh->SetSimulatePhysics(false);
	Mesh->AttachToComponent(Hand, FAttachmentTransformRules::KeepWorldTransform);

	bSnapping = true;
	SetComponentTickEnabled(true);
}

void UHandGrabComponent::Release(FVector LinearVelocity, FVector AngularVelocity)
{
	if (!Mesh || !HeldBy) return;

	Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetPhysicsLinearVelocity(LinearVelocity);
	Mesh->SetPhysicsAngularVelocityInDegrees(AngularVelocity);

	HeldBy = nullptr;
	bSnapping = false;
	SetComponentTickEnabled(false);
}

void UHandGrabComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bSnapping || !Mesh || !HeldBy)
	{
		bSnapping = false;
		SetComponentTickEnabled(false);
		return;
	}

	const FVector TargetPoint =
		HeldBy->GetComponentTransform().TransformPosition(PalmOffset);
	const FVector Delta = TargetPoint - Mesh->Bounds.Origin;
	const FVector Step = Delta * FMath::Clamp(DeltaTime * SnapSpeed, 0.f, 1.f);
	Mesh->AddWorldOffset(Step);

	if (Delta.SizeSquared() < 0.25f)
	{
		bSnapping = false;
		SetComponentTickEnabled(false);
	}
}

bool UHandGrabComponent::TraceFingerToObject(USceneComponent* FingerSegment,
	UPrimitiveComponent* Target, float MaxDist, FHitResult& OutHit)
{
	if (!FingerSegment || !Target || MaxDist <= 0.f) return false;

	const FVector Start = FingerSegment->GetComponentLocation();
	const FVector Dir = (Target->Bounds.Origin - Start).GetSafeNormal();
	if (Dir.IsNearlyZero()) return false;

	return Target->LineTraceComponent(OutHit, Start, Start + Dir * MaxDist,
		FCollisionQueryParams(SCENE_QUERY_STAT(FingerTrace), true));
}