#include "StoricaItemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
UStoricaItemComponent::UStoricaItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}
void UStoricaItemComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Trova la prima primitive component dell'owner (di solito la StaticMesh radice)
	UPrimitiveComponent* Prim = Owner->FindComponentByClass<UPrimitiveComponent>();
	if (!Prim)
	{
		return;
	}

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
	const float Impulse = NormalImpulse.Size();
	if (Impulse < MinImpulse)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastImpactTime < ImpactCooldown)
	{
		return;
	}
	LastImpactTime = Now;

	const float Volume = FMath::GetMappedRangeValueClamped(
		FVector2D(MinImpulse, MaxImpulse), FVector2D(0.15f, 1.f), Impulse);

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint, Volume);
	}

	if (ImpactVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactVFX,
			Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}
}