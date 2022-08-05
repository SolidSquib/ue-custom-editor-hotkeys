// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "ContentBrowserDelegates.h"

class FToolBarBuilder;
class FMenuBuilder;

DECLARE_LOG_CATEGORY_EXTERN(LogCustomEditorHotkeys, Log, All);

class FCustomEditorHotkeysModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command. */
	void PluginButtonClicked();

private:
	void ResetEditorCommands();
	void RegisterMenus();
	void OnExtendContentBrowserCommands(TSharedRef<FUICommandList> CommandList, FOnContentBrowserGetSelection GetSelectionDelegate);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<FUICommandList> CustomLevelEditorCommands;
	TSharedPtr<FUICommandList> CustomContentBrowserCommands;

	FDelegateHandle ContentBrowserCommandExtenderDelegateHandle;
	FDelegateHandle AssetRegistryInitialLoadCompleteDelegateHandle;
};
