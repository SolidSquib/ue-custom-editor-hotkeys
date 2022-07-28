// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomEditorHotkeysCommands.h"

#include "AssetRegistryModule.h"
#include "BlueprintEditorModule.h"
#include "SFunctionParamDialog.h"
#include "ActorActionUtility.h"
#include "AssetActionUtility.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityBlueprint.h"
#include "EditorUtilityObject.h"

#include "Editor/UnrealEdEngine.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "UnrealEdGlobals.h"

#define LOCTEXT_NAMESPACE "FCustomEditorHotkeysModule"

void FCustomEditorHotkeysCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "Refresh Custom Editor Hotkeys", "Refresh mapped hotkeys, adding new commands to the editor preferences and removing outdated ones.", EUserInterfaceActionType::Button, FInputChord());
}

void FCustomEditorHotkeysCommands::RegisterCustomCommands()
{
	TArray<FAssetData> Assets;
	FCustomEditorHotkeysBlutilityExtensions::GetBlutilityClasses(Assets, UActorActionUtility::StaticClass()->GetFName());
	FCustomEditorHotkeysBlutilityExtensions::GetBlutilityClasses(Assets, UAssetActionUtility::StaticClass()->GetFName());
		
	TArray<FCustomEditorHotkeysBlutilityExtensions::FFunctionAndUtil> UtilityFunctions;

	for (FAssetData& Asset : Assets)
	{
		if (UEditorUtilityBlueprint* Blueprint = Cast<UEditorUtilityBlueprint>(Asset.GetAsset()))
		{
			if (UClass* BPClass = Blueprint->GeneratedClass.Get())
			{
				if (UEditorUtilityObject* DefaultObject = Cast<UEditorUtilityObject>(BPClass->GetDefaultObject()))
				{					
					FCustomEditorHotkeysBlutilityExtensions::GetUtilityFunctions(DefaultObject, UtilityFunctions);
				}
			}
		}
	}

	for (const FCustomEditorHotkeysBlutilityExtensions::FFunctionAndUtil& UtilityFunction : UtilityFunctions)
	{
		FName FunctionName = FName(UtilityFunction.Function->GetName());		
		if (CustomLevelEditorCommands.Contains(FunctionName))
		{
			UE_LOG(LogTemp, Warning, TEXT("Duplicate custom command name found. Ignoring: \"%s\""), *FunctionName.ToString());
			continue;
		}

		FName DotName("." + FunctionName.ToString());
		ANSICHAR AnsiDotName[NAME_SIZE];
		DotName.GetPlainANSIString(AnsiDotName);

		TSharedPtr<FUICommandInfo> NewCommand;
		FUICommandInfo::MakeCommandInfo(AsShared(),
			NewCommand,
			FunctionName,
			FText::AsCultureInvariant(FunctionName.ToString()),
			FText::AsCultureInvariant(UtilityFunction.Function->GetDesc()),
			FSlateIcon(GetStyleSetName(), ISlateStyle::Join(GetContextName(), AnsiDotName)),
			EUserInterfaceActionType::Button,
			FInputChord()
		);

		if (UtilityFunction.Util->IsA<UActorActionUtility>())
		{
			CustomLevelEditorCommands.Add(FunctionName, NewCommand);
		}
		else if (UtilityFunction.Util->IsA<UAssetActionUtility>())
		{
			CustomContentBrowserCommands.Add(FunctionName, NewCommand);
		}
	}

	CommandsChanged.Broadcast(*this);
}

//////////////////////////////////////////////////////////////////////////

void FCustomEditorHotkeysBlutilityExtensions::GetBlutilityClasses(TArray<FAssetData>& OutAssets, const FName& InClassName)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Get class names
	TArray<FName> BaseNames;
	BaseNames.Add(InClassName);
	TSet<FName> Excluded;
	TSet<FName> DerivedNames;
	AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedNames);

	// Now get all UEditorUtilityBlueprint assets
	FARFilter Filter;
	Filter.ClassNames.Add(UEditorUtilityBlueprint::StaticClass()->GetFName());
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	// Check each asset to see if it matches our type
	for (const FAssetData& Asset : AssetList)
	{
		FAssetDataTagMapSharedView::FFindTagResult Result = Asset.TagsAndValues.FindTag(FBlueprintTags::GeneratedClassPath);
		if (Result.IsSet())
		{
			const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(Result.GetValue());
			const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);

			if (DerivedNames.Contains(*ClassName))
			{
				OutAssets.Add(Asset);
			}
		}
	}
}

