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

	BP_StartLipSync(VoiceAudio, Line->Sound);
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