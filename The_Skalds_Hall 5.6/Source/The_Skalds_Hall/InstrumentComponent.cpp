#include "InstrumentComponent.h"
#include "HandGrabComponent.h"
#include "MotionControllerComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"

UInstrumentComponent::UInstrumentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UInstrumentComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	if (!Owner || !Music) return;

	Audio = NewObject<UAudioComponent>(Owner, TEXT("InstrumentAudio"));
	Audio->SetSound(Music);
	Audio->bAutoActivate = false;
	Audio->SetVolumeMultiplier(Volume);
	Audio->SetupAttachment(Owner->GetRootComponent());
	Audio->RegisterComponent();
}

bool UInstrumentComponent::IsHeld() const
{
	const AActor* Owner = GetOwner();
	if (!Owner) return false;
	if (const UHandGrabComponent* Grab = Owner->FindComponentByClass<UHandGrabComponent>())
	{
		return Grab->IsGrabbed();
	}
	if (!Owner->GetRootComponent()) return false;
	for (const USceneComponent* P = Owner->GetRootComponent()->GetAttachParent(); P; P = P->GetAttachParent())
	{
		if (P->IsA<UMotionControllerComponent>()) return true;
	}
	return false;
}

void UInstrumentComponent::Play()
{
	if (!Audio || bPlaying) return;
	const float Duration = Music ? Music->GetDuration() : 0.f;
	float Start = bResume ? Elapsed : 0.f;
	if (Duration > 0.f && Duration < 10000.f)
	{
		Start = FMath::Fmod(Start, Duration);
	}
	Audio->FadeIn(FadeInTime, 1.f, Start);
	PlayStartTime = GetWorld()->GetTimeSeconds() - Start;
	bPlaying = true;
}

void UInstrumentComponent::Stop()
{
	if (!Audio || !bPlaying) return;
	Elapsed = GetWorld()->GetTimeSeconds() - PlayStartTime;
	Audio->FadeOut(FadeOutTime, 0.f);
	bPlaying = false;
}

void UInstrumentComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const bool bHeld = IsHeld();
	if (bHeld && !bPlaying) Play();
	else if (!bHeld && bPlaying) Stop();
}
