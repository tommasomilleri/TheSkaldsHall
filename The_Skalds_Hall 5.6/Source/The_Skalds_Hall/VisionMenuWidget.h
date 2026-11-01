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
class UVRSettingsSubsystem;

/**
 * Menu impostazioni VR in stile visionOS.
 */
UCLASS()
class THE_SKALDS_HALL_API UVisionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	TObjectPtr<USlider> VolumeSlider = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> VolumeValueText = nullptr;

	UPROPERTY()
	TObjectPtr<USlider> HeightSlider = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> HeightValueText = nullptr;

	UPROPERTY()
	TObjectPtr<UCheckBox> SeatedCheck = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> RecenterButton = nullptr;

	UFUNCTION() void HandleVolumeChanged(float Value);
	UFUNCTION() void HandleHeightChanged(float Value);
	UFUNCTION() void HandleSeatedChanged(bool bChecked);
	UFUNCTION() void HandleRecenterClicked();

	UVRSettingsSubsystem* GetSettings() const;

	UBorder* MakeGlassPanel();
	UWidget* MakeRow(const FString& Label, UWidget* Control);
	UWidget* MakeCard(const FString& Label, UWidget* Control, UTextBlock* ValueText);
	USlider* MakeVisionSlider(float MinVal, float MaxVal);
	UTextBlock* MakeText(const FString& Text, float Size, FLinearColor Color);
};