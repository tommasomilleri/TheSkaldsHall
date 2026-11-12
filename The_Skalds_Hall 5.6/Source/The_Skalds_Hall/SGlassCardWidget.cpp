#include "SGlassCardWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

void SGlassCardWidget::Construct(const FArguments& InArgs)
{
    const float Width = FMath::Max(InArgs._Width, 300.f);
    const float TitleSize = FMath::Max(InArgs._TitleSize, 8.f);
    const float BodySize = FMath::Max(InArgs._BodySize, 8.f);
    const float Opacity = FMath::Clamp(InArgs._BackgroundOpacity, 0.f, 1.f);

    const float PadX = 40.f;
    const float PadY = 32.f;
    const float TextWidth = Width - PadX * 2.f;

    // Fondo scuro semitrasparente: testo leggibile anche davanti a muri chiari.
    GlassBrush = MakeShared<FSlateRoundedBoxBrush>(
        FLinearColor(0.02f, 0.025f, 0.035f, Opacity),
        28.f,
        FLinearColor(1.f, 1.f, 1.f, 0.35f),
        2.f);

    AccentBrush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor(0.95f, 0.78f, 0.42f, 1.f), 3.f);
    DividerBrush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor(1.f, 1.f, 1.f, 0.18f), 1.f);

    ChildSlot
        [
            SNew(SBox)
                .WidthOverride(Width)
                [
                    SNew(SBorder)
                        .BorderImage(GlassBrush.Get())
                        .Padding(FMargin(PadX, PadY, PadX, PadY + 4.f))
                        [
                            SNew(SVerticalBox)

                                + SVerticalBox::Slot()
                                .AutoHeight()
                                [
                                    SNew(SHorizontalBox)
                                        + SHorizontalBox::Slot()
                                        .AutoWidth()
                                        .VAlign(VAlign_Fill)
                                        .Padding(0.f, 4.f, 16.f, 4.f)
                                        [
                                            SNew(SBox)
                                                .WidthOverride(6.f)
                                                [
                                                    SNew(SImage).Image(AccentBrush.Get())
                                                ]
                                        ]
                                    + SHorizontalBox::Slot()
                                        .FillWidth(1.f)
                                        .VAlign(VAlign_Center)
                                        [
                                            SNew(STextBlock)
                                                .Text(InArgs._Title)
                                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", FMath::RoundToInt(TitleSize)))
                                                .ColorAndOpacity(FLinearColor::White)
                                                .AutoWrapText(false)
                                                .WrapTextAt(TextWidth - 22.f)
                                                .LineHeightPercentage(1.1f)
                                                .ShadowOffset(FVector2D(0.f, 2.f))
                                                .ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
                                        ]
                                ]

                            + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.f, 16.f, 0.f, 18.f)
                                [
                                    SNew(SBox)
                                        .HeightOverride(2.f)
                                        [
                                            SNew(SImage).Image(DividerBrush.Get())
                                        ]
                                ]

                            + SVerticalBox::Slot()
                                .AutoHeight()
                                [
                                    SNew(STextBlock)
                                        .Text(InArgs._Body)
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", FMath::RoundToInt(BodySize)))
                                        .ColorAndOpacity(FLinearColor(0.96f, 0.96f, 0.96f, 1.f))
                                        .AutoWrapText(false)
                                        .WrapTextAt(TextWidth)
                                        .LineHeightPercentage(1.22f)
                                        .ShadowOffset(FVector2D(0.f, 1.5f))
                                        .ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f))
                                ]
                        ]
                ]
        ];
}