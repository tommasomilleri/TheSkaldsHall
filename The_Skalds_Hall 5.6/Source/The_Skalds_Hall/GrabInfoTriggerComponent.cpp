#include "GrabInfoTriggerComponent.h"
#include "GlassCardActor.h"
#include "MotionControllerComponent.h"

UGrabInfoTriggerComponent::UGrabInfoTriggerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

bool UGrabInfoTriggerComponent::IsHeldNow() const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->GetRootComponent()) return false;

	for (USceneComponent* P = Owner->GetRootComponent()->GetAttachParent();
		P; P = P->GetAttachParent())
	{
		if (P->IsA<UMotionControllerComponent>())
		{
			return true;
		}
	}
	return false;
}

void UGrabInfoTriggerComponent::TickComponent(float DeltaTime,
	ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const bool bHeld = IsHeldNow();
	if (bHeld != bWasHeld && LinkedCard)
	{
		if (bHeld)
		{
			LinkedCard->Open();
		}
		else if (bCloseOnRelease)
		{
			LinkedCard->Close();
		}
	}
	bWasHeld = bHeld;
}