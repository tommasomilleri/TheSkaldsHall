#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Brushes/SlateRoundedBoxBrush.h"

/**
 * Scheda "liquid glass": pannello arrotondato semitrasparente,
 * bordo luminoso (rim), highlight in alto, testo con ombra.
 */
class SGlassCardWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGlassCardWidget)
		: _Title(FText::GetEmpty()), _Body(FText::GetEmpty()) {
		}
		SLATE_ARGUMENT(FText, Title)   // titolo della scheda
			SLATE_ARGUMENT(FText, Body)    // testo descrittivo
	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

private:
	// I "brush" sono l'equivalente Slate del background CSS.
	// Li teniamo come membri perché devono vivere quanto il widget.
	TSharedPtr<FSlateRoundedBoxBrush> GlassBrush;
	TSharedPtr<FSlateRoundedBoxBrush> HighlightBrush;
	TSharedPtr<FSlateRoundedBoxBrush> HighlightBrushSoft;
};