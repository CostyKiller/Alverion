#include "LocalizedText.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"

namespace LocalizedText
{
	// Registered from Content/Localization/StringTables/ST_Game.csv when the module starts.
	const TCHAR* gameTableId = TEXT("ST_Game");

	FText get(const FString& key)
	{
		// FindOrLoad is the default and it is not enough.
		// It settles for a table that was found but not fully loaded,
		// which outside the editor is a table holding no entries at all,
		// so every lookup rendered as MISSING STRING TABLE ENTRY with nothing logged to say why.
		// This runs while a widget is being built, which is a safe place to block on a load.
		const FText text = FText::FromStringTable(FName(gameTableId), key, EStringTableLoadingPolicy::FindOrFullyLoad);

		// A key missing from the table renders as its own name,
		// which is louder than empty text and points straight at what needs adding.
		// The table has to be asked directly,
		// because a missing entry comes back as a complaint rather than as empty text.
		const FStringTableConstPtr table = FStringTableRegistry::Get().FindStringTable(FName(gameTableId));
		if (!table.IsValid() || !table->FindEntry(key).IsValid())
		{
			return FText::FromString(key);
		}

		return text;
	}

	FText getOr(const FString& key, const FText& fallback)
	{
		const FText text = FText::FromStringTable(FName(gameTableId), key, EStringTableLoadingPolicy::FindOrFullyLoad);

		// The table is asked directly for the same reason get does it: a missing entry comes back as a complaint rather than as empty text, so testing the text tells you nothing.
		const FStringTableConstPtr table = FStringTableRegistry::Get().FindStringTable(FName(gameTableId));
		if (!table.IsValid() || !table->FindEntry(key).IsValid())
		{
			return fallback;
		}

		return text;
	}

	void applyToTree(UWidgetTree* widgetTree, const TMap<FName, FString>& widgetNameToKey)
	{
		if (widgetTree == nullptr)
		{
			return;
		}

		for (const TPair<FName, FString>& pair : widgetNameToKey)
		{
			UTextBlock* textBlock = Cast<UTextBlock>(widgetTree->FindWidget(pair.Key));
			if (textBlock == nullptr)
			{
				continue;
			}

			textBlock->SetText(get(pair.Value));
		}
	}
}
