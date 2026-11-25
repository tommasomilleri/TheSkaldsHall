#include "VRHandComponent.h"
#include "HandGrabComponent.h"
#include "VRHingeComponent.h"
#include "GrabInfoTriggerComponent.h"
#include "MotionControllerComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UVRHandComponent::UVRHandComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Dopo l'aggiornamento dei controller, prima del rendering.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UVRHandComponent::BeginPlay()
{
	Super::BeginPlay();

	Controller = GetAttachParent();
	if (!Controller)
	{
		UE_LOG(LogTemp, Error, TEXT("[VRHand] %s deve essere figlio di un MotionController."), *GetName());
		return;
	}

	TArray<USceneComponent*> Children;
	Controller->GetChildrenComponents(true, Children);
	for (USceneComponent* C : Children)
	{
		USkeletalMeshComponent* Sk = Cast<USkeletalMeshComponent>(C);
		if (Sk && (HandMeshName.IsNone() || Sk->GetFName() == HandMeshName))
		{
			HandMesh = Sk;
			break;
		}
	}
	if (HandMesh)
	{
		HandMeshRelative = HandMesh->GetComponentTransform().GetRelativeTransform(Controller->GetComponentTransform());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[VRHand] Nessuna Skeletal Mesh della mano trovata sotto %s."), *Controller->GetName());
	}
}

FVector UVRHandComponent::GetPalm() const
{
	return Controller ? Controller->GetComponentTransform().TransformPosition(PalmOffset) : GetComponentLocation();
}

bool UVRHandComponent::IsHolding() const
{
	return HeldObject != nullptr || HeldHinge != nullptr;
}

void UVRHandComponent::FindCandidates(UHandGrabComponent*& OutObject, UVRHingeComponent*& OutHinge, float Radius) const
{
	OutObject = nullptr;
	OutHinge = nullptr;
	UWorld* World = GetWorld();
	if (!World) return;

	const FVector Palm = GetPalm();
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VRHandGrab), false, GetOwner());
	World->OverlapMultiByObjectType(Overlaps, Palm, FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
		FCollisionShape::MakeSphere(Radius + 25.f), Params);

	float BestObj = Radius;
	float BestHinge = TNumericLimits<float>::Max();
	TSet<AActor*> Visited;
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* A = O.GetActor();
		if (!A || Visited.Contains(A)) continue;
		Visited.Add(A);

		if (UHandGrabComponent* G = A->FindComponentByClass<UHandGrabComponent>())
		{
			if (!G->IsGrabbed())
			{
				const float D = G->GetDistanceToSurface(Palm);
				if (D >= 0.f && D <= BestObj)
				{
					BestObj = D;
					OutObject = G;
				}
			}
		}

		TArray<UVRHingeComponent*> Hinges;
		A->GetComponents<UVRHingeComponent>(Hinges);
		for (UVRHingeComponent* H : Hinges)
		{
			if (!H || H->IsGrabbed()) continue;
			const float D = FVector::Dist(H->GetHandleLocation(), Palm);
			const float MaxDist = (Radius > GrabRadius) ? FMath::Max(Radius, H->GrabRadius) : H->GrabRadius;
			if (D <= MaxDist && D < BestHinge)
			{
				BestHinge = D;
				OutHinge = H;
			}
		}
	}

	// Se ci sono entrambi, vince la maniglia solo se piu' vicina dell'oggetto.
	if (OutHinge && OutObject && BestHinge > BestObj)
	{
		OutHinge = nullptr;
	}
	else if (OutHinge)
	{
		OutObject = nullptr;
	}
}

bool UVRHandComponent::TryGrab()
{
	if (!Controller || IsHolding()) return false;

	UHandGrabComponent* Obj = nullptr;
	UVRHingeComponent* Hinge = nullptr;
	FindCandidates(Obj, Hinge, GrabRadius);

	if (HoverObject.IsValid())
	{
		HoverObject->SetHighlighted(false, HighlightMaterial);
		HoverObject = nullptr;
	}
	if (HoverHinge.IsValid())
	{
		HoverHinge->SetHighlighted(false, HighlightMaterial);
		HoverHinge = nullptr;
	}

	if (Hinge)
	{
		HeldHinge = Hinge;
		Hinge->Grab(Controller);
	}
	else if (Obj)
	{
		HeldObject = Obj;
		Obj->Grab(Controller);
	}
	else
	{
		return false;
	}
	Pulse(0.6f, 0.06f);
	return true;
}

