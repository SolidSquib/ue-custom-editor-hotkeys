// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class CustomEditorHotkeys : ModuleRules
{
	public CustomEditorHotkeys(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        string enginePath = Path.GetFullPath(Target.RelativeEnginePath);
        string editorSourcePath = enginePath + "Source/Editor/";

        // Must expose blutility's private headers or we can't include ActorActionUtility or related classes...
        PublicIncludePaths.Add(editorSourcePath + "Blutility/Private");

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Blutility",
				"LevelEditor",
				"ContentBrowser",
				"EditorStyle",
				"BlueprintGraph"
				// ... add private dependencies that you statically link with here ...	
			}
			);
	}
}
