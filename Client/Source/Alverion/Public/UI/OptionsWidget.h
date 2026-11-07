#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include "OptionsWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class USlider;
class USoundBase;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

// The one options screen. It is pushed onto the Menu layer from the main menu and onto GameMenu from the in
// world game menu later, which is why it holds no reference to either.
UCLASS()
class ALVERION_API UOptionsWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& geometry, float deltaSeconds) override;
	virtual FReply NativeOnKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& geometry, const FPointerEvent& pointerEvent) override;

	// Called by a key row when it is clicked. The next key or mouse press lands on this mapping and slot.
	void beginListening(FName mappingName, EPlayerMappableKeySlot slot, UTextBlock* keyText);

	// Selects a category the way its tab button would, so a screenshot can reach a page nobody clicked.
	void showCategory(int32 index);

protected:
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	// Category tabs and the switcher they drive.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> categorySwitcher;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> displayTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> audioTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> inputTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> gameplayTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> interfaceTabButton;

	// Display.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> resolutionCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> windowModeCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> frameCapCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> qualityCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> upscalerCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> screenPercentageSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> screenPercentageValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> naniteCheck;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> lumenCheck;

	// One per scalability bucket, in the order UGameUserSettings declares them.
	// The overall quality dropdown above writes all eleven at once, these override one at a time.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> viewDistanceCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> shadowCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> globalIlluminationCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> reflectionCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> antiAliasingCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> textureCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> effectsCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> postProcessingCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> foliageCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> shadingCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> landscapeCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> vsyncCheck;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> showFpsCheck;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> showStatsCheck;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> fovSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> fovValueText;

	// Audio.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> masterVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> musicVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> sfxVolumeSlider;

	// Input. The rows are built at runtime from the player's key profile, so the blueprint only supplies the box.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> keyBindingBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> mouseSensitivitySlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mouseSensitivityValueText;

	// Gameplay.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> cameraModeCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> nameplatesCheck;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> damageNumbersCheck;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> chatFilterCheck;

	// Interface.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> hudScaleSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> hudScaleValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> languageCombo;

	// Footer.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> applyButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> revertButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> defaultsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> backButton;

	// Shown only while a resolution change is waiting to be confirmed.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> revertCountdownText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> keepChangesButton;

	// A resolution nobody can see cannot be undone by the player, so it undoes itself.
	UPROPERTY(EditDefaultsOnly, Category = "Options")
	float resolutionRevertSeconds = 15.0f;

	// Played on every button press, and on the effects slider so its level can be heard while dragging.
	UPROPERTY(EditDefaultsOnly, Category = "Options")
	TObjectPtr<USoundBase> clickSound;

private:
	void playClickSound() const;
	void populateDisplay();
	void populateAudio();
	void populateInput();
	void populateGameplay();
	void populateInterface();
	void rebuildKeyBindingRows();

	void applySettings();
	void revertSettings();
	void restoreDefaults();
	void restoreDefaultKeyBindings();

	void beginResolutionCountdown();
	void finishResolutionCountdown(bool keepChanges);

	// Returns true when the press was consumed by a mapping that is waiting to be rebound.
	bool applyPendingRebind(const FKey& key);

	// Clears every other binding already holding this key, so the new one wins. Returns how many were displaced.
	int32 clearConflictingBindings(class UEnhancedInputUserSettings* inputSettings, const FKey& key, FName keepMappingName, EPlayerMappableKeySlot keepSlot) const;

	// Reapplies every translated label in place, so a language change shows without reopening the screen.
	void refreshLocalizedText();

	UFUNCTION()
	void handleDisplayTabClicked();

	UFUNCTION()
	void handleAudioTabClicked();

	UFUNCTION()
	void handleInputTabClicked();

	UFUNCTION()
	void handleGameplayTabClicked();

	UFUNCTION()
	void handleInterfaceTabClicked();

	UFUNCTION()
	void handleApplyClicked();

	UFUNCTION()
	void handleRevertClicked();

	UFUNCTION()
	void handleDefaultsClicked();

	UFUNCTION()
	void handleBackClicked();

	UFUNCTION()
	void handleKeepChangesClicked();

	UFUNCTION()
	void handleFovChanged(float value);

	UFUNCTION()
	void handleMouseSensitivityChanged(float value);

	UFUNCTION()
	void handleScreenPercentageChanged(float value);

	UFUNCTION()
	void handleHudScaleChanged(float value);

	UFUNCTION()
	void handleMasterVolumeChanged(float value);

	UFUNCTION()
	void handleMusicVolumeChanged(float value);

	UFUNCTION()
	void handleSfxVolumeChanged(float value);

	// Name of the mapping waiting for a key press, none when nothing is listening.
	FName _listeningMappingName = NAME_None;
	EPlayerMappableKeySlot _listeningSlot = EPlayerMappableKeySlot::First;

	UPROPERTY()
	TObjectPtr<UTextBlock> _listeningKeyText;

	// Resolution held while the countdown runs, so a display that never came back can be restored.
	FIntPoint _resolutionBeforeApply = FIntPoint::ZeroValue;
	TEnumAsByte<EWindowMode::Type> _windowModeBeforeApply = EWindowMode::WindowedFullscreen;
	float _revertSecondsLeft = 0.0f;

	// One per key row, kept alive here because the button delegate holds no reference of its own.
	UPROPERTY()
	TArray<TObjectPtr<class UKeyRebindHandler>> _rebindHandlers;

	// Held so the culture delegate is released when the screen goes away, since it outlives this widget.
	FDelegateHandle _cultureChangedHandle;
};

// A button click delegate cannot carry which row it came from, so each row gets one of these to remember.
UCLASS()
class ALVERION_API UKeyRebindHandler : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName mappingName;

	// Which of the action's binding slots this button edits.
	EPlayerMappableKeySlot slot = EPlayerMappableKeySlot::First;

	UPROPERTY()
	TObjectPtr<UOptionsWidget> owner;

	UPROPERTY()
	TObjectPtr<UTextBlock> keyText;

	UFUNCTION()
	void handleClicked();
};
