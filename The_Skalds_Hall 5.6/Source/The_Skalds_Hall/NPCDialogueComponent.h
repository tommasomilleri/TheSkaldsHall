#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPCDialogueComponent.generated.h"

class UAudioComponent;
class USoundBase;
class USoundAttenuation;
class USkeletalMeshComponent;
class UAnimMontage;           

USTRUCT(BlueprintType)
struct FDialogueLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USoundBase> Sound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UObject> LipSyncSequence;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
	FText Subtitle;

	/** Gesto facoltativo mentre parla. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> GestureMontage;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLineStarted, const FDialogueLine&, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLineFinished);

UCLASS(Blueprintable, ClassGroup = (NPC), meta = (BlueprintSpawnableComponent))
class THE_SKALDS_HALL_API UNPCDialogueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPCDialogueComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TMap<FName, FDialogueLine> Lines;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName HeadSocket = TEXT("head");

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TObjectPtr<USoundAttenuation> Attenuation;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue") FOnLineStarted OnLineStarted;
	UPROPERTY(BlueprintAssignable, Category = "Dialogue") FOnLineFinished OnLineFinished;

	UFUNCTION(BlueprintCallable, Category = "Dialogue") bool Say(FName LineKey);
	UFUNCTION(BlueprintCallable, Category = "Dialogue") void Stop();
	UFUNCTION(BlueprintPure, Category = "Dialogue") bool IsSpeaking() const { return bSpeaking; }
	UFUNCTION(BlueprintCallable, Category = "Dialogue") void SayGreeting() { Say(GreetingKey); }

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName GreetingKey = TEXT("Saluto");

	/** Calcola e interpola le curve bocca MetaHuman a partire dall'array di 15 visemi di OVR Lip Sync */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|OVRLipSync")
	static void CalculateMetaHumanLipCurves(
		const TArray<float>& Visemes,
		float DeltaTime,
		UPARAM(ref) float& JawOpen,
		UPARAM(ref) float& MouthClose,
		UPARAM(ref) float& MouthFunnel,
		UPARAM(ref) float& MouthPucker,
		UPARAM(ref) float& MouthSmileL,
		UPARAM(ref) float& MouthSmileR,
		UPARAM(ref) float& MouthLowerDownL,
		UPARAM(ref) float& MouthLowerDownR,
		UPARAM(ref) float& MouthUpperUpL,
		UPARAM(ref) float& MouthUpperUpR,
		float InterpSpeed = 20.f);
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void BP_StartLipSync(UAudioComponent* Audio, USoundBase* Sound, UObject* LipSyncSequence);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void BP_StopLipSync();

private:
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> VoiceAudio;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> BodyMesh;
	bool bSpeaking = false;

	UFUNCTION() void HandleAudioFinished();
	void InitVoice(); 
};