#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "NPCSeatComponent.generated.h"

class UAnimSequenceBase;
class ACharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNPCSeatStateChanged, bool, bSeated);

/**
 * Fa sedere un NPC su una panca/sedia e gli impedisce di vagare.
 */
UCLASS(ClassGroup = (NPC), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UNPCSeatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPCSeatComponent();

	/** Panca/sedia su cui sedersi (actor nel livello). Se vuoto l'NPC resta dov'e' piazzato. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Seat")
	TObjectPtr<AActor> SeatActor;

	/** Offset in spazio LOCALE del SeatActor (X avanti, Y destra, Z su), in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seat")
	FVector SeatOffset = FVector::ZeroVector;

	/** Yaw aggiuntivo (gradi) rispetto alla rotazione del SeatActor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seat")
	float SeatYawOffset = 0.f;

	/** Animazione idle da seduto, riprodotta in loop sulla mesh principale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seat")
	TObjectPtr<UAnimSequenceBase> SeatedIdleAnimation;

	/** Si siede automaticamente all'avvio del livello. */
	UPROPERTY(EditAnywhere, Category = "Seat")
	bool bSitOnBeginPlay = true;

	/** Ferma il Behavior Tree dell'AIController (random roam). */
	UPROPERTY(EditAnywhere, Category = "Seat")
	bool bStopBehaviorTree = true;

	/** Capsula in sola query mentre e' seduto (niente spinte fisiche dalla panca). */
	UPROPERTY(EditAnywhere, Category = "Seat")
	bool bQueryOnlyCapsuleWhileSeated = true;

	/** Ritardo prima di sedersi: lascia il tempo all'AIController di possedere il pawn e avviare il BT. */
	UPROPERTY(EditAnywhere, Category = "Seat", meta = (ClampMin = "0.05"))
	float SitDelay = 0.25f;

	UPROPERTY(BlueprintReadOnly, Category = "Seat")
	bool bIsSeated = false;

	UPROPERTY(BlueprintAssignable, Category = "Seat")
	FOnNPCSeatStateChanged OnSeatStateChanged;

	UFUNCTION(BlueprintCallable, Category = "Seat")
	void SitDown();

	UFUNCTION(BlueprintCallable, Category = "Seat")
	void StandUp();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	FTimerHandle SitTimer;
	TEnumAsByte<EAnimationMode::Type> PreviousAnimMode = EAnimationMode::AnimationBlueprint;
	TEnumAsByte<ECollisionEnabled::Type> PreviousCapsuleCollision = ECollisionEnabled::QueryAndPhysics;

	void ApplySeatTransform(ACharacter& Char) const;
	void SyncRoutine(bool bSeated) const;
};