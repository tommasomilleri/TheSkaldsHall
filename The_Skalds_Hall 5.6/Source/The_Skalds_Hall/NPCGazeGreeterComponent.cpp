#include "NPCGazeGreeterComponent.h"
#include "GlassCardActor.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UNPCGazeGreeterComponent::UNPCGazeGreeterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.15f;
}

bool UNPCGazeGreeterComponent::CanSeePlayer() const
{
	if (!GetWorld()) return false;
	const AActor* Owner = GetOwner();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Owner || !Player) return false;

	FVector PlayerEye = Player->GetActorLocation();
	if (const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		PlayerEye = Cam->GetCameraLocation();
	}

	const FVector ToPlayer = PlayerEye - Owner->GetActorLocation();
	if (ToPlayer.Size() > SightRadius) return false;

	const float CosAngle = FVector::DotProduct(
		Owner->GetActorForwardVector(), ToPlayer.GetSafeNormal2D());
	if (CosAngle < FMath::Cos(FMath::DegreesToRadians(SightHalfAngleDeg)))
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NPCGaze), false);
	Params.AddIgnoredActor(Owner);
	Params.AddIgnoredActor(Player);
	FVector EyeStart = Owner->GetActorLocation() + FVector(0.f, 0.f, 160.f);
	if (Owner->IsA<APawn>())
	{
		FRotator EyeRotation;
		Owner->GetActorEyesViewPoint(EyeStart, EyeRotation);
	}
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		Hit, EyeStart, PlayerEye, ECC_Visibility, Params);

	return !bBlocked;
}

void UNPCGazeGreeterComponent::Trigger()
{
	bTriggered = true;
	GazeTime = 0.f;

	if (VoiceLine)
	{
		UGameplayStatics::PlaySoundAtLocation(this, VoiceLine,
			GetOwner()->GetActorLocation());
	}
	if (LinkedCard)
	{
		LinkedCard->Open();
	}
	OnGreet.Broadcast();
}

void UNPCGazeGreeterComponent::TickComponent(float DeltaTime,
	ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bTriggered && bTriggerOnce) return;

	if (CanSeePlayer())
	{
		GazeTime += DeltaTime;
		if (GazeTime >= GazeSecondsRequired && !bTriggered)
		{
			Trigger();
		}
	}
	else
	{
		GazeTime = 0.f;
		if (!bTriggerOnce)
		{
			bTriggered = false;
		}
	}
}