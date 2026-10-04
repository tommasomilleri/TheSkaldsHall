#include "HandGrabComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
UHandGrabComponent::UHandGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Tick attivo SOLO durante lo snap verso il palmo: costo zero a riposo
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

	// REGOLA: la mesh deve avere collisione SIMPLE (box/convex).
	// MAI "Use Complex Collision As Simple": vieta la simulazione fisica
	// e rende l'oggetto un ologramma compenetrabile.
	Mesh->SetSimulatePhysics(true);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));

	if (Mesh->GetBodySetup() &&
		Mesh->GetBodySetup()->CollisionTraceFlag == CTF_UseComplexAsSimple)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[HandGrab] %s usa Complex-As-Simple: la fisica NON funzionera'. "
				"Imposta 'Project Default' e aggiungi collisione simple."),
			*GetNameSafe(GetOwner()));
	}
}

void UHandGrabComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Sicurezza: se l'actor muore mentre è in mano, stacchiamo pulito
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
	if (!Mesh || !Hand || HeldBy) return; // già in mano: ignora

	HeldBy = Hand;
	Mesh->SetSimulatePhysics(false);
	// KeepWorld + snap interpolato nel Tick = niente teletrasporto brusco
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

	// Punto di presa desiderato: palmo + offset (in spazio mano)
	const FVector TargetPoint =
		HeldBy->GetComponentTransform().TransformPosition(PalmOffset);

	// FIX PIVOT: il delta è calcolato dal CENTRO dei bounds della mesh,
	// non dal pivot dell'actor (spesso decentrato negli asset importati).
	const FVector Delta = TargetPoint - Mesh->Bounds.Origin;

	// Interp esponenziale del delta: morbido e indipendente dal framerate
	const FVector Step = Delta * FMath::Clamp(DeltaTime * SnapSpeed, 0.f, 1.f);
	GetOwner()->AddActorWorldOffset(Step);

	if (Delta.SizeSquared() < 0.25f) // < 0.5 cm: snap completato
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
	if (Dir.IsNearlyZero()) return false; // dito esattamente sul centro

	// Trace diretto sul SINGOLO componente: ignora il resto del mondo,
	// quindi nessun falso negativo da oggetti in mezzo.
	return Target->LineTraceComponent(OutHit, Start, Start + Dir * MaxDist,
		FCollisionQueryParams(SCENE_QUERY_STAT(FingerTrace), true));
}
