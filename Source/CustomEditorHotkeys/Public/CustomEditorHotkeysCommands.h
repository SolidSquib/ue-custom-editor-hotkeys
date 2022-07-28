// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "CustomEditorHotkeysStyle.h"

class FCustomEditorHotkeysCommands : public TCommands<FCustomEditorHotkeysCommands>
{
public:

	FCustomEditorHotkeysCommands()
		: TCommands<FCustomEditorHotkeysCommands>(TEXT("CustomEditorHotkeys"), NSLOCTEXT("Contexts", "CustomEditorHotkeys", "CustomEditorHotkeys Plugin"), NAME_None, FCustomEditorHotkeysStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

	static TMap<FName, TSharedPtr<FUICommandInfo>> GetCustomLevelEditorCommands()
	{
		return FCustomEditorHotkeysCommands::Get().CustomLevelEditorCommands;
	}

	static TMap<FName, TSharedPtr<FUICommandInfo>> GetCustomContentBrowserCommands()
	{
		return FCustomEditorHotkeysCommands::Get().CustomContentBrowserCommands;
	}

protected:
	friend class FCustomEditorHotkeysModule;

	void RegisterCustomCommands();

	static FCustomEditorHotkeysCommands& GetMutable()
	{
		return *(Instance.Pin());
	}

public:
	TSharedPtr<FUICommandInfo> PluginAction;
	TMap<FName, TSharedPtr<FUICommandInfo>> CustomLevelEditorCommands;
	TMap<FName, TSharedPtr<FUICommandInfo>> CustomContentBrowserCommands;
};

//////////////////////////////////////////////////////////////////////////

class UEditorUtilityObject;

// Blutility Menu extension helpers
class FCustomEditorHotkeysBlutilityExtensions
{
public:
	struct FFunctionAndUtil
	{
		FFunctionAndUtil(UFunction* InFunction, UEditorUtilityObject* InUtil)
			: Function(InFunction)
			, Util(InUtil) {}

		bool operator==(const FFunctionAndUtil& InFunction) const
		{
			return InFunction.Function == Function;
		}

		UFunction* Function;
		UEditorUtilityObject* Util;
	};

public:
	static void GetBlutilityClasses(TArray<FAssetData>& OutAssets, const FName& InClassName);
	static void CreateBlutilityActionsMenu(FMenuBuilder& MenuBuilder, TArray<class UEditorUtilityObject*> Utils);
	static TArray<UEditorUtilityObject*> GetUtilitiesSupportedBySelectedActors(const TArray<AActor*>& SelectedActors);
	static void GetUtilityFunctions(UEditorUtilityObject* Utility, TArray<FFunctionAndUtil>& OutFunctions, bool bDoSort = false);
	static void GetUtilityFunctions(const TArray<UEditorUtilityObject*>& Utilities, TArray<FFunctionAndUtil>& OutFunctions, bool bDoSort = false);
	static void ExecuteUtilityFunctionByName(FName FunctionName);
	static void ExecuteUtilityFunction(const FFunctionAndUtil& FunctionAndUtil);
};