void FCustomEditorHotkeysBlutilityExtensions::CreateBlutilityActionsMenu(FMenuBuilder& MenuBuilder, TArray<UEditorUtilityObject*> Utils)
{
	const static FName NAME_CallInEditor(TEXT("CallInEditor"));

	TArray<FFunctionAndUtil> FunctionsToList;
	TSet<UClass*> ProcessedClasses;

	// Find the exposed functions available in each class, making sure to not list shared functions from a parent class more than once
	for (UEditorUtilityObject* Util : Utils)
	{
		UClass* Class = Cast<UObject>(Util)->GetClass();

		if (ProcessedClasses.Contains(Class))
		{
			continue;
		}

		for (UClass* ParentClass = Class; ParentClass != UObject::StaticClass(); ParentClass = ParentClass->GetSuperClass())
		{
			ProcessedClasses.Add(ParentClass);
		}

		for (TFieldIterator<UFunction> FunctionIt(Class); FunctionIt; ++FunctionIt)
		{
			if (UFunction* Func = *FunctionIt)
			{
				if (Func->HasMetaData(NAME_CallInEditor) && Func->GetReturnProperty() == nullptr)
				{
					FunctionsToList.AddUnique(FFunctionAndUtil(Func, Util));
				}
			}
		}
	}

	// Sort the functions by name
	FunctionsToList.Sort([](const FFunctionAndUtil& A, const FFunctionAndUtil& B) { return A.Function->GetName() < B.Function->GetName(); });

	// Add a menu item for each function
	if (FunctionsToList.Num() > 0)
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT("ScriptedActorActions", "Scripted Actions"),
			LOCTEXT("ScriptedActorActionsTooltip", "Scripted actions available for the selected actors"),
			FNewMenuDelegate::CreateLambda([FunctionsToList](FMenuBuilder& InMenuBuilder)
				{
					for (const FFunctionAndUtil& FunctionAndUtil : FunctionsToList)
					{
						const FText TooltipText = FText::Format(LOCTEXT("AssetUtilTooltipFormat", "{0}\n(Shift-click to edit script)"), FunctionAndUtil.Function->GetToolTipText());

						InMenuBuilder.AddMenuEntry(
							FunctionAndUtil.Function->GetDisplayNameText(),
							TooltipText,
							FSlateIcon("EditorStyle", "GraphEditor.Event_16x"),
							FExecuteAction::CreateLambda([FunctionAndUtil]
								{
									if (FSlateApplication::Get().GetModifierKeys().IsShiftDown())
									{
										// Edit the script if we have shift held down
										if (UBlueprint* Blueprint = Cast<UBlueprint>(Cast<UObject>(FunctionAndUtil.Util)->GetClass()->ClassGeneratedBy))
										{
											if (IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Blueprint, true))
											{
												check(AssetEditor->GetEditorName() == TEXT("BlueprintEditor"));
												IBlueprintEditor* BlueprintEditor = static_cast<IBlueprintEditor*>(AssetEditor);
												BlueprintEditor->JumpToHyperlink(FunctionAndUtil.Function, false);
											}
											else
											{
												FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
												TSharedRef<IBlueprintEditor> BlueprintEditor = BlueprintEditorModule.CreateBlueprintEditor(EToolkitMode::Standalone, TSharedPtr<IToolkitHost>(), Blueprint, false);
												BlueprintEditor->JumpToHyperlink(FunctionAndUtil.Function, false);
											}
										}
									}
									else
									{
										// We dont run this on the CDO, as bad things could occur!
										UObject* TempObject = NewObject<UObject>(GetTransientPackage(), Cast<UObject>(FunctionAndUtil.Util)->GetClass());
										TempObject->AddToRoot(); // Some Blutility actions might run GC so the TempObject needs to be rooted to avoid getting destroyed

										if (FunctionAndUtil.Function->NumParms > 0)
										{
											// Create a parameter struct and fill in defaults
											TSharedRef<FStructOnScope> FuncParams = MakeShared<FStructOnScope>(FunctionAndUtil.Function);
											for (TFieldIterator<FProperty> It(FunctionAndUtil.Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
											{
												FString Defaults;
												if (UEdGraphSchema_K2::FindFunctionParameterDefaultValue(FunctionAndUtil.Function, *It, Defaults))
												{
													It->ImportText(*Defaults, It->ContainerPtrToValuePtr<uint8>(FuncParams->GetStructMemory()), PPF_None, nullptr);
												}
											}

											// pop up a dialog to input params to the function
											TSharedRef<SWindow> Window = SNew(SWindow)
												.Title(FunctionAndUtil.Function->GetDisplayNameText())
												.ClientSize(FVector2D(400, 200))
												.SupportsMinimize(false)
												.SupportsMaximize(false);

											TSharedPtr<SFunctionParamDialog> Dialog;
											Window->SetContent(
												SAssignNew(Dialog, SFunctionParamDialog, Window, FuncParams)
												.OkButtonText(LOCTEXT("OKButton", "OK"))
												.OkButtonTooltipText(FunctionAndUtil.Function->GetToolTipText()));

											GEditor->EditorAddModalWindow(Window);

											if (Dialog->bOKPressed)
											{
												FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BlutilityAction", "Blutility Action"));
												FEditorScriptExecutionGuard ScriptGuard;
												TempObject->ProcessEvent(FunctionAndUtil.Function, FuncParams->GetStructMemory());
											}
										}
										else
										{
											FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BlutilityAction", "Blutility Action"));
											FEditorScriptExecutionGuard ScriptGuard;
											TempObject->ProcessEvent(FunctionAndUtil.Function, nullptr);
										}

										TempObject->RemoveFromRoot();
									}
								}));
					}
				}),
			false,
					FSlateIcon("EditorStyle", "GraphEditor.Event_16x"));
	}
}

