
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VisionMenuWidget.generated.h"

class UVerticalBox;
class USlider;
class UCheckBox;
class UButton;
class UTextBlock;
class UBorder;

/**
 * Menu impostazioni VR in stile visionOS: pannello vetro scuro,
 * angoli arrotondati, righe con slider/toggle/bottoni.
 * Interamente costruito in C++ (WidgetTree), nessun asset UMG richiesto.
 */
UCLASS()
class THE_SKALDS_HALL_API UVisionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	UPROPERTY() USlider* VolumeSlider = nullptr;
	UPROPERTY() UTextBlock* VolumeValueText = nullptr;
	UPROPERTY() USlider* HeightSlider = nullptr;
	UPROPERTY() UTextBlock* HeightValueText = nullptr;
	UPROPERTY() UCheckBox* SeatedCheck = nullptr;
	UPROPERTY() UButton* RecenterButton = nullptr;

	UFUNCTION() void HandleVolumeChanged(float Value);
	UFUNCTION() void HandleHeightChanged(float Value);
	UFUNCTION() void HandleSeatedChanged(bool bChecked);
	UFUNCTION() void HandleRecenterClicked();

	class UVRSettingsSubsystem* GetSettings() const;

	// Helper di costruzione UI
	UBorder* MakeGlassPanel();
	UWidget* MakeRow(const FString& Label, UWidget* Control);
	UWidget* MakeCard(const FString& Label, UWidget* Control, UTextBlock* ValueText);
	USlider* MakeVisionSlider(float MinVal, float MaxVal);
	UTextBlock* MakeText(const FString& Text, float Size, FLinearColor Color);
};
