// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LastDay : ModuleRules
{
	public LastDay(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// 编辑器下需要 AssetRegistry 来注册自动生成的材质资源
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "AssetRegistry" });
		}

		PublicIncludePaths.AddRange(new string[] {
			"LastDay",
			"LastDay/Demo",
			"LastDay/Unit",
			"LastDay/Construction",
			"LastDay/Explosions",
			"LastDay/Projectiles",
			"LastDay/Weapons",
			"LastDay/Variant_Horror",
			"LastDay/Variant_Horror/UI",
			"LastDay/Variant_Shooter",
			"LastDay/Variant_Shooter/AI",
			"LastDay/Variant_Shooter/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
