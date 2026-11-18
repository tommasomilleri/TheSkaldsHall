#include "LiquidComponent.h"
#include "VRHingeComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"

ULiquidComponent::ULiquidComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void ULiquidComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<USceneComponent*> Comps;
	Owner->GetComponents<USceneComponent>(Comps);
	for (USceneComponent* C : Comps)
	{
		if (!C) continue;
		if (C->ComponentHasTag(TEXT("Liquid")))
		{
			LiquidMesh = Cast<UStaticMeshComponent>(C);
		}
		if (C->ComponentHasTag(TEXT("Spout")))
		{
			Spout = C;
		}
	}
	Tap = Owner->FindComponentByClass<UVRHingeComponent>();

	if (LiquidMesh)
	{
		LiquidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LiquidMesh->SetCastShadow(false);
		LiquidMID = LiquidMesh->CreateDynamicMaterialInstance(0);
	}

	USceneComponent* AttachTo = Spout ? Spout.Get() : Owner->GetRootComponent();
	if (PourEffect && AttachTo)
	{
		StreamFX = NewObject<UNiagaraComponent>(Owner, TEXT("LiquidStreamFX"));
		StreamFX->SetAsset(PourEffect);
		StreamFX->SetAutoActivate(false);
		StreamFX->SetupAttachment(AttachTo);
		StreamFX->SetUsingAbsoluteRotation(true);
		StreamFX->RegisterComponent();
	}
	if (PourSound && AttachTo)
	{
		StreamAudio = NewObject<UAudioComponent>(Owner, TEXT("LiquidStreamAudio"));
		StreamAudio->SetSound(PourSound);
		StreamAudio->bAutoActivate = false;
		StreamAudio->SetupAttachment(AttachTo);
		StreamAudio->RegisterComponent();
	}

	UpdateVisual();
}

void ULiquidComponent::AddLiquid(float Milliliters)
{
	if (bInfiniteSource) return;
	Fill = FMath::Clamp(Fill + Milliliters / CapacityMl, 0.f, 1.f);
}

FVector ULiquidComponent::GetSpoutLocation() const
{
	if (Spout) return Spout->GetComponentLocation();

	// Senza Spout: bordo superiore dalla parte piu' bassa (dove il liquido esce inclinando).
	const AActor* Owner = GetOwner();
	if (!Owner) return FVector::ZeroVector;
	FVector Origin, Extent;
	Owner->GetActorBounds(true, Origin, Extent);
	const FVector Up = Owner->GetActorUpVector();
	FVector Side = FVector(Up.X, Up.Y, 0.f);
	Side = Side.IsNearlyZero() ? FVector::ZeroVector : Side.GetSafeNormal();
	return Origin + Up * Extent.Z + Side * FMath::Min(Extent.X, Extent.Y);
}

float ULiquidComponent::ComputeFlow() const
{
	if (Tap)
	{
		const float Open = Tap->GetNormalizedAngle();
		return Open > MinTapOpening ? Open : 0.f;
	}
	if (Fill <= 0.f || !GetOwner()) return 0.f;

	// Piu' e' pieno, prima comincia a versare inclinandolo.
	const float UpZ = GetOwner()->GetActorUpVector().Z;
	const float Threshold = FMath::Lerp(-0.3f, 0.8f, Fill);
	return UpZ < Threshold ? FMath::Clamp((Threshold - UpZ) / 0.4f, 0.f, 1.f) : 0.f;
}

void ULiquidComponent::UpdateVisual()
{
	if (!LiquidMesh) return;

	LiquidMesh->SetVisibility(Fill > 0.005f);
	if (!LiquidMID) return;

	const FVector Origin = LiquidMesh->Bounds.Origin;
	const FVector Extent = LiquidMesh->Bounds.BoxExtent;
	const float FillZ = FMath::Lerp(Origin.Z - Extent.Z, Origin.Z + Extent.Z, Fill);
	LiquidMID->SetScalarParameterValue(TEXT("FillZ"), FillZ);
	LiquidMID->SetScalarParameterValue(TEXT("FillAmount"), Fill);
	LiquidMID->SetVectorParameterValue(TEXT("LiquidColor"), LiquidColor);
}

void ULiquidComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CurrentFlow = ComputeFlow();
	const bool bFlowing = CurrentFlow > 0.f && (bInfiniteSource || Fill > 0.f);

	if (bFlowing)
	{
		const float Amount = PourRateMlPerSec * CurrentFlow * DeltaTime;
		if (!bInfiniteSource)
		{
			Fill = FMath::Max(Fill - Amount / CapacityMl, 0.f);
		}

		const FVector Start = GetSpoutLocation();
		const FVector End = Start - FVector(0.f, 0.f, MaxStreamLength);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(LiquidStream), false, GetOwner());
		float Length = MaxStreamLength;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			Length = Hit.Distance;
			if (AActor* A = Hit.GetActor())
			{
				if (ULiquidComponent* Receiver = A->FindComponentByClass<ULiquidComponent>())
				{
					Receiver->AddLiquid(Amount);
				}
			}
		}

		if (StreamFX)
		{
			// Roll 180: l'asse Z del sistema Niagara (direzione del Fountain) punta verso il basso.
			StreamFX->SetWorldLocationAndRotation(Start, FRotator(0.f, 0.f, 180.f));
			StreamFX->SetVariableFloat(TEXT("User.Flow"), CurrentFlow);
			StreamFX->SetVariableFloat(TEXT("User.Length"), Length);
			StreamFX->SetVariableLinearColor(TEXT("User.Color"), LiquidColor);
		}
		if (StreamAudio)
		{
			StreamAudio->SetVolumeMultiplier(FMath::Lerp(0.3f, 1.f, CurrentFlow));
		}
		if (!bStreamOn)
		{
			bStreamOn = true;
			if (StreamFX) StreamFX->Activate(true);
			if (StreamAudio) StreamAudio->FadeIn(0.15f);
		}
	}
	else if (bStreamOn)
	{
		bStreamOn = false;
		if (StreamFX) StreamFX->Deactivate();
		if (StreamAudio) StreamAudio->FadeOut(0.25f, 0.f);
	}

	UpdateVisual();
}
