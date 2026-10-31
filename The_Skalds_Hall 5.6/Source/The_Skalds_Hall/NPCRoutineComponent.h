#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "NPCRoutineComponent.generated.h"

class UAnimMontage;
class UNPCDialogueComponent;

UENUM(BlueprintType)
enum class ERoutineStepType : uint8
{
	PlayMontage   UMETA(DisplayName = "Riproduci Montage"),
	MoveTo        UMETA(DisplayName = "Cammina verso punto"),
	Say           UMETA(DisplayName = "Parla (NPCDialogue)"),
	Wait          UMETA(DisplayName = "Attendi"),
	SetSeated     UMETA(DisplayName = "Imposta seduto/in piedi"),
	FaceDirection UMETA(DisplayName = "Girati (yaw assoluto)")
};

USTRUCT(BlueprintType)
struct FRoutineStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERoutineStepType Type = ERoutineStepType::Wait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type == ERoutineStepType::PlayMontage", EditConditionHides))
	TObjectPtr<UAnimMontage> Montage;

	/** Punto in spazio LOCALE della posizione iniziale dell'actor (widget 3D nel livello). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type == ERoutineStepType::MoveTo", EditConditionHides, MakeEditWidget = true))
	FVector TargetPoint = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type == ERoutineStepType::Say", EditConditionHides))
	FName DialogueKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type == ERoutineStepType::SetSeated", EditConditionHides))
	bool bSeated = false;

	/** Yaw assoluto in gradi (0 = +X mondo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type == ERoutineStepType::FaceDirection", EditConditionHides))
	float Yaw = 0.f;

	/** Attesa dopo lo step (secondi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float HoldSeconds = 0.f;

	/** Variazione casuale +/- su HoldSeconds (realismo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float HoldRandom = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSeatedChanged, bool, bSeated);

/**
 * Routine ciclica di step. Pause casuali, interruzione per salutare il player
 * (collega NPCGazeGreeter.OnGreet -> InterruptForGreeting), pausa esterna via
 * SetPaused (rifiutata mentre parla o saluta). Tutto a eventi/timer: 0 tick.
 */
UCLASS(ClassGroup = (NPC), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UNPCRoutineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPCRoutineComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Routine")
	TArray<FRoutineStep> Steps;

	UPROPERTY(EditAnywhere, Category = "Routine")
	bool bLoop = true;

	UPROPERTY(EditAnywhere, Category = "Routine")
	bool bAutoStart = true;

	/** Montage del saluto (es. MH_Waving_Montage). */
	UPROPERTY(EditAnywhere, Category = "Routine|Greeting")
	TObjectPtr<UAnimMontage> GreetMontage;

	UPROPERTY(EditAnywhere, Category = "Routine|Greeting")
	bool bResumeAfterGreeting = true;

	/** Letto dall'AnimBP per lo stato Seated. */
	UPROPERTY(BlueprintReadOnly, Category = "Routine")
	bool bIsSeated = false;

	UPROPERTY(BlueprintAssignable, Category = "Routine")
	FOnSeatedChanged OnSeatedChanged;

	UFUNCTION(BlueprintCallable, Category = "Routine") void StartRoutine();
	UFUNCTION(BlueprintCallable, Category = "Routine") void StopRoutine();
	UFUNCTION(BlueprintCallable, Category = "Routine") void SetPaused(bool bPause);
	UFUNCTION(BlueprintPure, Category = "Routine") int32 GetCurrentStep() const { return StepIndex; }
	UFUNCTION(BlueprintPure, Category = "Routine") bool IsRunning() const { return bRunning; }

	/** Ferma lo step, si gira verso il player (se in piedi), saluta, riprende. */
	UFUNCTION(BlueprintCallable, Category = "Routine|Greeting")
	void InterruptForGreeting();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	int32 StepIndex = -1;
	bool bRunning = false;
	bool bPaused = false;
	bool bGreeting = false;
	FTimerHandle StepTimer;
	FTransform StartTransform; // i TargetPoint sono relativi a QUESTA

	UPROPERTY(Transient) TObjectPtr<UNPCDialogueComponent> Dialogue;

	void RunStep();
	void NextStep();
	void ScheduleNext(const FRoutineStep& S);
	void ScheduleNextDelay(float Delay);
	void StopCurrentMovement();
	class UAnimInstance* GetAnim() const;

	UFUNCTION() void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION() void OnGreetMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION() void OnDialogueFinished();
	UFUNCTION() void OnMoveFinished(FAIRequestID RequestID, EPathFollowingResult::Type Result);
};