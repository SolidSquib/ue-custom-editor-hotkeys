#pragma once

#include "IStructureDetailsView.h"
#include "EditorStyleSet.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "FCustomEditorHotkeysModule"

/** Dialog widget used to display function properties */
class SFunctionParamDialog : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SFunctionParamDialog) {}

	/** Text to display on the "OK" button */
	SLATE_ARGUMENT(FText, OkButtonText)

		/** Tooltip text for the "OK" button */
		SLATE_ARGUMENT(FText, OkButtonTooltipText)

		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, TWeakPtr<SWindow> InParentWindow, TSharedRef<FStructOnScope> InStructOnScope)
	{
		bOKPressed = false;

		// Initialize details view
		FDetailsViewArgs DetailsViewArgs;
		{
			DetailsViewArgs.bAllowSearch = false;
			DetailsViewArgs.bHideSelectionTip = true;
			DetailsViewArgs.bLockable = false;
			DetailsViewArgs.bSearchInitialKeyFocus = true;
			DetailsViewArgs.bUpdatesFromSelection = false;
			DetailsViewArgs.bShowOptions = false;
			DetailsViewArgs.bShowModifiedPropertiesOption = false;
			DetailsViewArgs.bShowObjectLabel = false;
			DetailsViewArgs.bForceHiddenPropertyVisibility = true;
			DetailsViewArgs.bShowScrollBar = false;
		}

		FStructureDetailsViewArgs StructureViewArgs;
		{
			StructureViewArgs.bShowObjects = true;
			StructureViewArgs.bShowAssets = true;
			StructureViewArgs.bShowClasses = true;
			StructureViewArgs.bShowInterfaces = true;
		}

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedRef<IStructureDetailsView> StructureDetailsView = PropertyEditorModule.CreateStructureDetailView(DetailsViewArgs, StructureViewArgs, InStructOnScope);

		StructureDetailsView->GetDetailsView()->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& InPropertyAndParent)
			{
				return InPropertyAndParent.Property.HasAnyPropertyFlags(CPF_Parm);
			}));

		StructureDetailsView->GetDetailsView()->ForceRefresh();

		ChildSlot
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
			[
				StructureDetailsView->GetWidget().ToSharedRef()
			]
			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.Padding(2.0f)
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
			.ForegroundColor(FLinearColor::White)
			.ContentPadding(FMargin(6, 2))
			.OnClicked_Lambda([this, InParentWindow, InArgs]()
				{
					if (InParentWindow.IsValid())
					{
						InParentWindow.Pin()->RequestDestroyWindow();
					}
					bOKPressed = true;
					return FReply::Handled();
				})
			.ToolTipText(InArgs._OkButtonTooltipText)
					[
						SNew(STextBlock)
						.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
					.Text(InArgs._OkButtonText)
					]
			]
		+ SHorizontalBox::Slot()
			.Padding(2.0f)
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FEditorStyle::Get(), "FlatButton")
			.ForegroundColor(FLinearColor::White)
			.ContentPadding(FMargin(6, 2))
			.OnClicked_Lambda([InParentWindow]()
				{
					if (InParentWindow.IsValid())
					{
						InParentWindow.Pin()->RequestDestroyWindow();
					}
					return FReply::Handled();
				})
			[
				SNew(STextBlock)
				.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
					.Text(LOCTEXT("Cancel", "Cancel"))
			]
			]
			]
			]
			];
	}

	bool bOKPressed;
};

#undef LOCTEXT_NAMESPACE