#include "GrabInfoTriggerComponent.h"
#include "GlassCardActor.h"
#include "MotionControllerComponent.h"

UGrabInfoTriggerComponent::UGrabInfoTriggerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Controllare 10 volte al secondo basta e non pesa
	PrimaryComponentTick.TickInterval = 0.1f;
}

bool UGrabInfoTriggerComponent::IsHeldNow() const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->GetRootComponent()) return false;

	// Stessa tecnica della scheda: risali la catena di attach
	for (USceneComponent* P = Owner->GetRootComponent()->GetAttachParent();
		P; P = P->GetAttachParent())
	{
		if (P->IsA<UMotionControllerComponent>()) return true;
	}
	return false;
}

void UGrabInfoTriggerComponent::TickComponent(float DeltaTime,
	ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const bool bHeld = IsHeldNow();

	// Reagiamo solo al CAMBIO di stato (preso / lasciato)
	if (bHeld != bWasHeld && LinkedCard)
	{
		if (bHeld) { LinkedCard->Open(); }
		else if (bCloseOnRelease) { LinkedCard->Close(); }
	}
	bWasHeld = bHeld;
}