void UVRHandComponent::Release()
{
	if (HeldHinge)
	{
		HeldHinge->Release();
		HeldHinge = nullptr;
	}
	if (HeldObject)
	{
		FVector Lin, Ang;
		ComputeVelocity(Lin, Ang);
		HeldObject->Release(Lin * ThrowVelocityScale, Ang);
		HeldObject = nullptr;
	}
}

void UVRHandComponent::RecordHistory()
{
	if (!Controller || !GetWorld()) return;
	PosHistory[HistoryIndex] = Controller->GetComponentLocation();
	RotHistory[HistoryIndex] = Controller->GetComponentQuat();
	TimeHistory[HistoryIndex] = GetWorld()->GetTimeSeconds();
	HistoryIndex = (HistoryIndex + 1) % HistorySize;
	HistoryCount = FMath::Min(HistoryCount + 1, HistorySize);
}

void UVRHandComponent::ComputeVelocity(FVector& OutLinear, FVector& OutAngular) const
{
	OutLinear = FVector::ZeroVector;
	OutAngular = FVector::ZeroVector;
	if (HistoryCount < 2) return;

	const int32 Newest = (HistoryIndex - 1 + HistorySize) % HistorySize;
	const int32 Oldest = (HistoryIndex - HistoryCount + HistorySize) % HistorySize;
	const float Dt = TimeHistory[Newest] - TimeHistory[Oldest];
	if (Dt <= KINDA_SMALL_NUMBER) return;

	OutLinear = (PosHistory[Newest] - PosHistory[Oldest]) / Dt;

	const FQuat Delta = RotHistory[Newest] * RotHistory[Oldest].Inverse();
	FVector Axis;
	float Angle;
	Delta.ToAxisAndAngle(Axis, Angle);
	if (Angle > PI)
	{
		Angle -= 2.f * PI;
	}
	OutAngular = Axis * FMath::RadiansToDegrees(Angle) / Dt;
}

void UVRHandComponent::UpdateHover()
{
	UHandGrabComponent* Obj = nullptr;
	UVRHingeComponent* Hinge = nullptr;
	if (!IsHolding())
	{
		FindCandidates(Obj, Hinge, HighlightRadius);
	}

	if (HoverObject.Get() != Obj)
	{
		if (HoverObject.IsValid()) HoverObject->SetHighlighted(false, HighlightMaterial);
		if (Obj)
		{
			Obj->SetHighlighted(true, HighlightMaterial);
			if (bHapticsOnHover) Pulse(0.2f, 0.03f);
		}
		HoverObject = Obj;
	}
	if (HoverHinge.Get() != Hinge)
	{
		if (HoverHinge.IsValid()) HoverHinge->SetHighlighted(false, HighlightMaterial);
		if (Hinge)
		{
			Hinge->SetHighlighted(true, HighlightMaterial);
			if (bHapticsOnHover) Pulse(0.2f, 0.03f);
		}
		HoverHinge = Hinge;
	}
}

