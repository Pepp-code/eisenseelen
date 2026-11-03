using UnrealBuildTool;

public class Eisenseelen : ModuleRules
{
	public Eisenseelen(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// Small module: every file compiles alone, so names in anonymous namespaces cannot collide.
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "NavigationSystem", "ProceduralMeshComponent" });
		PrivateDependencyModuleNames.Add("Json");

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