TArray<UEditorUtilityObject*> FCustomEditorHotkeysBlutilityExtensions::GetUtilitiesSupportedBySelectedActors(const TArray<AActor*>& SelectedActors)
{
	TArray<UEditorUtilityObject*> SupportedUtils;
	TArray<FAssetData> UtilAssets;
	GetBlutilityClasses(UtilAssets, UActorActionUtility::StaticClass()->GetFName());

	if (UtilAssets.Num() > 0)
	{
		for (AActor* Actor : SelectedActors)
		{
			if (Actor)
			{
				for (FAssetData& UtilAsset : UtilAssets)
				{
					if (UEditorUtilityBlueprint* Blueprint = Cast<UEditorUtilityBlueprint>(UtilAsset.GetAsset()))
					{
						if (UClass* BPClass = Blueprint->GeneratedClass.Get())
						{
							if (UActorActionUtility* DefaultObject = Cast<UActorActionUtility>(
								BPClass->GetDefaultObject()))
							{
								UClass* SupportedClass = DefaultObject->GetSupportedClass();
								if (SupportedClass == nullptr || (SupportedClass && Actor->GetClass()->IsChildOf(
									SupportedClass)))
								{
									SupportedUtils.AddUnique(DefaultObject);
								}
							}
						}
					}
				}
			}
		}
	}

	return SupportedUtils;
}

void FCustomEditorHotkeysBlutilityExtensions::GetUtilityFunctions(UEditorUtilityObject* Utility, TArray<FFunctionAndUtil>& OutFunctions, bool bDoSort /*= false*/)
{
	UClass* Class = Cast<UObject>(Utility)->GetClass();

	for (TFieldIterator<UFunction> FunctionIt(Class); FunctionIt; ++FunctionIt)
	{
		if (UFunction* Func = *FunctionIt)
		{
			if (Func->HasMetaData(TEXT("CallInEditor")) && Func->GetReturnProperty() == nullptr)
			{
				OutFunctions.AddUnique(FFunctionAndUtil(Func, Utility));
			}
		}
	}
}

