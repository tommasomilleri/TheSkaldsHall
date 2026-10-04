#include "SGlassCardWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

void SGlassCardWidget::Construct(const FArguments& InArgs)
{
    // === Equivalente del tuo CSS ===
    // background: rgba(255,255,255,0.07); border-radius: 32px;
    // bordo rim bianco 1.5px (in Slate il bordo è uniforme, non a gradiente)
    GlassBrush = MakeShared<FSlateRoundedBoxBrush>(
        FLinearColor(1.f, 1.f, 1.f, 0.07f),   // colore riempimento
        32.f,                                  // raggio angoli
        FLinearColor(1.f, 1.f, 1.f, 0.45f),   // colore bordo (rim)
        1.5f);                                 // spessore bordo

    // Lo "specular highlight" del tuo ::after — fascia chiara in alto
    HighlightBrush = MakeShared<FSlateRoundedBoxBrush>(
        FLinearColor(1.f, 1.f, 1.f, 0.14f), 32.f);
    HighlightBrushSoft = MakeShared<FSlateRoundedBoxBrush>(
        FLinearColor(1.f, 1.f, 1.f, 0.05f), 32.f);


    ChildSlot
        [
            SNew(SOverlay) // Overlay = elementi sovrapposti (come position:absolute)

                // STRATO 1: il pannello vetro con i testi
                + SOverlay::Slot()
                [
                    SNew(SBorder)
                        .BorderImage(GlassBrush.Get())
                        .Padding(FMargin(28.f, 24.f))
                        [
                            SNew(SVerticalBox)

                                // Titolo
                                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
                                [
                                    SNew(STextBlock)
                                        .Text(InArgs._Title)
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
                                        .ColorAndOpacity(FLinearColor::White)
                                        // come text-shadow nel CSS:
                                        .ShadowOffset(FVector2D(0, 1.5f))
                                        .ShadowColorAndOpacity(FLinearColor(0, 0.04f, 0.12f, 0.45f))
                                ]

                            // Corpo
                            + SVerticalBox::Slot().AutoHeight()
                                [
                                    SNew(STextBlock)
                                        .Text(InArgs._Body)
                                        .AutoWrapText(true)
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                                        .ColorAndOpacity(FLinearColor(1, 1, 1, 0.88f))
                                        .ShadowOffset(FVector2D(0, 1))
                                        .ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.30f))
                                ]
                        ]
                ]

            // STRATO 2: highlight superiore (solo decorativo, non cliccabile)
            + SOverlay::Slot().VAlign(VAlign_Top)
                [
                    SNew(SBox).HeightOverride(80.f)
                        [
                            SNew(SBorder)
                                .BorderImage(HighlightBrush.Get())
                                .Visibility(EVisibility::HitTestInvisible)
                        ]
                ]
        ];
}