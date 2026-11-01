#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Brushes/SlateRoundedBoxBrush.h"

/**
 * Scheda liquid glass in Slate puro.
 */
class SGlassCardWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGlassCardWidget)
		: _Title(FText::GetEmpty())
		, _Body(FText::GetEmpty())
		{
		}
		SLATE_ARGUMENT(FText, Title)
		SLATE_ARGUMENT(FText, Body)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedPtr<FSlateRoundedBoxBrush> GlassBrush;
	TSharedPtr<FSlateRoundedBoxBrush> HighlightBrush;
	TSharedPtr<FSlateRoundedBoxBrush> HighlightBrushSoft;
};