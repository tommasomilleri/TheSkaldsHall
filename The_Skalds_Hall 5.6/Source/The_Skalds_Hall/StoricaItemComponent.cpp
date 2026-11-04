#include "StoricaItemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"

UStoricaItemComponent::UStoricaItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStoricaItemComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	if (!Owner) return;

	UPrimitiveComponent* Prim = Owner->FindComponentByClass<UPrimitiveComponent>();
	if (!Prim) return;

	if (bEnablePhysics)
	{
		Prim->SetSimulatePhysics(true);
	}
	Prim->SetNotifyRigidBodyCollision(true);
	Prim->OnComponentHit.AddDynamic(this, &UStoricaItemComponent::OnItemHit);
}

void UStoricaItemComponent::OnItemHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!GetWorld()) return;

	const float Mass = HitComp ? FMath::Max(HitComp->GetMass(), 0.01f) : 1.f;
	const float DeltaV = NormalImpulse.Size() / Mass;
	if (DeltaV < MinImpulse) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastImpactTime < ImpactCooldown) return;
	LastImpactTime = Now;

	const float Volume = FMath::GetMappedRangeValueClamped(
		FVector2D(MinImpulse, MaxImpulse), FVector2D(0.15f, 1.f), DeltaV);

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint,
			Volume, 1.f, 0.f, ImpactAttenuation);
	}

	if (ImpactVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactVFX,
			Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}
}