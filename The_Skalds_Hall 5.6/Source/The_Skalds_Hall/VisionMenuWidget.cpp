#include "VisionMenuWidget.h"
#include "VRSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/BorderSlot.h"
#include "Components/SizeBox.h"
#include "Kismet/GameplayStatics.h"

namespace VisionStyle
{
	// Palette visionOS
	const FLinearColor PanelBg(0.01f, 0.01f, 0.015f, 0.35f);      // vetro scuro caldo
	const FLinearColor PanelEdge(1.f, 1.f, 1.f, 0.22f);           // bordo luminoso
	const FLinearColor CardBg(1.f, 1.f, 1.f, 0.05f);              // card riga
	const FLinearColor CardEdge(1.f, 1.f, 1.f, 0.10f);
	const FLinearColor TextPrimary(1.f, 1.f, 1.f, 0.96f);
	const FLinearColor TextSecondary(1.f, 1.f, 1.f, 0.55f);
	const FLinearColor SliderTrack(1.f, 1.f, 1.f, 0.22f);
	const FLinearColor Accent(1.f, 1.f, 1.f, 1.f);                 // handle/CTA bianchi
}

TSharedRef<SWidget> UVisionMenuWidget::RebuildWidget()
{
	using namespace VisionStyle;

	// ---------- Pannello vetro radice ----------
	UBorder* Root = MakeGlassPanel();
	WidgetTree->RootWidget = Root;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->SetContent(Column);
	if (UBorderSlot* BSlot = Cast<UBorderSlot>(Column->Slot))
	{
		BSlot->SetPadding(FMargin(34.f, 30.f));
	}

	// ---------- Header ----------
	UTextBlock* Title = MakeText(TEXT("Impostazioni"), 30.f, TextPrimary);
	{
		FSlateFontInfo F = Title->GetFont();
		F.TypefaceFontName = FName("Bold");
		Title->SetFont(F);
	}
	UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(Title);
	TitleSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 4.f));

	UTextBlock* Subtitle = MakeText(TEXT("Esperienza VR"), 14.f, TextSecondary);
	UVerticalBoxSlot* SubSlot = Column->AddChildToVerticalBox(Subtitle);
	SubSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 22.f));

	// ---------- Card: Volume ----------
	VolumeSlider = MakeVisionSlider(0.f, 1.f);
	VolumeValueText = MakeText(TEXT("100%"), 15.f, TextSecondary);
	Column->AddChildToVerticalBox(MakeCard(TEXT("Volume"), VolumeSlider, VolumeValueText));

	// ---------- Card: Altezza ----------
	HeightSlider = MakeVisionSlider(-30.f, 30.f);
	HeightValueText = MakeText(TEXT("0 cm"), 15.f, TextSecondary);
	Column->AddChildToVerticalBox(MakeCard(TEXT("Altezza"), HeightSlider, HeightValueText));

	// ---------- Card: Modalita' seduti ----------
	SeatedCheck = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	SeatedCheck->SetRenderScale(FVector2D(1.4f, 1.4f));
	Column->AddChildToVerticalBox(MakeCard(TEXT("Modalita' seduti"), SeatedCheck, nullptr));

	// ---------- Bottone pill: Ricentra ----------
	RecenterButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FButtonStyle Pill = RecenterButton->GetStyle();

	FSlateBrush PillNormal;
	PillNormal.DrawAs = ESlateBrushDrawType::RoundedBox;
	PillNormal.TintColor = FLinearColor(1.f, 1.f, 1.f, 0.14f);
	PillNormal.OutlineSettings.CornerRadii = FVector4(26.f, 26.f, 26.f, 26.f);
	PillNormal.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	PillNormal.OutlineSettings.Width = 1.f;
	PillNormal.OutlineSettings.Color = FLinearColor(1.f, 1.f, 1.f, 0.20f);
	Pill.SetNormal(PillNormal);

	FSlateBrush PillHover = PillNormal;
	PillHover.TintColor = FLinearColor(1.f, 1.f, 1.f, 0.28f);
	Pill.SetHovered(PillHover);

	FSlateBrush PillPressed = PillNormal;
	PillPressed.TintColor = FLinearColor(1.f, 1.f, 1.f, 0.38f);
	Pill.SetPressed(PillPressed);

	RecenterButton->SetStyle(Pill);

	UTextBlock* RecenterLabel = MakeText(TEXT("Ricentra vista"), 17.f, TextPrimary);
	{
		FSlateFontInfo F = RecenterLabel->GetFont();
		F.TypefaceFontName = FName("Bold");
		RecenterLabel->SetFont(F);
	}
	RecenterButton->AddChild(RecenterLabel);
	if (UButtonSlot* RSlot = Cast<UButtonSlot>(RecenterLabel->Slot))
	{
		RSlot->SetPadding(FMargin(34.f, 13.f));
	}

	UVerticalBoxSlot* BtnSlot = Column->AddChildToVerticalBox(RecenterButton);
	BtnSlot->SetHorizontalAlignment(HAlign_Center);
	BtnSlot->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));

	return Super::RebuildWidget();
}

void UVisionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (VolumeSlider)
	{
		VolumeSlider->OnValueChanged.AddDynamic(this, &UVisionMenuWidget::HandleVolumeChanged);
	}
	if (HeightSlider)
	{
		HeightSlider->OnValueChanged.AddDynamic(this, &UVisionMenuWidget::HandleHeightChanged);
	}
	if (SeatedCheck)
	{
		SeatedCheck->OnCheckStateChanged.AddDynamic(this, &UVisionMenuWidget::HandleSeatedChanged);
	}
	if (RecenterButton)
	{
		RecenterButton->OnClicked.AddDynamic(this, &UVisionMenuWidget::HandleRecenterClicked);
	}

	if (UVRSettingsSubsystem* S = GetSettings())
	{
		if (VolumeSlider) { VolumeSlider->SetValue(S->GetMasterVolume()); }
		if (HeightSlider) { HeightSlider->SetValue(S->GetHeightOffset()); }
		if (SeatedCheck) { SeatedCheck->SetIsChecked(S->IsSeatedMode()); }
		HandleVolumeChanged(S->GetMasterVolume());
		HandleHeightChanged(S->GetHeightOffset());
	}
}

void UVisionMenuWidget::HandleVolumeChanged(float Value)
{
	if (UVRSettingsSubsystem* S = GetSettings())
	{
		S->SetMasterVolume(Value);
	}
	if (VolumeValueText)
	{
		VolumeValueText->SetText(FText::FromString(
			FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f))));
	}
}

void UVisionMenuWidget::HandleHeightChanged(float Value)
{
	if (UVRSettingsSubsystem* S = GetSettings())
	{
		S->SetHeightOffset(Value);
	}
	if (HeightValueText)
	{
		HeightValueText->SetText(FText::FromString(
			FString::Printf(TEXT("%d cm"), FMath::RoundToInt(Value))));
	}
}

void UVisionMenuWidget::HandleSeatedChanged(bool bChecked)
{
	if (UVRSettingsSubsystem* S = GetSettings())
	{
		S->SetSeatedMode(bChecked);
	}
}

void UVisionMenuWidget::HandleRecenterClicked()
{
	if (UVRSettingsSubsystem* S = GetSettings())
	{
		S->RecenterView();
	}
}

UVRSettingsSubsystem* UVisionMenuWidget::GetSettings() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UVRSettingsSubsystem>();
	}
	return nullptr;
}

