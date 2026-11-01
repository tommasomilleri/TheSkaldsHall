#include "VRTeleportComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"
#include "Engine/World.h"

UVRTeleportComponent::UVRTeleportComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UVRTeleportComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
	static const TArray<FVector> Empty;
	OnArcUpdated.Broadcast(Empty, FVector::ZeroVector, false);
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

	FPredictProjectilePathParams Params(
		0.f,
		Source->GetComponentLocation(),
		Source->GetForwardVector() * LaunchSpeed,
		2.f);
	Params.bTraceWithCollision = true;
	Params.TraceChannel = ECC_WorldStatic;
	Params.SimFrequency = 15.f;
	Params.ActorsToIgnore.Add(GetOwner());

	FPredictProjectilePathResult Result;
	bHasValidDest = false;

	if (UGameplayStatics::PredictProjectilePath(this, Params, Result))
	{
		if (const UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(World))
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

	TArray<FVector> Pts;
	Pts.Reserve(Result.PathData.Num());
	for (const FPredictProjectilePathPointData& P : Result.PathData)
	{
		Pts.Add(P.Location);
	}
	OnArcUpdated.Broadcast(Pts, ValidDestination, bHasValidDest);

#if ENABLE_DRAW_DEBUG
	const FColor Col = bHasValidDest ? FColor::Cyan : FColor::Red;
	for (int32 i = 1; i < Result.PathData.Num(); ++i)
	{
		DrawDebugLine(World, Result.PathData[i - 1].Location,
			Result.PathData[i].Location, Col, false, -1.f, 0, 0.6f);
	}
	if (bHasValidDest)
	{
		DrawDebugCylinder(World, ValidDestination,
			ValidDestination + FVector(0.f, 0.f, 2.f), 35.f, 24, Col, false, -1.f);
	}
#endif
}

void UVRTeleportComponent::ConfirmTeleport(float SnapYaw)
{
	if (bTeleportInProgress) return;
	if (!bHasValidDest)
	{
		CancelAiming();
		return;
	}

	UWorld* World = GetWorld();
	APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);

	if (!World || !Cam)
	{
		DoTeleport(SnapYaw);
		return;
	}

	bTeleportInProgress = true;
	SetComponentTickEnabled(false);

	Cam->StartCameraFade(0.f, 1.f, FadeOutTime, FLinearColor::Black, false, true);

	TWeakObjectPtr<UVRTeleportComponent> WeakThis(this);
	World->GetTimerManager().SetTimer(FadeTimer,
		[WeakThis, SnapYaw]()
		{
			if (!WeakThis.IsValid()) return;
			WeakThis->DoTeleport(SnapYaw);
			if (APlayerCameraManager* C = UGameplayStatics::GetPlayerCameraManager(WeakThis.Get(), 0))
			{
				C->StartCameraFade(1.f, 0.f, WeakThis->FadeInTime, FLinearColor::Black);
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
		OnTeleported.Broadcast(ValidDestination);
	}
	CancelAiming();
}