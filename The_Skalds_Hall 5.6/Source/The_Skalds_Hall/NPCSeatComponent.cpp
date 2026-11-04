#include "NPCSeatComponent.h"
#include "NPCRoutineComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UNPCSeatComponent::UNPCSeatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNPCSeatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!bSitOnBeginPlay) return;

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(SitTimer, this, &UNPCSeatComponent::SitDown,
			FMath::Max(SitDelay, 0.05f), false);
	}
}

void UNPCSeatComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(SitTimer);
	}
	Super::EndPlay(Reason);
}

void UNPCSeatComponent::SitDown()
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char)
	{
		UE_LOG(LogTemp, Warning, TEXT("[NPCSeat] %s: l'owner deve essere un Character."), *GetNameSafe(GetOwner()));
		return;
	}
	if (bIsSeated) return;

	if (AAIController* AI = Cast<AAIController>(Char->GetController()))
	{
		AI->StopMovement();
		if (bStopBehaviorTree)
		{
			if (UBrainComponent* Brain = AI->GetBrainComponent())
			{
				Brain->StopLogic(TEXT("NPCSeat"));
			}
		}
	}

	if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = Char->GetCapsuleComponent())
	{
		PreviousCapsuleCollision = Capsule->GetCollisionEnabled();
		if (bQueryOnlyCapsuleWhileSeated)
		{
			Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}

	ApplySeatTransform(*Char);

	if (USkeletalMeshComponent* Mesh = Char->GetMesh())
	{
		PreviousAnimMode = Mesh->GetAnimationMode();
		if (SeatedIdleAnimation)
		{
			Mesh->PlayAnimation(SeatedIdleAnimation, true);
		}
	}

	bIsSeated = true;
	SyncRoutine(true);
	OnSeatStateChanged.Broadcast(true);
}

void UNPCSeatComponent::StandUp()
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char || !bIsSeated) return;

	if (USkeletalMeshComponent* Mesh = Char->GetMesh())
	{
		if (SeatedIdleAnimation)
		{
			Mesh->Stop();
			Mesh->SetAnimationMode(PreviousAnimMode);
		}
	}

	if (UCapsuleComponent* Capsule = Char->GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(PreviousCapsuleCollision);
	}

	if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}

	if (AAIController* AI = Cast<AAIController>(Char->GetController()))
	{
		if (bStopBehaviorTree)
		{
			if (UBrainComponent* Brain = AI->GetBrainComponent())
			{
				Brain->RestartLogic();
			}
		}
	}

	bIsSeated = false;
	SyncRoutine(false);
	OnSeatStateChanged.Broadcast(false);
}

void UNPCSeatComponent::ApplySeatTransform(ACharacter& Char) const
{
	if (!SeatActor) return;
	const FTransform SeatT = SeatActor->GetActorTransform();
	FVector Location = SeatT.TransformPosition(SeatOffset);

	if (const UCapsuleComponent* Capsule = Char.GetCapsuleComponent())
	{
		Location.Z += Capsule->GetScaledCapsuleHalfHeight();
	}

	const FRotator Rotation(0.f, SeatT.Rotator().Yaw + SeatYawOffset, 0.f);
	Char.SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void UNPCSeatComponent::SyncRoutine(bool bSeated) const
{
	if (UNPCRoutineComponent* Routine = GetOwner() ? GetOwner()->FindComponentByClass<UNPCRoutineComponent>() : nullptr)
	{
		if (bSeated)
		{
			Routine->StopRoutine();
		}
		Routine->bIsSeated = bSeated;
		Routine->OnSeatedChanged.Broadcast(bSeated);
	}
}