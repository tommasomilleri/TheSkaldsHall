
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
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
 * Il menu (UI) chiama queste funzioni; il VRPawn ascolta OnSettingsChanged
 * per applicare l'offset di altezza / modalita' seduti.
 */
UCLASS()
class THE_SKALDS_HALL_API UVRSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Notifica UI/Pawn quando un valore cambia. */
	UPROPERTY(BlueprintAssignable, Category = "VR Settings")
	FOnVRSettingsChanged OnSettingsChanged;

	// ---- Volume ----
	UFUNCTION(BlueprintCallable, Category = "VR Settings")
	void SetMasterVolume(float NewVolume);

	UFUNCTION(BlueprintPure, Category = "VR Settings")
	float GetMasterVolume() const { return MasterVolume; }

	// ---- Vista ----
	UFUNCTION(BlueprintCallable, Category = "VR Settings")
	void RecenterView();

	// ---- Modalita' seduti ----
	UFUNCTION(BlueprintCallable, Category = "VR Settings")
	void SetSeatedMode(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "VR Settings")
	bool IsSeatedMode() const { return bSeatedMode; }

	// ---- Altezza fine ----
	UFUNCTION(BlueprintCallable, Category = "VR Settings")
	void SetHeightOffset(float NewOffset);

	UFUNCTION(BlueprintPure, Category = "VR Settings")
	float GetHeightOffset() const { return HeightOffset; }

	/** Offset Z totale da applicare alla Camera Origin del pawn. */
	UFUNCTION(BlueprintPure, Category = "VR Settings")
	float GetTotalCameraZOffset() const;

	// ---- Persistenza ----
	UFUNCTION(BlueprintCallable, Category = "VR Settings")
	void SaveSettings();

private:
	float MasterVolume = 1.f;
	bool bSeatedMode = false;
	float HeightOffset = 0.f;

	/** Alza la camera di 45 cm quando si e' seduti. */
	static constexpr float SeatedModeZBoost = 45.f;

	void LoadSettings();
	void ApplyVolume() const;
	void NotifyChanged();
};

