#include "NamingConventionHelper.h"
#include "ContentBrowserModule.h"
#include "EditorAssetLibrary.h"
#include "Logging/LogCategory.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FNamingConventionHelperModule"

void FNamingConventionHelperModule::StartupModule()
{
	GetNamingConventions();
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuAssetExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	CBMenuAssetExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateStatic(&ExtendAssetSelectionMenu));
}

void FNamingConventionHelperModule::ShutdownModule()
{
}

TSharedRef<FExtender> FNamingConventionHelperModule::ExtendAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddMenuExtension(
		"CommonAssetActions",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateStatic(&AssetExtenderFunc, SelectedAssets));
	return Extender;
}

void FNamingConventionHelperModule::AssetExtenderFunc(FMenuBuilder& MenuBuilder, const TArray<FAssetData> SelectedAssets)
{
	MenuBuilder.BeginSection("Smart Rename Actions", LOCTEXT("SMART_RENAME_MENU_HEADER", "Smart Rename Actions"));
	{
		MenuBuilder.AddMenuEntry(LOCTEXT("EXECUTE_RENAME", "Execute Smart Rename"),LOCTEXT("RENAME_TOOLTIP", "Rename selected files with asset appropriate prefix."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Linter.Toolbar.Icon"),
			FUIAction(FExecuteAction::CreateLambda([SelectedAssets]()
			{
				ExecuteAssetRenaming(SelectedAssets);
			})),NAME_None, EUserInterfaceActionType::Button);

		MenuBuilder.AddMenuEntry(LOCTEXT("EXECUTE_UNDO_RENAME", "Undo Asset Renaming"),LOCTEXT("UNDO_RENAME_TOOLTIP", "Undo smart renaming of selected files."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Linter.Toolbar.Icon"),
			FUIAction(FExecuteAction::CreateLambda([SelectedAssets]()
			{
				UndoAssetRenaming(SelectedAssets);
			})),NAME_None, EUserInterfaceActionType::Button);
	}
	MenuBuilder.EndSection();
}

void FNamingConventionHelperModule::ExecuteAssetRenaming(const TArray<FAssetData>& SelectedAssets)
{
	FScopedSlowTask RenameAssetsTask(SelectedAssets.Num(), LOCTEXT("RENAME_ASSETS", "Renaming Assets"));
	RenameAssetsTask.MakeDialog(true);
	for(const FAssetData& AssetData : SelectedAssets)
	{
		RenameAssetsTask.EnterProgressFrame();
		if(AssetData.GetAsset() && AssetData.AssetClassPath.GetAssetName() != FName("BlueprintGeneratedClass"))
		{
			const FString AssetName = AssetData.AssetName.ToString();
			const FString NewAssetName = GetAssetPrefix(AssetData.AssetClassPath).Append(AssetName);
			if(!DoesPrefixExistInName(AssetData))
			{
				RenameAsset(AssetData, NewAssetName);
			}
		}
		else
			UE_LOG(LogEngine, Warning, TEXT("Renaming failed. Asset doesn't exist."));
	}
}

void FNamingConventionHelperModule::UndoAssetRenaming(const TArray<FAssetData>& SelectedAssets)
{
	FScopedSlowTask RenameAssetsTask(SelectedAssets.Num(), LOCTEXT("UNDO_ASSET_RENAME", "Undo Asset Renaming"));
	RenameAssetsTask.MakeDialog(true);
	for(const FAssetData& AssetData : SelectedAssets)
	{
		RenameAssetsTask.EnterProgressFrame();
		if(AssetData.GetAsset() && AssetData.AssetClassPath.GetAssetName() != FName("BlueprintGeneratedClass"))
		{
			if(DoesPrefixExistInName(AssetData))
			{
				const FString AssetName = AssetData.AssetName.ToString();
				const FString Prefix = GetAssetPrefix(AssetData.AssetClassPath);
				const FString NewAssetName = AssetName.RightChop(Prefix.Len());
				RenameAsset(AssetData, NewAssetName);
			}
		}
	}
}

TMap<FString, FString>& FNamingConventionHelperModule::GetNamingConventions()
{
	static TMap<FString, FString> NamingConventions;
	static bool bInitialized = false;
	// Initialize the map only once when accessed for the first time
	if (!bInitialized)
	{
		FString ConfigPath = FPaths::ProjectConfigDir();
		ConfigPath.Append("NamingConventions.csv");
		NamingConventions = ReadCSVFile(ConfigPath);
		bInitialized = true;
	}
	return NamingConventions;
}

TMap<FString, FString> FNamingConventionHelperModule::ReadCSVFile(const FString& FilePath)
{
	TArray<FString> CSVLines;
	TMap<FString, FString> NamingConventions;
	// Read the CSV file into an array of lines
	if (FFileHelper::LoadFileToStringArray(CSVLines, *FilePath))
	{
		for (const FString& CSVLine : CSVLines)
		{
			TArray<FString> CSVValues;
			CSVLine.ParseIntoArray(CSVValues, TEXT(","), true);
			const uint32 Size = CSVValues.Num() - 1;
			// Now CSVValues contains the values of each column in the CSV line
			for (uint32 Index = 0; Index < Size; Index += 2)
			{
				const FString Key = CSVValues[Index];
				const FString Value = CSVValues[Index + 1];
				NamingConventions.Add(Key, Value);
				UE_LOG(LogTemp, Warning, TEXT("Naming Conventions: %s, %s"), *Key, *Value);
			}
		}
	}
	else
		UE_LOG(LogTemp, Error, TEXT("Failed to read CSV file from path: %s"), *FilePath);
	return NamingConventions;
}

void FNamingConventionHelperModule::RenameAsset(const FAssetData& SelectedAsset, const FString& NewAssetName)
{
	if(SelectedAsset.GetAsset() && SelectedAsset.AssetClassPath.GetAssetName() != FName("BlueprintGeneratedClass"))
	{
		const FString PackageName = SelectedAsset.PackagePath.ToString();
		const FString AssetName = SelectedAsset.AssetName.ToString();
		const FString OldAssetPath = FString::Printf(TEXT("%s/%s"), *PackageName, *AssetName);
		const FString NewAssetPath = FString::Printf(TEXT("%s/%s"), *PackageName, *NewAssetName);
		if(UEditorAssetLibrary::RenameAsset(OldAssetPath, NewAssetPath))
		{
			UE_LOG(LogEngine, Warning, TEXT("Successfully renamed %s to %s"), *AssetName, *NewAssetName);
		}
		else
			UE_LOG(LogEngine, Warning, TEXT("Failed to rename %s"), *AssetName);
	}
	else
		UE_LOG(LogEngine, Warning, TEXT("Renaming failed. Asset doesn't exist."));
}

FString FNamingConventionHelperModule::GetAssetPrefix(const FTopLevelAssetPath ClassPath)
{
	UE_LOG(LogTemp, Warning, TEXT("Class Path: %s"), *ClassPath.GetAssetName().ToString());
	TMap<FString, FString>& NamingConventions = GetNamingConventions();
	const FString ClassName = ClassPath.GetAssetName().ToString();
	FString Prefix = "";
	if(NamingConventions.Contains(ClassName))
		Prefix = NamingConventions[ClassName];
	return Prefix;
}

bool FNamingConventionHelperModule::DoesPrefixExistInName(const FAssetData& AssetData)
{
	const FString AssetName = AssetData.AssetName.ToString();
	const FString Prefix = GetAssetPrefix(AssetData.AssetClassPath);
	uint32 LeftChopIndex = AssetName.Len() - Prefix.Len();
	const FString LeftChopPrefix = AssetName.LeftChop(LeftChopIndex);
	if(Prefix.Equals(LeftChopPrefix))
		return true;
	return false;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNamingConventionHelperModule, NamingConventionHelper)