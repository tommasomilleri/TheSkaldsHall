#include "ObjectPoolComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

void UObjectPoolComponent::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) return;
	if (!PooledClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ObjectPool] %s: PooledClass non impostata."), *GetNameSafe(GetOwner()));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Available.Reserve(PoolSize);
	All.Reserve(PoolSize);

	for (int32 i = 0; i < PoolSize; ++i)
	{
		AActor* A = World->SpawnActor<AActor>(PooledClass, FVector(0.f, 0.f, -10000.f), FRotator::ZeroRotator, SpawnParams);
		if (!A) continue;
		SetActive(A, false);
		Available.Add(A);
		All.Add(A);
	}

	UE_LOG(LogTemp, Log, TEXT("[ObjectPool] Pre-istanziati %d/%d attori di %s."), All.Num(), PoolSize, *PooledClass->GetName());
}

void UObjectPoolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AActor* A : All)
	{
		if (IsValid(A))
		{
			A->Destroy();
		}
	}
	All.Empty();
	Available.Empty();
	Super::EndPlay(EndPlayReason);
}

AActor* UObjectPoolComponent::Acquire(const FTransform& SpawnTransform)
{
	while (Available.Num() > 0)
	{
		AActor* A = Available.Pop();
		if (!IsValid(A)) continue;
		A->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
		SetActive(A, true);
		return A;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ObjectPool] Pool esaurito (%s). Aumenta PoolSize."),
		PooledClass ? *PooledClass->GetName() : TEXT("null"));
	return nullptr;
}

void UObjectPoolComponent::Release(AActor* Actor)
{
	if (!IsValid(Actor)) return;
	if (Available.Contains(Actor)) return;

	SetActive(Actor, false);
	Actor->SetActorLocation(FVector(0.f, 0.f, -10000.f), false, nullptr, ETeleportType::TeleportPhysics);
	Available.Add(Actor);
}

void UObjectPoolComponent::SetActive(AActor* Actor, bool bActive) const
{
	Actor->SetActorHiddenInGame(!bActive);
	Actor->SetActorEnableCollision(bActive);
	Actor->SetActorTickEnabled(bActive);

	if (UPrimitiveComponent* Prim = Actor->FindComponentByClass<UPrimitiveComponent>())
	{
		Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Prim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		Prim->SetSimulatePhysics(bActive && Prim->BodyInstance.bSimulatePhysics);
	}
}