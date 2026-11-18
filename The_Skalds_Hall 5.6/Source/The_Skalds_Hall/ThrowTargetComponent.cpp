#include "ThrowTargetComponent.h"
#include "Components/TextRenderComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

UThrowTargetComponent::UThrowTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UThrowTargetComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	if (!Owner) return;

	ScoreText = Owner->FindComponentByClass<UTextRenderComponent>();
	Center = Owner->GetRootComponent();
	TArray<USceneComponent*> Comps;
	Owner->GetComponents<USceneComponent>(Comps);
	for (USceneComponent* C : Comps)
	{
		if (C && C->ComponentHasTag(TEXT("Center")))
		{
			Center = C;
		}
	}
	UpdateText(-1);
}

int32 UThrowTargetComponent::RegisterHit(const FVector& WorldLocation)
{
	if (!Center) return 0;

	if (ThrowsPerRound > 0 && Throws >= ThrowsPerRound)
	{
		ResetScore();
	}

	// Distanza dal centro sul piano del bersaglio (la faccia guarda +X del Center).
	const FVector Local = Center->GetComponentTransform().InverseTransformPositionNoScale(WorldLocation);
	const float Dist = FVector2D(Local.Y, Local.Z).Size();

	int32 Points = 0;
	for (int32 i = 0; i < RingRadii.Num() && i < RingPoints.Num(); ++i)
	{
		if (Dist <= RingRadii[i])
		{
			Points = RingPoints[i];
			break;
		}
	}

	++Throws;
	TotalScore += Points;

	USoundBase* Sound = (Points > 0 && RingPoints.Num() > 0 && Points == RingPoints[0] && BullseyeSound) ? BullseyeSound : HitSound;
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, WorldLocation);
	}

	UpdateText(Points);
	OnScored.Broadcast(Points, TotalScore);
	return Points;
}

void UThrowTargetComponent::ResetScore()
{
	TotalScore = 0;
	Throws = 0;
	UpdateText(-1);
}

void UThrowTargetComponent::UpdateText(int32 LastPoints)
{
	if (!ScoreText) return;
	FString S = FString::Printf(TEXT("Punti: %d"), TotalScore);
	if (ThrowsPerRound > 0)
	{
		S += FString::Printf(TEXT("\nLanci: %d/%d"), Throws, ThrowsPerRound);
	}
	if (LastPoints >= 0)
	{
		S += FString::Printf(TEXT("\n+%d"), LastPoints);
	}
	ScoreText->SetText(FText::FromString(S));
}
