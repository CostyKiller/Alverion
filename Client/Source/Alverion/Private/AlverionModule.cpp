#include "Internationalization/StringTableRegistry.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "MeshPartitionHeightmapImporter.h"
#endif

/** Registers the menus' string table before any screen asks it for text, and lets a game run from the editor open the continent. */
class FAlverionModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// A CSV rather than a string table asset, so anyone can add or change a line as text.
		// The namespace is what the translations are keyed under, so it must stay ST_Game for them to match.
		LOCTABLE_FROMFILE_GAME("ST_Game", "ST_Game", "Localization/StringTables/ST_Game.csv");

#if WITH_EDITOR
		// L_Continent's terrain source actors carry a component class from the plugin's editor library.
		// A game run from the editor executable loads no editor modules, and World Partition asserts reading the map without that class.
		// Starting the module would need the level editor, so its library is only linked: one exported function, named here, makes the loader bring it in and register its classes.
		// A packaged game has neither those actors nor this library.
		UE_LOG(LogTemp, Verbose, TEXT("Terrain editor library linked at %p."), &UE::MeshPartition::FHeightmapImporter::GetSectionInfos);
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FAlverionModule, Alverion, "Alverion");
