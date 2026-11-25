#include "VRHingeComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

static const FName HandleTag(TEXT("Handle"));
static const FName NoHighlightTag(TEXT("NoHighlight"));
UVRHingeComponent::UVRHingeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UVRHingeComponent::BeginPlay()
{
	Super::BeginPlay();
	BaseRelativeRotation = GetRelativeRotation().Quaternion();
	ApplyAngle(FMath::Clamp(bReturnToRest ? RestAngle : CurrentAngle, MinAngle, MaxAngle));
}

FTransform UVRHingeComponent::GetPivotFrame() const
{
	const FTransform Local(BaseRelativeRotation, GetRelativeLocation());
	const USceneComponent* Parent = GetAttachParent();
	return Parent ? Local * Parent->GetComponentTransform() : Local;
}

float UVRHingeComponent::HandAngle(const FVector& WorldPos) const
{
	const FVector L = GetPivotFrame().InverseTransformPosition(WorldPos);
	return FMath::RadiansToDegrees(FMath::Atan2(L.Y, L.X));
}

FVector UVRHingeComponent::GetHandleLocation() const
{
	TArray<USceneComponent*> Children;
	GetChildrenComponents(true, Children);
	for (const USceneComponent* C : Children)
	{
		if (C && C->ComponentHasTag(HandleTag))
		{
			if (const UPrimitiveComponent* P = Cast<UPrimitiveComponent>(C))
			{
				return P->Bounds.Origin;
			}
			return C->GetComponentLocation();
		}
	}

	// Fallback sull'Owner: necessario se la maniglia (es. RingMesh) simula fisica
	// e Chaos la stacca dalla gerarchia dei figli a runtime.
	if (const AActor* Owner = GetOwner())
	{
		TArray<USceneComponent*> OwnerComps;
		Owner->GetComponents<USceneComponent>(OwnerComps);
		for (const USceneComponent* C : OwnerComps)
		{
			if (C && C->ComponentHasTag(HandleTag))
			{
				if (const UPrimitiveComponent* P = Cast<UPrimitiveComponent>(C))
				{
					return P->Bounds.Origin;
				}
				return C->GetComponentLocation();
			}
		}
	}

	for (const USceneComponent* C : Children)
	{
		if (const UPrimitiveComponent* P = Cast<UPrimitiveComponent>(C))
		{
			return P->Bounds.Origin;
		}
	}
	return GetComponentLocation();
}

float UVRHingeComponent::GetNormalizedAngle() const
{
	return FMath::IsNearlyEqual(MaxAngle, MinAngle) ? 0.f : (CurrentAngle - MinAngle) / (MaxAngle - MinAngle);
}

void UVRHingeComponent::SetAngle(float NewAngle)
{
	AngularVelocity = 0.f;
	ApplyAngle(NewAngle);
}

void UVRHingeComponent::ApplyAngle(float NewAngle)
{
	const float Clamped = FMath::Clamp(NewAngle, MinAngle, MaxAngle);
	const bool bHitLimit = !FMath::IsNearlyEqual(Clamped, NewAngle, 0.01f);
	if (bHitLimit && !bAtLimit && LimitSound && FMath::Abs(AngularVelocity) > 20.f)
	{
		UGameplayStatics::PlaySoundAtLocation(this, LimitSound, GetComponentLocation(),
			FMath::Clamp(FMath::Abs(AngularVelocity) / 200.f, 0.2f, 1.f));
	}
	bAtLimit = bHitLimit;
	if (bHitLimit)
	{
		AngularVelocity = 0.f;
	}

	const float Delta = Clamped - CurrentAngle;
	if (FMath::IsNearlyZero(Delta, 0.001f) && HasBegunPlay()) return;

	CurrentAngle = Clamped;
	SetRelativeRotation(BaseRelativeRotation * FQuat(FVector::UpVector, FMath::DegreesToRadians(CurrentAngle)));

	MovedSinceSound += FMath::Abs(Delta);
	if (MoveSound && MovedSinceSound > 25.f)
	{
		MovedSinceSound = 0.f;
		UGameplayStatics::PlaySoundAtLocation(this, MoveSound, GetHandleLocation(), 0.5f, FMath::FRandRange(0.92f, 1.08f));
	}
	OnAngleChanged.Broadcast(GetNormalizedAngle());
}

void UVRHingeComponent::Grab(USceneComponent* Hand)
{
	if (!Hand || HeldBy) return;
	HeldBy = Hand;
	AngularVelocity = 0.f;
	GrabHandAngle = HandAngle(Hand->GetComponentLocation());
	GrabStartAngle = CurrentAngle;
}

void UVRHingeComponent::Release()
{
	HeldBy = nullptr;
	if (!bKeepMomentum)
	{
		AngularVelocity = 0.f;
	}
}

void UVRHingeComponent::SetHighlighted(bool bOn, UMaterialInterface* Material)
{
	TArray<UMeshComponent*> Meshes;

	TArray<USceneComponent*> Children;
	GetChildrenComponents(true, Children);
	for (USceneComponent* C : Children)
	{
		if (UMeshComponent* M = Cast<UMeshComponent>(C))
		{
			if (M->ComponentHasTag(HandleTag))
			{
				Meshes.AddUnique(M);
			}
		}
	}

	if (Meshes.Num() == 0)
	{
		if (AActor* Owner = GetOwner())
		{
			TArray<UMeshComponent*> OwnerMeshes;
			Owner->GetComponents<UMeshComponent>(OwnerMeshes);
			for (UMeshComponent* M : OwnerMeshes)
			{
				if (M && M->ComponentHasTag(HandleTag))
				{
					Meshes.AddUnique(M);
				}
			}
		}
	}

	if (Meshes.Num() == 0)
	{
		for (USceneComponent* C : Children)
		{
			if (UMeshComponent* M = Cast<UMeshComponent>(C))
			{
				if (!M->ComponentHasTag(NoHighlightTag))
				{
					Meshes.AddUnique(M);
				}
			}
		}
	}

	for (UMeshComponent* M : Meshes)
	{
		M->SetOverlayMaterial(bOn ? Material : nullptr);
	}
}

void UVRHingeComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (DeltaTime <= 0.f) return;

	if (HeldBy)
	{
		const FVector HandPos = HeldBy->GetComponentLocation();
		if (FVector::Dist(HandPos, GetHandleLocation()) > BreakDistance)
		{
			Release();
			return;
		}
		// Angolo con "unwrap" per evitare salti a +-180.
		const float Raw = FMath::FindDeltaAngleDegrees(GrabHandAngle, HandAngle(HandPos));
		const float Target = GrabStartAngle + Raw;
		const float Prev = CurrentAngle;
		ApplyAngle(FMath::FInterpTo(CurrentAngle, Target, DeltaTime, 25.f));
		AngularVelocity = (CurrentAngle - Prev) / DeltaTime;
		return;
	}

	if (bReturnToRest)
	{
		ApplyAngle(FMath::FInterpTo(CurrentAngle, RestAngle, DeltaTime, ReturnSpeed));
		return;
	}

	if (bKeepMomentum && FMath::Abs(AngularVelocity) > 0.5f)
	{
		AngularVelocity *= FMath::Exp(-Friction * DeltaTime);
		ApplyAngle(CurrentAngle + AngularVelocity * DeltaTime);
	}
}
