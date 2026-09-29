#include "NPCGazeGreeterComponent.h"
#include "GlassCardActor.h"
#include "Kismet/GameplayStatics.h"

UNPCGazeGreeterComponent::UNPCGazeGreeterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.15f; // ~7 controlli al secondo
}

bool UNPCGazeGreeterComponent::CanSeePlayer() const
{
	const AActor* Owner = GetOwner();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Owner || !Player) return false;

	// 1) Distanza
	const FVector ToPlayer = Player->GetActorLocation() - Owner->GetActorLocation();
	if (ToPlayer.Size() > SightRadius) return false;

	// 2) Angolo: il player è dentro il cono davanti all'NPC?
	const float CosAngle = FVector::DotProduct(
		Owner->GetActorForwardVector(), ToPlayer.GetSafeNormal());
	if (CosAngle < FMath::Cos(FMath::DegreesToRadians(SightHalfAngleDeg)))
		return false;

	// 3) Linea di vista: c'è un muro in mezzo?
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);
	const FVector EyeStart = Owner->GetActorLocation() + FVector(0, 0, 60.f);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		Hit, EyeStart, Player->GetActorLocation(), ECC_Visibility, Params);

	return !bBlocked || Hit.GetActor() == Player;
}

void UNPCGazeGreeterComponent::Trigger()
{
	bTriggered = true;

	if (VoiceLine)
	{
		UGameplayStatics::PlaySoundAtLocation(this, VoiceLine,
			GetOwner()->GetActorLocation());
	}
	if (LinkedCard) { LinkedCard->Open(); }
}

void UNPCGazeGreeterComponent::TickComponent(float DeltaTime,
	ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bTriggered && bTriggerOnce) return;

	if (CanSeePlayer())
	{
		GazeTime += 0.15f; // = TickInterval
		if (GazeTime >= GazeSecondsRequired && !bTriggered) { Trigger(); }
	}
	else
	{
		GazeTime = 0.f;                    // il timer riparte da zero
		if (!bTriggerOnce) bTriggered = false; // riattivabile
	}
}