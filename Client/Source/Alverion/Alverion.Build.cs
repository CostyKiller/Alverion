using UnrealBuildTool;

public class Alverion : ModuleRules
{
	public Alverion(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "CommonUI", "CommonInput", "GameplayTags" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "Network", "ApplicationCore", "RHI" });

		// Editor builds only: links the terrain plugin's editor library so a game run from the editor executable can read L_Continent. See AlverionModule.cpp.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "MeshPartition", "MeshPartitionEditor" });
		}
	}
}
