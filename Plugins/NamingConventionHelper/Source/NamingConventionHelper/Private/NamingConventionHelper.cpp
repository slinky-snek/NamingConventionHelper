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

			// Now CSVValues contains the values of each column in the CSV line
			for (int index = 0; index < CSVValues.Num() - 1; index += 2)
			{
				FString Key = CSVValues[index];
				FString Value = CSVValues[index + 1];
				NamingConventions.Add(Key, Value);
				UE_LOG(LogTemp, Warning, TEXT("Naming Conventions: %s, %s"), *Key, *Value);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read CSV file from path: %s"), *FilePath);
	}
	return NamingConventions;
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
	MenuBuilder.BeginSection("Smart Rename Actions", LOCTEXT("Smart Rename Actions", "Smart Rename Actions"));
	{
		MenuBuilder.AddMenuEntry(LOCTEXT("Execute Smart Rename", "Execute Smart Rename"),LOCTEXT("Rename selected files with asset appropriate prefix.", "Rename selected files with asset appropriate prefix."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Linter.Toolbar.Icon"),
			FUIAction(FExecuteAction::CreateLambda([SelectedAssets]()
			{
				RenameAssets(SelectedAssets);
			})),NAME_None, EUserInterfaceActionType::Button);
	}
	MenuBuilder.EndSection();
}

void FNamingConventionHelperModule::RenameAssets(const TArray<FAssetData> SelectedAssets)
{
	for(const FAssetData& AssetData : SelectedAssets)
	{
		if(AssetData.GetAsset() && AssetData.AssetClassPath.GetAssetName() != FName("BlueprintGeneratedClass"))
		{
			const FString PackageName = AssetData.PackagePath.ToString();
			const FString AssetName = AssetData.AssetName.ToString();
			const FString OldAssetPath = FString::Printf(TEXT("%s/%s"), *PackageName, *AssetName);
			const FString NewName = GetAssetPrefix(AssetData.AssetClassPath).Append(AssetName);
			const FString NewAssetPath = FString::Printf(TEXT("%s/%s"), *PackageName, *NewName);
			if(!DoesPrefixExistInName(AssetData))
				if(UEditorAssetLibrary::RenameAsset(OldAssetPath, NewAssetPath))
				{
					UE_LOG(LogEngine, Warning, TEXT("Successfully renamed %s to %s"), *AssetName, *NewName);
				}
				else
				{
					UE_LOG(LogEngine, Warning, TEXT("Failed to rename %s"), *AssetName);
				}
		}
	}
}

FString FNamingConventionHelperModule::GetAssetPrefix(const FTopLevelAssetPath ClassPath)
{
	UE_LOG(LogTemp, Warning, TEXT("Class Path: %s"), *ClassPath.GetAssetName().ToString());
	TMap<FString, FString>& NamingConventions = GetNamingConventions();
	const FString ClassName = ClassPath.GetAssetName().ToString();
	FString Prefix = "BP_";// = NamingConventions.Find(ClassName);
	return Prefix;
}

bool FNamingConventionHelperModule::DoesPrefixExistInName(const FAssetData& AssetData)
{
	TArray<FString> AssetNameParts;
	const FString AssetName = AssetData.AssetName.ToString();
	const FString Delimiter = "_";
	const FString Prefix = GetAssetPrefix(AssetData.AssetClassPath);
	AssetName.ParseIntoArray(AssetNameParts, *Delimiter);
	FString CurrentPrefix;
	if(AssetNameParts.Num() > 0)
		CurrentPrefix = AssetNameParts[0].Append("_");
	if(Prefix == CurrentPrefix)
		return true;
	return false;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNamingConventionHelperModule, NamingConventionHelper)