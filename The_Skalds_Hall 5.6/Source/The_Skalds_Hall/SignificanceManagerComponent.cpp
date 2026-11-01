#include "SignificanceManagerComponent.h"
#include "NPCDialogueComponent.h"
#include "NPCRoutineComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/LODSyncComponent.h"
#include "Camera/PlayerCameraManager.h"
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
		TArray<USkeletalMeshComponent*> Found;
		Owner->GetComponents<USkeletalMeshComponent>(Found);
		SkelMeshes.Append(Found);

		LODSync = Owner->FindComponentByClass<ULODSyncComponent>();
		Dialogue = Owner->FindComponentByClass<UNPCDialogueComponent>();
		Routine = Owner->FindComponentByClass<UNPCRoutineComponent>();
	}

	if (SkelMeshes.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Significance] %s: nessuna SkeletalMeshComponent."),
			*GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
	}
}

void USignificanceManagerComponent::TickComponent(float DeltaTime,
	ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (SkelMeshes.Num() == 0) return;

	// Mentre parla non tocchiamo LOD/tick (lip sync stabile)
	if (bRespectDialogue && Dialogue && Dialogue->IsSpeaking()) return;

	const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const AActor* Owner = GetOwner();
	if (!Cam || !Owner) return;

	const FVector ToNPC = Owner->GetActorLocation() - Cam->GetCameraLocation();
	const float DistSq = ToNPC.SizeSquared();

	// Dot < MinViewDot = l'NPC è fuori dal campo visivo (dietro/di lato)
	const float Dot = FVector::DotProduct(
		Cam->GetCameraRotation().Vector(), ToNPC.GetSafeNormal());

	const bool bSignificant =
		DistSq <= FMath::Square(MaxSignificantDistance) && Dot >= MinViewDot;

	if (bSignificant == bWasSignificant) return; // reagiamo SOLO al cambio
	bWasSignificant = bSignificant;
	ApplySignificance(bSignificant);

	if (Routine)
	{
		Routine->SetPaused(!bSignificant);
	}
}

void USignificanceManagerComponent::ApplySignificance(bool bSignificant)
{
	for (USkeletalMeshComponent* M : SkelMeshes)
	{
		if (!M) continue;

		M->VisibilityBasedAnimTickOption = bSignificant
			? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones
			: EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

		if (!LODSync) // senza LODSync forziamo noi (1-based, 0 = automatico)
		{
			const int32 NumLODs = FMath::Max(M->GetNumLODs(), 1);
			M->SetForcedLOD(bSignificant ? 0 : FMath::Clamp(InsignificantLOD + 1, 1, NumLODs));
		}
	}

	if (LODSync) // con LODSync il LOD si forza QUI (0-based, -1 = automatico)
	{
		LODSync->ForcedLOD = bSignificant ? -1 : InsignificantLOD;
	}
}