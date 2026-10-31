#include "SignificanceManagerComponent.h"
#include "NPCDialogueComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/LODSyncComponent.h"
#include "Kismet/GameplayStatics.h"

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
	}
	if (SkelMeshes.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Significance] %s: nessuna SkeletalMeshComponent."),
			*GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
	}
}

// In TickComponent: sostituisci "if (!SkelMesh) return;" con:
if (SkelMeshes.Num() == 0) return;
if (bRespectDialogue && Dialogue && Dialogue->IsSpeaking()) return;

void USignificanceManagerComponent::ApplySignificance(bool bSignificant)
{
	for (USkeletalMeshComponent* M : SkelMeshes)
	{
		if (!M) continue;
		M->VisibilityBasedAnimTickOption = bSignificant
			? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones
			: EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		if (!LODSync) // senza LODSync forziamo noi (1-based, clamp ai LOD reali)
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