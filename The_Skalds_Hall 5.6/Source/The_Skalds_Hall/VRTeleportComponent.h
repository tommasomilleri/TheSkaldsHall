#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VRTeleportComponent.generated.h"
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

UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UVRTeleportComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVRTeleportComponent();
	UPROPERTY(BlueprintAssignable, Category = "VR|Teleport") FOnTeleportArcUpdated OnArcUpdated;
	UPROPERTY(BlueprintAssignable, Category = "VR|Teleport") FOnTeleported OnTeleported;
	/** Inizia a mirare (chiamalo quando spingi lo stick avanti). */
	UFUNCTION(BlueprintCallable, Category = "VR|Teleport")
	void StartAiming(USceneComponent* AimSource);

	/** Conferma: blink + teleport. SnapYaw = rotazione snap pre-teleport
	 *  (es. direzione del thumbstick al rilascio). */
	UFUNCTION(BlueprintCallable, Category = "VR|Teleport")
	void ConfirmTeleport(float SnapYaw = 0.f);

	/** Annulla la mira senza teleportare. */
	UFUNCTION(BlueprintCallable, Category = "VR|Teleport")
	void CancelAiming();

	UFUNCTION(BlueprintPure, Category = "VR|Teleport")
	bool HasValidDestination() const { return bHasValidDest; }

	/** Velocità iniziale dell'arco (cm/s). Più alta = arco più lungo. */
	UPROPERTY(EditAnywhere, Category = "VR|Teleport", meta = (ClampMin = "100.0"))
	float LaunchSpeed = 900.f;

	/** Durata fade a nero (blink out). Alyx usa ~0.1s. */
	UPROPERTY(EditAnywhere, Category = "VR|Teleport", meta = (ClampMin = "0.01"))
	float FadeOutTime = 0.1f;

	/** Durata ritorno dal nero (blink in). */
	UPROPERTY(EditAnywhere, Category = "VR|Teleport", meta = (ClampMin = "0.01"))
	float FadeInTime = 0.15f;

	/** Tolleranza di proiezione sulla NavMesh (cm). */
	UPROPERTY(EditAnywhere, Category = "VR|Teleport")
	FVector NavProjectionExtent = FVector(50.f, 50.f, 100.f);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY() TObjectPtr<USceneComponent> Source;
	FVector ValidDestination = FVector::ZeroVector;
	bool bHasValidDest = false;
	bool bTeleportInProgress = false;

	void DoTeleport(float SnapYaw);
	FTimerHandle FadeTimer;
};

