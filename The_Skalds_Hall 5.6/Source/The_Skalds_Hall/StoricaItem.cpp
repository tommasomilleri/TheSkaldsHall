#include "StoricaItem.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
AStoricaItem::AStoricaItem()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	Mesh->SetSimulatePhysics(true);
	Mesh->SetNotifyRigidBodyCollision(true);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
}
void AStoricaItem::BeginPlay()
{
	Super::BeginPlay();
	Mesh->OnComponentHit.AddDynamic(this, &AStoricaItem::OnItemHit);
}
void AStoricaItem::OnItemHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
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