void FCustomEditorHotkeysBlutilityExtensions::GetUtilityFunctions(const TArray<UEditorUtilityObject*>& Utilities, TArray<FFunctionAndUtil>& OutFunctions, bool bDoSort /*= false*/)
{
	TSet<UClass*> ProcessedClasses;

	// Find the exposed functions available in each class, making sure to not list shared functions from a parent class more than once
	for (UEditorUtilityObject* Utility : Utilities)
	{
		UClass* Class = Cast<UObject>(Utility)->GetClass();

		if (ProcessedClasses.Contains(Class))
		{
			continue;
		}

		GetUtilityFunctions(Utility, OutFunctions, false);

		for (UClass* ParentClass = Class; ParentClass != UObject::StaticClass(); ParentClass = ParentClass->GetSuperClass())
		{
			ProcessedClasses.Add(ParentClass);
		}
	}

	if (bDoSort)
	{
		// Sort the functions by name
		OutFunctions.Sort([](const FFunctionAndUtil& A, const FFunctionAndUtil& B)
			{
				return A.Function->GetName() < B.Function->GetName();
			});
	}
}

void FCustomEditorHotkeysBlutilityExtensions::ExecuteUtilityFunctionByName(FName FunctionName)
{
	if (!GUnrealEd)
		return;

	if (UEditorActorSubsystem* EditorActorSubsystem = GUnrealEd->GetEditorSubsystem<UEditorActorSubsystem>())
	{
		const TArray<AActor*> SelectedActors = EditorActorSubsystem->GetSelectedLevelActors();
		const TArray<UEditorUtilityObject*> SupportedUtils = GetUtilitiesSupportedBySelectedActors(SelectedActors);

		TArray<FFunctionAndUtil> Functions;
		GetUtilityFunctions(SupportedUtils, Functions);

		for (const FFunctionAndUtil& FunctionAndUtil : Functions)
		{
			if (FunctionAndUtil.Function)
			{
				FString CommandTitle = FunctionAndUtil.Function->GetDisplayNameText().ToString();
				CommandTitle.RemoveSpacesInline();

				if (FunctionName == FName(*CommandTitle))
				{
					ExecuteUtilityFunction(FunctionAndUtil);
					return;
				}
			}
		}
	}
}

void FCustomEditorHotkeysBlutilityExtensions::ExecuteUtilityFunction(const FFunctionAndUtil& FunctionAndUtil)
{	
	// We dont run this on the CDO, as bad things could occur!
	UObject* TempObject = NewObject<
		UObject>(GetTransientPackage(), Cast<UObject>(FunctionAndUtil.Util)->GetClass());
	TempObject->AddToRoot();
	// Some Blutility actions might run GC so the TempObject needs to be rooted to avoid getting destroyed

	if (FunctionAndUtil.Function->NumParms > 0)
	{
		// Create a parameter struct and fill in defaults
		TSharedRef<FStructOnScope> FuncParams = MakeShared<FStructOnScope>(FunctionAndUtil.Function);
		for (TFieldIterator<FProperty> It(FunctionAndUtil.Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			FString Defaults;
			if (UEdGraphSchema_K2::FindFunctionParameterDefaultValue(FunctionAndUtil.Function, *It, Defaults))
			{
				It->ImportText(*Defaults, It->ContainerPtrToValuePtr<uint8>(FuncParams->GetStructMemory()),
					PPF_None, nullptr);
			}
		}

		// pop up a dialog to input params to the function
		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(FunctionAndUtil.Function->GetDisplayNameText())
			.ClientSize(FVector2D(400, 200))
			.SupportsMinimize(false)
			.SupportsMaximize(false);

		TSharedPtr<SFunctionParamDialog> Dialog;
		Window->SetContent(
			SAssignNew(Dialog, SFunctionParamDialog, Window, FuncParams)
			.OkButtonText(LOCTEXT("OKButton", "OK"))
			.OkButtonTooltipText(FunctionAndUtil.Function->GetToolTipText()));

		GEditor->EditorAddModalWindow(Window);

		if (Dialog->bOKPressed)
		{
			FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BlutilityAction", "Blutility Action"));
			FEditorScriptExecutionGuard ScriptGuard;
			TempObject->ProcessEvent(FunctionAndUtil.Function, FuncParams->GetStructMemory());
		}
	}
	else
	{
		FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BlutilityAction", "Blutility Action"));
		FEditorScriptExecutionGuard ScriptGuard;
		TempObject->ProcessEvent(FunctionAndUtil.Function, nullptr);
	}

	TempObject->RemoveFromRoot();
}

#undef LOCTEXT_NAMESPACE
