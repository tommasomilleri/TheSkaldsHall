#include "NPCGazeGreeterComponent.h"
#include "GlassCardActor.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"          // FIX 2

UNPCGazeGreeterComponent::UNPCGazeGreeterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.15f; // ~7 controlli al secondo
}

bool UNPCGazeGreeterComponent::CanSeePlayer() const
{
	if (!GetWorld()) return false;
	const AActor* Owner = GetOwner();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Owner || !Player) return false;

	// FIX 2: in VR il "player" e' la TESTA (camera HMD), non la capsula ai piedi.
	// Se il player e' seduto o dietro un tavolo, il trace verso i piedi
	// colpisce il tavolo e l'NPC "non ti vede". Usiamo la camera.
	FVector PlayerEye = Player->GetActorLocation();
	if (const APlayerCameraManager* Cam =
		UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		PlayerEye = Cam->GetCameraLocation();
	}

	// 1) Distanza
	const FVector ToPlayer = PlayerEye - Owner->GetActorLocation();
	if (ToPlayer.Size() > SightRadius) return false;

	// 2) Angolo: il player e' dentro il cono davanti all'NPC?
	const float CosAngle = FVector::DotProduct(
		Owner->GetActorForwardVector(), ToPlayer.GetSafeNormal2D());
	if (CosAngle < FMath::Cos(FMath::DegreesToRadians(SightHalfAngleDeg)))
		return false;

	// 3) Linea di vista: c'e' un muro in mezzo?
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NPCGaze), false);
	Params.AddIgnoredActor(Owner);
	Params.AddIgnoredActor(Player);
	const FVector EyeStart = Owner->GetActorLocation() + FVector(0, 0, 160.f); // altezza occhi NPC
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		Hit, EyeStart, PlayerEye, ECC_Visibility, Params);

	return !bBlocked;
}

void UNPCGazeGreeterComponent::Trigger()
{
	bTriggered = true;
	GazeTime = 0.f;                                  // FIX 3: reset per il ri-trigger

	if (VoiceLine)
	{
		UGameplayStatics::PlaySoundAtLocation(this, VoiceLine,
			GetOwner()->GetActorLocation());
	}
	if (LinkedCard) { LinkedCard->Open(); }
	OnGreet.Broadcast();                             // hook per il lip sync (vedi Parte 3)
}

void UNPCGazeGreeterComponent::TickComponent(float DeltaTime,
	ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bTriggered && bTriggerOnce) return;

	if (CanSeePlayer())
	{
		// FIX 1: usa DeltaTime reale. Con TickInterval>0, DeltaTime che arriva
		// qui e' GIA' il tempo accumulato dall'ultimo tick (non il frame time).
		GazeTime += DeltaTime;
		if (GazeTime >= GazeSecondsRequired && !bTriggered) { Trigger(); }
	}
	else
	{
		GazeTime = 0.f;
		if (!bTriggerOnce) bTriggered = false;
	}
}
