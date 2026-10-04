#include "VRTeleportComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Engine/World.h"

UVRTeleportComponent::UVRTeleportComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Tick attivo SOLO mentre si mira: costo zero a riposo
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UVRTeleportComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Sicurezza: niente lambda orfane se il pawn muore durante il blink
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FadeTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UVRTeleportComponent::StartAiming(USceneComponent* AimSource)
{
	if (bTeleportInProgress || !AimSource) return;
	Source = AimSource;
	SetComponentTickEnabled(true);
}

void UVRTeleportComponent::CancelAiming()
{
	Source = nullptr;
	bHasValidDest = false;
	SetComponentTickEnabled(false);
}

void UVRTeleportComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	if (!Source || !World)
	{
		CancelAiming();
		return;
	}

	// 1) Arco parabolico dal controller
	FPredictProjectilePathParams Params(
		0.f,                                      // raggio del proiettile
		Source->GetComponentLocation(),           // partenza
		Source->GetForwardVector() * LaunchSpeed, // velocità iniziale
		2.f);                                     // durata max simulata (s)
	Params.bTraceWithCollision = true;
	Params.TraceChannel = ECC_WorldStatic;
	Params.SimFrequency = 15.f;
	Params.ActorsToIgnore.Add(GetOwner()); // non colpire il pawn stesso

	FPredictProjectilePathResult Result;
	bHasValidDest = false;

	if (UGameplayStatics::PredictProjectilePath(this, Params, Result))
	{
		// 2) Valida sulla NavMesh: niente teleport su tavoli/muri/NPC
		if (const UNavigationSystemV1* Nav =
			UNavigationSystemV1::GetCurrent(World))
		{
			FNavLocation NavLoc;
			if (Nav->ProjectPointToNavigation(
				Result.HitResult.Location, NavLoc, NavProjectionExtent))
			{
				ValidDestination = NavLoc.Location;
				bHasValidDest = true;
			}
		}
	}

	// 3) Visual placeholder (sostituibile con Niagara ribbon + decal)
	const FColor Col = bHasValidDest ? FColor::Cyan : FColor::Red;
	for (int32 i = 1; i < Result.PathData.Num(); ++i)
	{
		DrawDebugLine(World, Result.PathData[i - 1].Location,
			Result.PathData[i].Location, Col, false, -1.f, 0, 0.6f);
	}
	if (bHasValidDest)
	{
		DrawDebugCylinder(World, ValidDestination,
			ValidDestination + FVector(0, 0, 2.f), 35.f, 24, Col, false, -1.f);
	}
}

void UVRTeleportComponent::ConfirmTeleport(float SnapYaw)
{
	if (bTeleportInProgress) return;
	if (!bHasValidDest) { CancelAiming(); return; }

	UWorld* World = GetWorld();
	APlayerCameraManager* Cam =
		UGameplayStatics::GetPlayerCameraManager(this, 0);

	if (!World || !Cam) { DoTeleport(SnapYaw); return; }

	bTeleportInProgress = true;
	SetComponentTickEnabled(false); // spegni la mira durante il blink

	// "Blink" alla Alyx: fade a nero, teleport al buio, fade di ritorno.
	// bHoldWhenFinished=true tiene lo schermo nero finché non facciamo
	// il fade-in: il giocatore NON vede mai lo spostamento.
	Cam->StartCameraFade(0.f, 1.f, FadeOutTime, FLinearColor::Black,
		/*bShouldFadeAudio=*/false, /*bHoldWhenFinished=*/true);

	TWeakObjectPtr<UVRTeleportComponent> WeakThis(this);
	World->GetTimerManager().SetTimer(FadeTimer,
		[WeakThis, SnapYaw]()
		{
			if (!WeakThis.IsValid()) return; // componente distrutto nel frattempo
			WeakThis->DoTeleport(SnapYaw);
			if (APlayerCameraManager* C =
				UGameplayStatics::GetPlayerCameraManager(WeakThis.Get(), 0))
			{
				C->StartCameraFade(1.f, 0.f, WeakThis->FadeInTime,
					FLinearColor::Black);
			}
			WeakThis->bTeleportInProgress = false;
		},
		FadeOutTime, false);
}

void UVRTeleportComponent::DoTeleport(float SnapYaw)
{
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		FRotator NewRot = Pawn->GetActorRotation();
		NewRot.Yaw += SnapYaw;
		Pawn->TeleportTo(ValidDestination, NewRot);
	}
	CancelAiming();
}
