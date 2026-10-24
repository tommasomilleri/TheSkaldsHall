// ----- FILE: Source/The_Skalds_Hall/NPCDialogueComponent.cpp -----
#include "NPCDialogueComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"

UNPCDialogueComponent::UNPCDialogueComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // event-driven: zero costo a riposo
}

void UNPCDialogueComponent::BeginPlay()
{
	Super::BeginPlay();
	// Il MetaHuman assembla Body/Face DOPO il BeginPlay dei componenti:
	// rimandiamo l'init al prossimo tick (fix warning "no attach parent" in packaged).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UNPCDialogueComponent::InitVoice);
	}
}

void UNPCDialogueComponent::InitVoice()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<USkeletalMeshComponent*> Meshes;
	Owner->GetComponents<USkeletalMeshComponent>(Meshes);
	for (USkeletalMeshComponent* M : Meshes)
	{
		if (M && M->DoesSocketExist(HeadSocket)) { BodyMesh = M; break; }
	}
	if (!BodyMesh && Meshes.Num() > 0) BodyMesh = Meshes[0];

	VoiceAudio = NewObject<UAudioComponent>(Owner, TEXT("VoiceAudio"));
	VoiceAudio->bAutoActivate = false;
	VoiceAudio->bAllowSpatialization = true;
	VoiceAudio->AttenuationSettings = Attenuation;
	// A runtime: PRIMA Register, POI Attach (SetupAttachment è da costruttore)
	VoiceAudio->RegisterComponent();
	VoiceAudio->AttachToComponent(
		BodyMesh ? static_cast<USceneComponent*>(BodyMesh) : Owner->GetRootComponent(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		BodyMesh ? HeadSocket : NAME_None);
	VoiceAudio->OnAudioFinished.AddDynamic(this, &UNPCDialogueComponent::HandleAudioFinished);

	if (!BodyMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialogue] %s: nessuna SkeletalMesh con socket '%s'."),
			*GetNameSafe(Owner), *HeadSocket.ToString());
	}
}

void UNPCDialogueComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Stop();
	Super::EndPlay(Reason);
}

bool UNPCDialogueComponent::Say(FName LineKey)
{
	if (bSpeaking || !VoiceAudio) return false;

	const FDialogueLine* Line = Lines.Find(LineKey);
	if (!Line || !Line->Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialogue] %s: battuta '%s' mancante."),
			*GetNameSafe(GetOwner()), *LineKey.ToString());
		return false;
	}

	bSpeaking = true;
	VoiceAudio->SetSound(Line->Sound);
	VoiceAudio->Play();

	if (Line->GestureMontage && BodyMesh)
		if (UAnimInstance* Anim = BodyMesh->GetAnimInstance())
			Anim->Montage_Play(Line->GestureMontage, 1.f);

	BP_StartLipSync(VoiceAudio, Line->Sound, Line->LipSyncSequence);
	OnLineStarted.Broadcast(*Line);
	return true;
}

void UNPCDialogueComponent::Stop()
{
	if (!bSpeaking) return;
	bSpeaking = false;
	if (VoiceAudio) VoiceAudio->Stop();
	BP_StopLipSync();
	OnLineFinished.Broadcast();
}

