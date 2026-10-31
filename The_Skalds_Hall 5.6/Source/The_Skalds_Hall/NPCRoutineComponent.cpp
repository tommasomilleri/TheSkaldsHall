#include "NPCRoutineComponent.h"
#include "NPCDialogueComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

UNPCRoutineComponent::UNPCRoutineComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNPCRoutineComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Owner = GetOwner())
	{
		Dialogue = Owner->FindComponentByClass<UNPCDialogueComponent>();
		StartTransform = Owner->GetActorTransform();
	}
	if (bAutoStart) StartRoutine();
}

void UNPCRoutineComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	StopRoutine();
	Super::EndPlay(Reason);
}

UAnimInstance* UNPCRoutineComponent::GetAnim() const
{
	const ACharacter* Char = Cast<ACharacter>(GetOwner());
	return (Char && Char->GetMesh()) ? Char->GetMesh()->GetAnimInstance() : nullptr;
}

void UNPCRoutineComponent::StartRoutine()
{
	if (bRunning || Steps.Num() == 0) return;
	bRunning = true;
	StepIndex = -1;
	NextStep();
}

void UNPCRoutineComponent::StopRoutine()
{
	bRunning = false;
	StopCurrentMovement();
	if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(StepTimer);
}

void UNPCRoutineComponent::SetPaused(bool bPause)
{
	if (bPause && ((Dialogue && Dialogue->IsSpeaking()) || bGreeting)) return;
	bPaused = bPause;
	UWorld* W = GetWorld();
	if (!W) return;
	if (bPaused) W->GetTimerManager().PauseTimer(StepTimer);
	else         W->GetTimerManager().UnPauseTimer(StepTimer);
}

void UNPCRoutineComponent::StopCurrentMovement()
{
	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
		if (AAIController* AI = Cast<AAIController>(Char->GetController()))
		{
			AI->ReceiveMoveCompleted.RemoveDynamic(this, &UNPCRoutineComponent::OnMoveFinished);
			AI->StopMovement();
		}
}

void UNPCRoutineComponent::NextStep()
{
	if (!bRunning || bGreeting) return;
	StepIndex++;
	if (StepIndex >= Steps.Num())
	{
		if (!bLoop) { bRunning = false; return; }
		StepIndex = 0;
	}
	RunStep();
}

void UNPCRoutineComponent::ScheduleNext(const FRoutineStep& S)
{
	ScheduleNextDelay(S.HoldSeconds + FMath::FRandRange(-S.HoldRandom, S.HoldRandom));
}

void UNPCRoutineComponent::ScheduleNextDelay(float Delay)
{
	if (UWorld* W = GetWorld())
		W->GetTimerManager().SetTimer(StepTimer, this, &UNPCRoutineComponent::NextStep,
			FMath::Max(Delay, 0.05f), false);
}

void UNPCRoutineComponent::RunStep()
{
	const FRoutineStep& S = Steps[StepIndex];
	AActor* Owner = GetOwner();
	ACharacter* Char = Cast<ACharacter>(Owner);

	switch (S.Type)
	{
	case ERoutineStepType::PlayMontage:
	{
		UAnimInstance* Anim = GetAnim();
		if (!Anim || !S.Montage) { ScheduleNext(S); return; }
		Anim->Montage_Play(S.Montage, 1.f);
		FOnMontageEnded Ended;
		Ended.BindUObject(this, &UNPCRoutineComponent::OnMontageEnded);
		Anim->Montage_SetEndDelegate(Ended, S.Montage);
		break;
	}
	case ERoutineStepType::MoveTo:
	{
		AAIController* AI = Char ? Cast<AAIController>(Char->GetController()) : nullptr;
		if (!AI)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Routine] %s: serve un AIController (Auto Possess AI = Placed in World)."), *GetNameSafe(Owner));
			ScheduleNext(S); return;
		}
		const FVector WorldTarget = StartTransform.TransformPosition(S.TargetPoint);
		FAIMoveRequest Req(WorldTarget);
		Req.SetAcceptanceRadius(12.f);
		Req.SetUsePathfinding(true);
		Req.SetAllowPartialPath(true);
		const FPathFollowingRequestResult R = AI->MoveTo(Req);
		if (R.Code == EPathFollowingRequestResult::RequestSuccessful)
			AI->ReceiveMoveCompleted.AddUniqueDynamic(this, &UNPCRoutineComponent::OnMoveFinished);
		else
			ScheduleNext(S);
		break;
	}
	case ERoutineStepType::Say:
	{
		if (!Dialogue || !Dialogue->Say(S.DialogueKey)) { ScheduleNext(S); return; }
		Dialogue->OnLineFinished.AddUniqueDynamic(this, &UNPCRoutineComponent::OnDialogueFinished);
		break;
	}
	case ERoutineStepType::SetSeated:
	{
		bIsSeated = S.bSeated;
		OnSeatedChanged.Broadcast(bIsSeated);
		if (Char && Char->GetCharacterMovement())
			Char->GetCharacterMovement()->SetMovementMode(bIsSeated ? MOVE_None : MOVE_Walking);
		ScheduleNext(S);
		break;
	}
	case ERoutineStepType::FaceDirection:
	{
		if (Owner)
		{
			FRotator R = Owner->GetActorRotation(); R.Yaw = S.Yaw;
			Owner->SetActorRotation(R);
		}
		ScheduleNext(S);
		break;
	}
	case ERoutineStepType::Wait:
	default:
		ScheduleNext(S);
		break;
	}
}

void UNPCRoutineComponent::OnMontageEnded(UAnimMontage*, bool)
{
	if (!bRunning || bGreeting) return;
	ScheduleNext(Steps[StepIndex]);
}

void UNPCRoutineComponent::OnDialogueFinished()
{
	if (Dialogue) Dialogue->OnLineFinished.RemoveDynamic(this, &UNPCRoutineComponent::OnDialogueFinished);
	if (!bRunning || bGreeting) return;
	ScheduleNext(Steps[StepIndex]);
}

void UNPCRoutineComponent::OnMoveFinished(FAIRequestID, EPathFollowingResult::Type)
{
	StopCurrentMovement();
	if (!bRunning || bGreeting) return;
	ScheduleNext(Steps[StepIndex]);
}

void UNPCRoutineComponent::InterruptForGreeting()
{
	if (bGreeting) return;
	bGreeting = true;
	StopCurrentMovement();
	if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(StepTimer);

	if (!bIsSeated)
		if (AActor* Owner = GetOwner())
			if (const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
			{
				FVector Dir = Cam->GetCameraLocation() - Owner->GetActorLocation();
				Dir.Z = 0.f;
				if (!Dir.IsNearlyZero())
				{
					FRotator R = Owner->GetActorRotation();
					R.Yaw = Dir.Rotation().Yaw;
					Owner->SetActorRotation(R);
				}
			}

	UAnimInstance* Anim = GetAnim();
	if (Anim && GreetMontage)
	{
		Anim->Montage_Play(GreetMontage, 1.f);
		FOnMontageEnded Ended;
		Ended.BindUObject(this, &UNPCRoutineComponent::OnGreetMontageEnded);
		Anim->Montage_SetEndDelegate(Ended, GreetMontage);
	}
	else
	{
		OnGreetMontageEnded(nullptr, false);
	}
}

void UNPCRoutineComponent::OnGreetMontageEnded(UAnimMontage*, bool)
{
	bGreeting = false;
	if (!bRunning) return;
	if (bResumeAfterGreeting) RunStep();      // riprende lo step corrente
	else                      ScheduleNextDelay(0.5f);
}