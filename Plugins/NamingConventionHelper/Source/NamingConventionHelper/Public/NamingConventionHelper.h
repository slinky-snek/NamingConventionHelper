#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FContentBrowserModule;

class FNamingConventionHelperModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	/** IModuleInterface implementation */
	
	// Context menu extension functions
	static TSharedRef<FExtender> ExtendAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
	static void AssetExtenderFunc(FMenuBuilder& MenuBuilder, const TArray<FAssetData> SelectedAssets);

	// Public rename/undo functions
	static void ExecuteAssetRenaming(const TArray<FAssetData>& SelectedAssets);
	static void UndoAssetRenaming(const TArray<FAssetData>& SelectedAssets);

	// Helpers to get information about asset classes
	static void PrintClassNames(const TArray<FAssetData>& SelectedAssets);

private:
	static TMap<FString, FString>& GetNamingConventions();
	static TMap<FString, FString> ReadCSVFile(const FString& FilePath);
	static void RenameAsset(const FAssetData& SelectedAsset, const FString& NewAssetName);
	static FString GetAssetPrefix(const FTopLevelAssetPath ClassPath);
	static bool DoesPrefixExistInName(const FAssetData& AssetData);
};
