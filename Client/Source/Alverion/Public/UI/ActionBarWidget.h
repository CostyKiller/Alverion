#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "ActionBarWidget.generated.h"

/**
 * The row of ten slots along the bottom of the world screen, captioned with the number keys 1 to 0.
 * A placeholder: the slots hold nothing and the keys do nothing until there are skills and items to put in them.
 */
UCLASS()
class ALVERION_API UActionBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
