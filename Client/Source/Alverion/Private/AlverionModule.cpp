#include "Internationalization/StringTableRegistry.h"
#include "Modules/ModuleManager.h"

/** Registers the menus' string table before any screen asks it for text. */
class FAlverionModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// A CSV rather than a string table asset, so anyone can add or change a line as text.
		// The namespace is what the translations are keyed under, so it must stay ST_Game for them to match.
		LOCTABLE_FROMFILE_GAME("ST_Game", "ST_Game", "Localization/StringTables/ST_Game.csv");
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FAlverionModule, Alverion, "Alverion");
