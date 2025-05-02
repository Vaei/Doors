// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Doors : ModuleRules
{
	public Doors(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayAbilities",
				"GameplayTags",
				"DeveloperSettings",
				"TargetingSystem",
				"Grasp",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"NetCore",
				"UMG",
			}
			);
	}
}
