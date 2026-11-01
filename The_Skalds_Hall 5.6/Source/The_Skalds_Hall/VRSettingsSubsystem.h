#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "Engine/TimerHandle.h"
#include "VRSettingsSubsystem.generated.h"

/** Dati persistenti delle impostazioni VR. */
UCLASS()
class THE_SKALDS_HALL_API UVRSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	float MasterVolume = 1.f;

	UPROPERTY()
	bool bSeatedMode = false;

	UPROPERTY()
	float HeightOffset = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVRSettingsChanged);

/**
 * Backend centralizzato delle impostazioni dell'esperienza VR.
 */
UCLASS()
class THE_SKALDS_HALL_API UVRSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "VR Settings")
	FOnVRSettingsChanged OnSettingsChanged;

	UFUNCTION(BlueprintCallable, Category = "VR Settings")
	void SetMasterVolume(float NewVolume);

	UFUNCTION(BlueprintPure, Category = "VR Settings")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintCallable, Category = "VR Settings")
	void RecenterView();

	UFUNCTION(BlueprintCallable, Category = "VR Settings")
	void SetSeatedMode(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "VR Settings")
	bool IsSeatedMode() const { return bSeatedMode; }

	UFUNCTION(BlueprintCallable, Category = "VR Settings")
	void SetHeightOffset(float NewOffset);

	UFUNCTION(BlueprintPure, Category = "VR Settings")
	float GetHeightOffset() const { return HeightOffset; }

	UFUNCTION(BlueprintPure, Category = "VR Settings")
	float GetTotalCameraZOffset() const;

	UFUNCTION(BlueprintCallable, Category = "VR Settings")
	void SaveSettings();

private:
	float MasterVolume = 1.f;
	bool bSeatedMode = false;
	float HeightOffset = 0.f;
	FTimerHandle SaveDebounceTimer;

	static constexpr float SeatedModeZBoost = 45.f;

	void LoadSettings();
	void ApplyVolume() const;
	void NotifyChanged();
};