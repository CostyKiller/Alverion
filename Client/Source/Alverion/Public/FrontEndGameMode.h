#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "FrontEndGameMode.generated.h"

class SCharacterScreen;
class SCreateScreen;
class SLoginScreen;
class SWidget;
class UServerConnectionSubsystem;

/** Login, the character list and character creation, over a daylit empty map. Hands over to the world when a character is chosen. */
UCLASS()
class ALVERION_API AFrontEndGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFrontEndGameMode();

	/** Where the world is loaded, with the world's game mode named in the travel options. */
	static void travelToWorld(UObject* worldContext);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type reason) override;

private:
	void show(const TSharedRef<SWidget>& screen);
	void showLogin();
	void showCharacters();
	void showCreate();

	void login(const FString& host, const FString& account, const FString& password);
	void bindFlows();
	void unbindFlows();
	UServerConnectionSubsystem* connection() const;

	/** From -autologin=account,password,character in development builds: the character to enter once the list arrives. */
	FString _autoEnter;

	TSharedPtr<SWidget> _shown;
	TSharedPtr<SLoginScreen> _login;
	TSharedPtr<SCharacterScreen> _characters;
	TSharedPtr<SCreateScreen> _create;
};
