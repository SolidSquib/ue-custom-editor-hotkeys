// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomEditorHotkeysStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr<FSlateStyleSet> FCustomEditorHotkeysStyle::StyleInstance = nullptr;

void FCustomEditorHotkeysStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FCustomEditorHotkeysStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FCustomEditorHotkeysStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("CustomEditorHotkeysStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

TSharedRef< FSlateStyleSet > FCustomEditorHotkeysStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("CustomEditorHotkeysStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("CustomEditorHotkeys")->GetBaseDir() / TEXT("Resources"));

	Style->Set("CustomEditorHotkeys.PluginAction", new IMAGE_BRUSH(TEXT("HotkeysRefresh"), Icon40x40));
	return Style;
}

#undef IMAGE_BRUSH

void FCustomEditorHotkeysStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FCustomEditorHotkeysStyle::Get()
{
	return *StyleInstance;
}
