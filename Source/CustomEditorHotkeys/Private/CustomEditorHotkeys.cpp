// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomEditorHotkeys.h"
#include "CustomEditorHotkeysStyle.h"
#include "CustomEditorHotkeysCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "LevelEditor.h"
#include "ContentBrowserModule.h"
#include <AssetRegistry/AssetRegistryModule.h>

static const FName CustomEditorHotkeysTabName("CustomEditorHotkeys");

DEFINE_LOG_CATEGORY(LogCustomEditorHotkeys);

#define LOCTEXT_NAMESPACE "FCustomEditorHotkeysModule"

void FCustomEditorHotkeysModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FCustomEditorHotkeysStyle::Initialize();
	FCustomEditorHotkeysStyle::ReloadTextures();
	
	FCustomEditorHotkeysCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);
	CustomLevelEditorCommands = MakeShareable(new FUICommandList);
	CustomContentBrowserCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FCustomEditorHotkeysCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FCustomEditorHotkeysModule::PluginButtonClicked),
		FCanExecuteAction());

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistryInitialLoadCompleteDelegateHandle = AssetRegistry.OnFilesLoaded().AddLambda([this, &AssetRegistry] {

		ResetEditorCommands();
		AssetRegistry.OnFilesLoaded().Remove(AssetRegistryInitialLoadCompleteDelegateHandle);
	});

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FCustomEditorHotkeysModule::RegisterMenus));

	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetGlobalLevelEditorActions()->Append(CustomLevelEditorCommands->AsShared());
	
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowserModule.GetAllContentBrowserCommandExtenders().Add(FContentBrowserCommandExtender::CreateRaw(this, &FCustomEditorHotkeysModule::OnExtendContentBrowserCommands));
	ContentBrowserCommandExtenderDelegateHandle = ContentBrowserModule.GetAllContentBrowserCommandExtenders().Last().GetHandle();
}

void FCustomEditorHotkeysModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FCustomEditorHotkeysStyle::Shutdown();

	for (const TPair<FName, TSharedPtr<FUICommandInfo>>& Command : FCustomEditorHotkeysCommands::Get().CustomLevelEditorCommands)
	{
		CustomLevelEditorCommands->UnmapAction(Command.Value);
	}

	for (const TPair<FName, TSharedPtr<FUICommandInfo>>& Command : FCustomEditorHotkeysCommands::Get().CustomContentBrowserCommands)
	{
		CustomContentBrowserCommands->UnmapAction(Command.Value);
	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowserModule.GetAllContentBrowserCommandExtenders().RemoveAll([this](const FContentBrowserCommandExtender& Delegate) { return Delegate.GetHandle() == ContentBrowserCommandExtenderDelegateHandle; });


	FCustomEditorHotkeysCommands::Unregister();
}

void FCustomEditorHotkeysModule::PluginButtonClicked()
{
	ResetEditorCommands();
}

void FCustomEditorHotkeysModule::ResetEditorCommands()
{
	if (FCustomEditorHotkeysCommands::IsRegistered())
	{
		// Unmap
		for (const auto& Pair : FCustomEditorHotkeysCommands::GetCustomLevelEditorCommands())
		{
			if (CustomLevelEditorCommands->IsActionMapped(Pair.Value))
			{
				CustomLevelEditorCommands->UnmapAction(Pair.Value);				
			}
		}

		for (const auto& Pair : FCustomEditorHotkeysCommands::GetCustomContentBrowserCommands())
		{
			if (CustomContentBrowserCommands->IsActionMapped(Pair.Value))
			{
				CustomContentBrowserCommands->UnmapAction(Pair.Value);
			}
		}

		FCustomEditorHotkeysCommands::GetMutable().RegisterCustomCommands();

		// Remap
		for (const auto& Pair : FCustomEditorHotkeysCommands::GetCustomLevelEditorCommands())
		{
			if (!CustomLevelEditorCommands->IsActionMapped(Pair.Value))
			{
				CustomLevelEditorCommands->MapAction(Pair.Value,
					FExecuteAction::CreateStatic(&FCustomEditorHotkeysBlutilityExtensions::ExecuteActorUtilityFunctionByName, Pair.Key));
			}
			else
			{
				UE_LOG(LogCustomEditorHotkeys, Warning, TEXT("Duplicate Custom Command mapping found: \"%s\""), *Pair.Key.ToString());
			}
		}

		for (const auto& Pair : FCustomEditorHotkeysCommands::GetCustomContentBrowserCommands())
		{
			if (!CustomContentBrowserCommands->IsActionMapped(Pair.Value))
			{
				CustomContentBrowserCommands->MapAction(Pair.Value,
					FExecuteAction::CreateStatic(&FCustomEditorHotkeysBlutilityExtensions::ExecuteAssetUtilityFunctionByName, Pair.Key));
			}
			else
			{
				UE_LOG(LogCustomEditorHotkeys, Warning, TEXT("Duplicate Custom Command mapping found: \"%s\""), *Pair.Key.ToString());
			}
		}
	}
}

void FCustomEditorHotkeysModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FCustomEditorHotkeysCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FCustomEditorHotkeysCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

void FCustomEditorHotkeysModule::OnExtendContentBrowserCommands(TSharedRef<FUICommandList> CommandList, FOnContentBrowserGetSelection GetSelectionDelegate)
{
	CommandList->Append(CustomContentBrowserCommands->AsShared());
	UE_LOG(LogCustomEditorHotkeys, Log, TEXT("Custom commands appended to content browser module."));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCustomEditorHotkeysModule, CustomEditorHotkeys)