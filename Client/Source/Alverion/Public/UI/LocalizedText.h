#pragma once

#include "CoreMinimal.h"

class UWidgetTree;

// Applies string table entries to text blocks by widget name, so a screen can be localized without binding
// every label as a variable. The table is seeded from the shared translation files.
namespace LocalizedText
{
	// The one table the whole front end reads from.
	ALVERION_API extern const TCHAR* gameTableId;

	ALVERION_API FText get(const FString& key);

	// The entry, or the given text when the table has no such key.
	// get returns the key itself, which is right for a label nobody wrote yet and wrong where something better is already to hand.
	ALVERION_API FText getOr(const FString& key, const FText& fallback);

	// For each pair, finds the text block named by the key and sets it to the string table entry named by the
	// value. A missing widget is skipped, so a blueprint can drop a label without breaking anything.
	ALVERION_API void applyToTree(UWidgetTree* widgetTree, const TMap<FName, FString>& widgetNameToKey);
}