void UVRHandComponent::UpdateHandMesh()
{
	if (!HandMesh || !Controller) return;

	const FTransform Desired = HandMeshRelative * Controller->GetComponentTransform();
	const FVector Target = Desired.GetLocation();

	if (!bBlockHandMesh || IsHolding() || !bHasVisualHand
		|| FVector::Dist(VisualHandLocation, Target) > MaxHandSeparation)
	{
		VisualHandLocation = Target;
		bHasVisualHand = true;
		HandMesh->SetWorldTransform(Desired);
		return;
	}

	UWorld* World = GetWorld();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VRHandBlock), false, GetOwner());
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);

	FHitResult Hit;
	FVector NewLoc = Target;
	// Max 3 tentativi: gli oggetti tenuti in mano (anche dall'altra mano) vengono ignorati.
	for (int32 Attempt = 0; World && Attempt < 3; ++Attempt)
	{
		if (!World->SweepSingleByObjectType(Hit, VisualHandLocation, Target, FQuat::Identity, Objects,
			FCollisionShape::MakeSphere(HandCollisionRadius), Params))
		{
			break;
		}
		const UHandGrabComponent* G = Hit.GetActor() ? Hit.GetActor()->FindComponentByClass<UHandGrabComponent>() : nullptr;
		if (G && G->IsGrabbed())
		{
			Params.AddIgnoredActor(Hit.GetActor());
			continue;
		}
		// Se gia' dentro (start penetrating) lasciamo la mano libera per non incastrarla.
		NewLoc = Hit.bStartPenetrating ? Target : Hit.Location;
		break;
	}

	VisualHandLocation = NewLoc;
	HandMesh->SetWorldLocationAndRotation(NewLoc, Desired.GetRotation());
}

void UVRHandComponent::UpdateCurl(float DeltaTime)
{
	const float Target = HeldObject ? HeldObject->GetGripCurl() : (HeldHinge ? 0.9f : 0.f);
	CurrentCurl = FMath::FInterpTo(CurrentCurl, Target, DeltaTime, 14.f);

	if (!HandMesh || GripCurlVariable.IsNone()) return;
	UAnimInstance* Anim = HandMesh->GetAnimInstance();
	if (!Anim) return;

	FProperty* Prop = Anim->GetClass()->FindPropertyByName(GripCurlVariable);
	if (FDoubleProperty* D = CastField<FDoubleProperty>(Prop))
	{
		D->SetPropertyValue_InContainer(Anim, CurrentCurl);
	}
	else if (FFloatProperty* F = CastField<FFloatProperty>(Prop))
	{
		F->SetPropertyValue_InContainer(Anim, CurrentCurl);
	}
}

void UVRHandComponent::UpdateTouch()
{
	if (!bTouchOpensCards || IsHolding()) return;
	UWorld* World = GetWorld();
	if (!World) return;

	const FVector Tip = GetPalm();
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VRHandTouch), false, GetOwner());
	World->OverlapMultiByObjectType(Overlaps, Tip, FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
		FCollisionShape::MakeSphere(TouchRadius + HandCollisionRadius), Params);

	for (const FOverlapResult& O : Overlaps)
	{
		if (AActor* A = O.GetActor())
		{
			if (UGrabInfoTriggerComponent* T = A->FindComponentByClass<UGrabInfoTriggerComponent>())
			{
				T->NotifyTouched();
			}
		}
	}
}

void UVRHandComponent::Pulse(float Amplitude, float Duration)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	UMotionControllerComponent* MC = Cast<UMotionControllerComponent>(Controller);
	if (!PC || !MC || !GetWorld()) return;

	const EControllerHand Hand = MC->GetTrackingMotionSource().ToString().Contains(TEXT("Left"))
		? EControllerHand::Left : EControllerHand::Right;
	PC->SetHapticsByValue(1.f, Amplitude, Hand);
	GetWorld()->GetTimerManager().SetTimer(HapticTimer, this, &UVRHandComponent::StopPulse, Duration, false);
}

void UVRHandComponent::StopPulse()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	UMotionControllerComponent* MC = Cast<UMotionControllerComponent>(Controller);
	if (!PC || !MC) return;
	const EControllerHand Hand = MC->GetTrackingMotionSource().ToString().Contains(TEXT("Left"))
		? EControllerHand::Left : EControllerHand::Right;
	PC->SetHapticsByValue(0.f, 0.f, Hand);
}

void UVRHandComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Controller) return;

	// Se l'oggetto e' stato preso dall'altra mano o distrutto.
	if (HeldObject && !HeldObject->IsGrabbed()) HeldObject = nullptr;
	if (HeldHinge && !HeldHinge->IsGrabbed()) HeldHinge = nullptr;

	RecordHistory();
	UpdateHover();
	UpdateHandMesh();
	UpdateCurl(DeltaTime);
	UpdateTouch();
}
