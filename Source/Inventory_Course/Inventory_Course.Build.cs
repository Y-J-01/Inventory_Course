// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Inventory_Course : ModuleRules
{
	public Inventory_Course(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Inventory_Course",
			"Inventory_Course/Variant_Platforming",
			"Inventory_Course/Variant_Platforming/Animation",
			"Inventory_Course/Variant_Combat",
			"Inventory_Course/Variant_Combat/AI",
			"Inventory_Course/Variant_Combat/Animation",
			"Inventory_Course/Variant_Combat/Gameplay",
			"Inventory_Course/Variant_Combat/Interfaces",
			"Inventory_Course/Variant_Combat/UI",
			"Inventory_Course/Variant_SideScrolling",
			"Inventory_Course/Variant_SideScrolling/AI",
			"Inventory_Course/Variant_SideScrolling/Gameplay",
			"Inventory_Course/Variant_SideScrolling/Interfaces",
			"Inventory_Course/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
