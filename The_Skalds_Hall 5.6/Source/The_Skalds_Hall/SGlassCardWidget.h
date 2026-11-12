#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Brushes/SlateRoundedBoxBrush.h"
class SGlassCardWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGlassCardWidget)
		: _Title(FText::GetEmpty())
		, _Body(FText::GetEmpty())
		, _Width(900.f)
		, _TitleSize(34.f)
		, _BodySize(23.f)
		, _BackgroundOpacity(0.72f)
		{
		}
		SLATE_ARGUMENT(FText, Title)
		SLATE_ARGUMENT(FText, Body)
		SLATE_ARGUMENT(float, Width)
		SLATE_ARGUMENT(float, TitleSize)
		SLATE_ARGUMENT(float, BodySize)
		SLATE_ARGUMENT(float, BackgroundOpacity)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
private:
	TSharedPtr<FSlateRoundedBoxBrush> GlassBrush;
	TSharedPtr<FSlateRoundedBoxBrush> AccentBrush;
	TSharedPtr<FSlateRoundedBoxBrush> DividerBrush;
};