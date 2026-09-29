
#include "VRSettingsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Misc/App.h"

static const FString SettingsSlot = TEXT("VRSettings");

void UVRSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadSettings();
	ApplyVolume();
}

void UVRSettingsSubsystem::SetMasterVolume(float NewVolume)
{
	MasterVolume = FMath::Clamp(NewVolume, 0.f, 1.f);
	ApplyVolume();
	NotifyChanged();
}

void UVRSettingsSubsystem::RecenterView()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void UVRSettingsSubsystem::SetSeatedMode(bool bEnabled)
{
	bSeatedMode = bEnabled;
	NotifyChanged();
}

void UVRSettingsSubsystem::SetHeightOffset(float NewOffset)
{
	HeightOffset = FMath::Clamp(NewOffset, -30.f, 30.f);
	NotifyChanged();
}

float UVRSettingsSubsystem::GetTotalCameraZOffset() const
{
	return HeightOffset + (bSeatedMode ? SeatedModeZBoost : 0.f);
}

void UVRSettingsSubsystem::SaveSettings()
{
	UVRSettingsSaveGame* Save = Cast<UVRSettingsSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UVRSettingsSaveGame::StaticClass()));
	if (Save)
	{
		Save->MasterVolume = MasterVolume;
		Save->bSeatedMode = bSeatedMode;
		Save->HeightOffset = HeightOffset;
		UGameplayStatics::SaveGameToSlot(Save, SettingsSlot, 0);
	}
}

void UVRSettingsSubsystem::LoadSettings()
{
	if (UGameplayStatics::DoesSaveGameExist(SettingsSlot, 0))
	{
		UVRSettingsSaveGame* Save = Cast<UVRSettingsSaveGame>(
			UGameplayStatics::LoadGameFromSlot(SettingsSlot, 0));
		if (Save)
		{
			MasterVolume = Save->MasterVolume;
			bSeatedMode = Save->bSeatedMode;
			HeightOffset = Save->HeightOffset;
		}
	}
}

void UVRSettingsSubsystem::ApplyVolume() const
{
	FApp::SetVolumeMultiplier(MasterVolume);
}

void UVRSettingsSubsystem::NotifyChanged()
{
	OnSettingsChanged.Broadcast();
	const_cast<UVRSettingsSubsystem*>(this)->SaveSettings();
}

