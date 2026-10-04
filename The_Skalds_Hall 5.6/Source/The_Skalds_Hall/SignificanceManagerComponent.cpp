#include "SignificanceManagerComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

USignificanceManagerComponent::USignificanceManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f; // 4 valutazioni/secondo bastano
}

void USignificanceManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* Owner = GetOwner())
	{
		SkelMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}
	if (!SkelMesh)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Significance] %s: nessuna SkeletalMeshComponent trovata."),
			*GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
	}
}

void USignificanceManagerComponent::TickComponent(float DeltaTime,
	ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!SkelMesh) return;

	const APlayerCameraManager* Cam =
		UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Cam) return;

	const FVector ToNPC =
		GetOwner()->GetActorLocation() - Cam->GetCameraLocation();
	const float DistSq = ToNPC.SizeSquared();

	// Dot < MinViewDot = l'NPC è fuori dal campo visivo (dietro/di lato)
	const float Dot = FVector::DotProduct(
		Cam->GetActorForwardVector(), ToNPC.GetSafeNormal());

	const bool bSignificant =
		DistSq <= FMath::Square(MaxSignificantDistance) && Dot >= MinViewDot;

	if (bSignificant == bWasSignificant) return; // reagiamo SOLO al cambio
	bWasSignificant = bSignificant;
	ApplySignificance(bSignificant);
}

void USignificanceManagerComponent::ApplySignificance(bool bSignificant)
{
	if (bSignificant)
	{
		// NPC vicino e in vista: animazioni complete, LOD automatico
		SkelMesh->VisibilityBasedAnimTickOption =
			EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		SkelMesh->SetForcedLOD(0); // 0 = torna alla selezione automatica
	}
	else
	{
		// NPC lontano o dietro di te: pose solo se renderizzato, LOD basso.
		// ATTENZIONE: SetForcedLOD è 1-BASED (1 = LOD0), quindi +1.
		SkelMesh->VisibilityBasedAnimTickOption =
			EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		SkelMesh->SetForcedLOD(InsignificantLOD + 1);
	}
}
