#include "HandGrabComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

static const FName GripTag(TEXT("Grip"));

UHandGrabComponent::UHandGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	GrabMode = EHandGrabMode::KeepOffset;
}

void UHandGrabComponent::BeginPlay()
{
	Super::BeginPlay();

	Mesh = FindMainMesh(GetOwner());
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandGrab] %s: nessuna StaticMeshComponent trovata."), *GetNameSafe(GetOwner()));
		return;
	}

	if (Mesh->GetBodySetup() && Mesh->GetBodySetup()->CollisionTraceFlag == CTF_UseComplexAsSimple)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[HandGrab] %s usa Complex-As-Simple: la fisica non funziona bene. Imposta 'Project Default' e aggiungi collisione simple."),
			*GetNameSafe(GetOwner()));
	}

	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetLinearDamping(LinearDamping);
	Mesh->SetAngularDamping(AngularDamping);
	Mesh->SetUseCCD(bUseCCD);
	Mesh->BodyInstance.SetMaxDepenetrationVelocity(MaxDepenetrationVelocity);
	Mesh->SetSimulatePhysics(true);

	if (Mesh->GetMass() < MinMassKg)
	{
		Mesh->SetMassOverrideInKg(NAME_None, MinMassKg, true);
	}

	// Parte "addormentato": niente scatti al Play se tocca il tavolo o altri oggetti.
	if (bStartAsleep)
	{
		Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		Mesh->PutRigidBodyToSleep();
	}

	Mesh->SetNotifyRigidBodyCollision(true);
	Mesh->OnComponentHit.AddDynamic(this, &UHandGrabComponent::HandleHit);

	HomeTransform = Mesh->GetComponentTransform();
}

UStaticMeshComponent* UHandGrabComponent::FindMainMesh(const AActor* Actor)
{
	if (!Actor) return nullptr;
	if (UStaticMeshComponent* Root = Cast<UStaticMeshComponent>(Actor->GetRootComponent()))
	{
		return Root;
	}
	TArray<UStaticMeshComponent*> Meshes;
	Actor->GetComponents<UStaticMeshComponent>(Meshes);
	for (UStaticMeshComponent* M : Meshes)
	{
		if (M && !M->ComponentHasTag(TEXT("Liquid")))
		{
			return M;
		}
	}
	return nullptr;
}

void UHandGrabComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReturnTimer);
	}
	if (HeldBy && Mesh)
	{
		Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
	HeldBy = nullptr;
	bSnapping = false;
	bReturning = false;
	Super::EndPlay(EndPlayReason);
}

void UHandGrabComponent::SetHighlighted(bool bOn, UMaterialInterface* Material)
{
	if (!Mesh) return;
	HighlightCount = FMath::Max(0, HighlightCount + (bOn ? 1 : -1));
	Mesh->SetOverlayMaterial((HighlightCount > 0 && !HeldBy) ? Material : nullptr);
}

float UHandGrabComponent::GetDistanceToSurface(const FVector& Point) const
{
	if (!Mesh) return -1.f;
	FVector Closest;
	return Mesh->GetClosestPointOnCollision(Point, Closest);
}

USceneComponent* UHandGrabComponent::FindClosestGrip(const FVector& Palm) const
{
	const AActor* Owner = GetOwner();
	if (!Owner) return nullptr;

	USceneComponent* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	TArray<USceneComponent*> Comps;
	Owner->GetComponents<USceneComponent>(Comps);
	for (USceneComponent* C : Comps)
	{
		if (C && C->ComponentHasTag(GripTag))
		{
			const float D = FVector::DistSquared(C->GetComponentLocation(), Palm);
			if (D < BestDistSq)
			{
				BestDistSq = D;
				Best = C;
			}
		}
	}
	return Best;
}

FTransform UHandGrabComponent::ComputeTargetWorld(const FVector& Palm, const FQuat& HandRot, USceneComponent*& OutGrip) const
{
	FTransform MeshTf = Mesh->GetComponentTransform();
	OutGrip = (GrabMode != EHandGrabMode::SnapCenter) ? FindClosestGrip(Palm) : nullptr;

	if (OutGrip)
	{
		const FTransform GripInMesh = OutGrip->GetComponentTransform().GetRelativeTransform(MeshTf);
		if (bAlignGripRotation)
		{
			MeshTf.SetRotation(HandRot * GripInMesh.GetRotation().Inverse());
		}
		const FVector GripWorld = MeshTf.TransformPosition(GripInMesh.GetLocation());
		MeshTf.AddToTranslation(Palm - GripWorld);
		return MeshTf;
	}

	const FVector Center = Mesh->Bounds.Origin;
	if (GrabMode == EHandGrabMode::SnapCenter)
	{
		MeshTf.AddToTranslation(Palm - Center);
		return MeshTf;
	}

	// KeepOffset: l'oggetto si sposta solo se il palmo e' dentro o troppo vicino.
	const float Dist = GetDistanceToSurface(Palm);
	if (Dist < 0.f || Dist >= SurfaceGap)
	{
		return MeshTf;
	}

	FVector Dir = Center - Palm;
	if (!Dir.Normalize())
	{
		Dir = HandRot.GetForwardVector();
	}
	const FVector E = Mesh->Bounds.BoxExtent;
	const float ExtentAlongDir = FMath::Abs(Dir.X) * E.X + FMath::Abs(Dir.Y) * E.Y + FMath::Abs(Dir.Z) * E.Z;
	MeshTf.AddToTranslation(Palm + Dir * (ExtentAlongDir + SurfaceGap) - Center);
	return MeshTf;
}

