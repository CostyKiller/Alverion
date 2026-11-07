using UnrealBuildTool;

// The server protocol: login server, game server, and nothing that draws.
public class Network : ModuleRules
{
	public Network(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Sockets", "Networking" });

		// Blowfish and the RSA modular power, the two pieces of the login crypto not worth writing by hand.
		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
	}
}