void UNPCDialogueComponent::HandleAudioFinished()
{
	if (!bSpeaking) return; // Stop() ha già fatto il broadcast
	bSpeaking = false;
	BP_StopLipSync();
	OnLineFinished.Broadcast();
}
void UNPCDialogueComponent::CalculateMetaHumanLipCurves(
	const TArray<float>& V,
	float DeltaTime,
	float& JawOpen,
	float& MouthClose,
	float& MouthFunnel,
	float& MouthPucker,
	float& MouthSmileL,
	float& MouthSmileR,
	float& MouthLowerDownL,
	float& MouthLowerDownR,
	float& MouthUpperUpL,
	float& MouthUpperUpR,
	float InterpSpeed)
{
	// Se l'array non ha i 15 visemi di OVR (o non sta parlando), riporta dolcemente la bocca a 0
	if (V.Num() < 15)
	{
		JawOpen = FMath::FInterpTo(JawOpen, 0.f, DeltaTime, InterpSpeed);
		MouthClose = FMath::FInterpTo(MouthClose, 0.f, DeltaTime, InterpSpeed);
		MouthFunnel = FMath::FInterpTo(MouthFunnel, 0.f, DeltaTime, InterpSpeed);
		MouthPucker = FMath::FInterpTo(MouthPucker, 0.f, DeltaTime, InterpSpeed);
		MouthSmileL = FMath::FInterpTo(MouthSmileL, 0.f, DeltaTime, InterpSpeed);
		MouthSmileR = FMath::FInterpTo(MouthSmileR, 0.f, DeltaTime, InterpSpeed);
		MouthLowerDownL = FMath::FInterpTo(MouthLowerDownL, 0.f, DeltaTime, InterpSpeed);
		MouthLowerDownR = FMath::FInterpTo(MouthLowerDownR, 0.f, DeltaTime, InterpSpeed);
		MouthUpperUpL = FMath::FInterpTo(MouthUpperUpL, 0.f, DeltaTime, InterpSpeed);
		MouthUpperUpR = FMath::FInterpTo(MouthUpperUpR, 0.f, DeltaTime, InterpSpeed);
		return;
	}

	// Indici OVR: 1=PP, 2=FF, 3=TH, 4=DD, 5=kk, 6=CH, 7=SS, 8=nn, 9=RR, 10=aa, 11=E, 12=ih, 13=oh, 14=ou
	const float TargetJawOpen = FMath::Clamp(V[10] * 0.7f + V[13] * 0.5f + V[11] * 0.4f + V[5] * 0.3f + V[12] * 0.25f + V[14] * 0.2f + V[4] * 0.2f, 0.f, 1.f);
	const float TargetMouthClose = FMath::Clamp(V[1] * 1.0f, 0.f, 1.f);
	const float TargetMouthFunnel = FMath::Clamp(V[14] * 0.5f + V[6] * 0.6f + V[13] * 0.7f + V[2] * 0.4f, 0.f, 1.f);
	const float TargetMouthPucker = FMath::Clamp(V[14] * 0.9f + V[9] * 0.4f, 0.f, 1.f);
	const float TargetMouthSmile = FMath::Clamp(V[12] * 0.5f + V[11] * 0.3f + V[7] * 0.2f, 0.f, 1.f);
	const float TargetLowerDown = FMath::Clamp(V[10] * 0.4f + V[11] * 0.3f + V[2] * 0.3f + V[3] * 0.3f, 0.f, 1.f);
	const float TargetUpperUp = FMath::Clamp(V[6] * 0.3f + V[7] * 0.25f + V[10] * 0.2f, 0.f, 1.f);

	JawOpen = FMath::FInterpTo(JawOpen, TargetJawOpen, DeltaTime, InterpSpeed);
	MouthClose = FMath::FInterpTo(MouthClose, TargetMouthClose, DeltaTime, InterpSpeed);
	MouthFunnel = FMath::FInterpTo(MouthFunnel, TargetMouthFunnel, DeltaTime, InterpSpeed);
	MouthPucker = FMath::FInterpTo(MouthPucker, TargetMouthPucker, DeltaTime, InterpSpeed);
	MouthSmileL = FMath::FInterpTo(MouthSmileL, TargetMouthSmile, DeltaTime, InterpSpeed);
	MouthSmileR = FMath::FInterpTo(MouthSmileR, TargetMouthSmile, DeltaTime, InterpSpeed);
	MouthLowerDownL = FMath::FInterpTo(MouthLowerDownL, TargetLowerDown, DeltaTime, InterpSpeed);
	MouthLowerDownR = FMath::FInterpTo(MouthLowerDownR, TargetLowerDown, DeltaTime, InterpSpeed);
	MouthUpperUpL = FMath::FInterpTo(MouthUpperUpL, TargetUpperUp, DeltaTime, InterpSpeed);
	MouthUpperUpR = FMath::FInterpTo(MouthUpperUpR, TargetUpperUp, DeltaTime, InterpSpeed);
}