UBorder* UVisionMenuWidget::MakeGlassPanel()
{
	using namespace VisionStyle;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	FSlateBrush Glass;
	Glass.DrawAs = ESlateBrushDrawType::RoundedBox;
	Glass.TintColor = PanelBg;
	Glass.OutlineSettings.CornerRadii = FVector4(34.f, 34.f, 34.f, 34.f);
	Glass.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	Glass.OutlineSettings.Width = 1.5f;
	Glass.OutlineSettings.Color = PanelEdge;
	Panel->SetBrush(Glass);
	return Panel;
}

UWidget* UVisionMenuWidget::MakeRow(const FString& Label, UWidget* Control)
{
	// Mantenuta per compatibilita', delega a MakeCard.
	return MakeCard(Label, Control, nullptr);
}

UWidget* UVisionMenuWidget::MakeCard(const FString& Label, UWidget* Control, UTextBlock* ValueText)
{
	using namespace VisionStyle;

	// Card arrotondata (stile riga visionOS)
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	FSlateBrush CardBrush;
	CardBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	CardBrush.TintColor = CardBg;
	CardBrush.OutlineSettings.CornerRadii = FVector4(20.f, 20.f, 20.f, 20.f);
	CardBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	CardBrush.OutlineSettings.Width = 1.f;
	CardBrush.OutlineSettings.Color = CardEdge;
	Card->SetBrush(CardBrush);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Card->SetContent(Row);
	if (UBorderSlot* CSlotPad = Cast<UBorderSlot>(Row->Slot))
	{
		CSlotPad->SetPadding(FMargin(20.f, 16.f));
	}

	// Etichetta
	UTextBlock* LabelText = MakeText(Label, 17.f, TextPrimary);
	UHorizontalBoxSlot* LSlot = Row->AddChildToHorizontalBox(LabelText);
	LSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LSlot->SetVerticalAlignment(VAlign_Center);

	// Valore (opzionale, a destra prima del controllo)
	if (ValueText)
	{
		UHorizontalBoxSlot* VSlot = Row->AddChildToHorizontalBox(ValueText);
		VSlot->SetVerticalAlignment(VAlign_Center);
		VSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
	}

	// Controllo a larghezza fissa
	USizeBox* ControlBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ControlBox->SetWidthOverride(230.f);
	ControlBox->AddChild(Control);
	UHorizontalBoxSlot* CSlot = Row->AddChildToHorizontalBox(ControlBox);
	CSlot->SetVerticalAlignment(VAlign_Center);

	// Wrapper per lo spacing verticale tra card
	UVerticalBox* Wrapper = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UVerticalBoxSlot* WSlot = Wrapper->AddChildToVerticalBox(Card);
	WSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	return Wrapper;
}

USlider* UVisionMenuWidget::MakeVisionSlider(float MinVal, float MaxVal)
{
	using namespace VisionStyle;

	USlider* S = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	S->SetMinValue(MinVal);
	S->SetMaxValue(MaxVal);
	S->SetSliderBarColor(SliderTrack);
	S->SetSliderHandleColor(Accent);

	FSliderStyle Style = S->GetWidgetStyle();
	Style.SetBarThickness(6.f);

	// Handle circolare bianco con leggera ombra
	FSlateBrush Thumb;
	Thumb.DrawAs = ESlateBrushDrawType::RoundedBox;
	Thumb.TintColor = Accent;
	Thumb.OutlineSettings.CornerRadii = FVector4(11.f, 11.f, 11.f, 11.f);
	Thumb.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	Thumb.ImageSize = FVector2D(22.f, 22.f);
	Style.SetNormalThumbImage(Thumb);
	Style.SetHoveredThumbImage(Thumb);

	S->SetWidgetStyle(Style);
	return S;
}

UTextBlock* UVisionMenuWidget::MakeText(const FString& Text, float Size, FLinearColor Color)
{
	UTextBlock* TB = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TB->SetText(FText::FromString(Text));
	FSlateFontInfo Font = TB->GetFont();
	Font.Size = Size;
	TB->SetFont(Font);
	TB->SetColorAndOpacity(FSlateColor(Color));
	return TB;
}