void UHandGrabComponent::Grab(USceneComponent* Hand)
{
	if (!Mesh || !Hand || HeldBy) return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReturnTimer);
	}
	bReturning = false;

	HeldBy = Hand;
	Mesh->SetOverlayMaterial(nullptr);
	Mesh->SetSimulatePhysics(false);

	if (bIgnorePawnWhileHeld)
	{
		SavedPawnResponse = Mesh->GetCollisionResponseToChannel(ECC_Pawn);
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	const FTransform HandTf = Hand->GetComponentTransform();
	const FVector Palm = HandTf.TransformPosition(PalmOffset);
	USceneComponent* Grip = nullptr;
	const FTransform TargetWorld = ComputeTargetWorld(Palm, HandTf.GetRotation(), Grip);

	ActiveGripCurl = DefaultGripCurl;
	if (Grip)
	{
		for (const FName& Tag : Grip->ComponentTags)
		{
			const FString S = Tag.ToString();
			if (S.StartsWith(TEXT("Curl=")))
			{
				ActiveGripCurl = FMath::Clamp(FCString::Atof(*S.RightChop(5)), 0.f, 1.f);
			}
		}
	}

	Mesh->AttachToComponent(Hand, FAttachmentTransformRules::KeepWorldTransform);
	TargetRelative = TargetWorld.GetRelativeTransform(HandTf);
	TargetRelative.SetScale3D(Mesh->GetRelativeScale3D());

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, Mesh->GetComponentLocation(), 0.6f);
	}

	bSnapping = true;
	SetComponentTickEnabled(true);
}

void UHandGrabComponent::Release(FVector LinearVelocity, FVector AngularVelocity)
{
	if (!Mesh || !HeldBy) return;

	Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	if (bIgnorePawnWhileHeld)
	{
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, SavedPawnResponse);
	}
	Mesh->SetSimulatePhysics(true);
	Mesh->SetPhysicsLinearVelocity(LinearVelocity);
	Mesh->SetPhysicsAngularVelocityInDegrees(AngularVelocity);

	HeldBy = nullptr;
	bSnapping = false;
	SetComponentTickEnabled(false);

	if (bReturnHome)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(ReturnTimer, this, &UHandGrabComponent::StartReturn, FMath::Max(ReturnDelay, 0.01f), false);
		}
	}
}

void UHandGrabComponent::StartReturn()
{
	if (!Mesh || HeldBy) return;
	Mesh->SetSimulatePhysics(false);
	ReturnStart = Mesh->GetComponentTransform();
	ReturnAlpha = 0.f;
	bReturning = true;
	SetComponentTickEnabled(true);
}

void UHandGrabComponent::HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!ImpactSound || !Mesh || HeldBy) return;

	const UWorld* World = GetWorld();
	if (!World) return;
	const float Now = World->GetTimeSeconds();
	if (Now - LastImpactTime < ImpactCooldown) return;

	// Variazione di velocita' dovuta all'urto = impulso / massa.
	const float Mass = FMath::Max(Mesh->GetMass(), 0.01f);
	const float Speed = NormalImpulse.Size() / Mass;
	if (Speed < MinImpactSpeed) return;

	LastImpactTime = Now;
	const float Volume = static_cast<float>(FMath::GetMappedRangeValueClamped(
		FVector2D(MinImpactSpeed, FullVolumeSpeed), FVector2D(0.15, 1.0), static_cast<double>(Speed)));
	UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint, Volume, FMath::FRandRange(0.9f, 1.1f));
}

void UHandGrabComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Mesh)
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (bReturning)
	{
		ReturnAlpha = FMath::Min(ReturnAlpha + DeltaTime / ReturnTime, 1.f);
		const float A = FMath::SmoothStep(0.f, 1.f, ReturnAlpha);
		Mesh->SetWorldLocationAndRotation(
			FMath::Lerp(ReturnStart.GetLocation(), HomeTransform.GetLocation(), A),
			FQuat::Slerp(ReturnStart.GetRotation(), HomeTransform.GetRotation(), A));
		if (ReturnAlpha >= 1.f)
		{
			bReturning = false;
			Mesh->SetSimulatePhysics(true);
			Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			Mesh->PutRigidBodyToSleep();
			SetComponentTickEnabled(false);
		}
		return;
	}

	if (!bSnapping || !HeldBy)
	{
		bSnapping = false;
		SetComponentTickEnabled(false);
		return;
	}

	const float Alpha = FMath::Clamp(DeltaTime * SnapSpeed, 0.f, 1.f);
	const FVector Loc = FMath::Lerp(Mesh->GetRelativeLocation(), TargetRelative.GetLocation(), Alpha);
	const FQuat Rot = FQuat::Slerp(Mesh->GetRelativeRotation().Quaternion(), TargetRelative.GetRotation(), Alpha);
	Mesh->SetRelativeLocationAndRotation(Loc, Rot);

	if (FVector::DistSquared(Loc, TargetRelative.GetLocation()) < 0.04f
		&& Rot.AngularDistance(TargetRelative.GetRotation()) < 0.01f)
	{
		Mesh->SetRelativeLocationAndRotation(TargetRelative.GetLocation(), TargetRelative.GetRotation());
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
