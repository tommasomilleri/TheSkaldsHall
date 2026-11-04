#include "SGlassCardWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

void SGlassCardWidget::Construct(const FArguments& InArgs)
{
	GlassBrush = MakeShared<FSlateRoundedBoxBrush>(
		FLinearColor(1.f, 1.f, 1.f, 0.07f),
		32.f,
		FLinearColor(1.f, 1.f, 1.f, 0.45f),
		1.5f);

	HighlightBrush = MakeShared<FSlateRoundedBoxBrush>(
		FLinearColor(1.f, 1.f, 1.f, 0.14f), 32.f);

	HighlightBrushSoft = MakeShared<FSlateRoundedBoxBrush>(
		FLinearColor(1.f, 1.f, 1.f, 0.05f), 32.f);

	ChildSlot
		[
			SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder)
						.BorderImage(GlassBrush.Get())
						.Padding(FMargin(28.f, 24.f))
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
								[
									SNew(STextBlock)
										.Text(InArgs._Title)
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
										.ColorAndOpacity(FLinearColor::White)
										.ShadowOffset(FVector2D(0.f, 1.5f))
										.ShadowColorAndOpacity(FLinearColor(0.f, 0.04f, 0.12f, 0.45f))
								]
							+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
										.Text(InArgs._Body)
										.AutoWrapText(true)
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
										.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.88f))
										.ShadowOffset(FVector2D(0.f, 1.f))
										.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.30f))
								]
						]
				]
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