#include "VRMenuBootstrapSubsystem.h"
#include "VisionMenuActor.h"
#include "MainMenuGameMode.h"
#include "Engine/World.h"
#include "EngineUtils.h"
bool UVRMenuBootstrapSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
void UVRMenuBootstrapSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (InWorld.GetNetMode() == NM_DedicatedServer) return;
	if (Cast<AMainMenuGameMode>(InWorld.GetAuthGameMode())) return;

	TActorIterator<AVisionMenuActor> Existing(&InWorld);
	if (Existing) return;

	InWorld.SpawnActor<AVisionMenuActor>(AVisionMenuActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
}