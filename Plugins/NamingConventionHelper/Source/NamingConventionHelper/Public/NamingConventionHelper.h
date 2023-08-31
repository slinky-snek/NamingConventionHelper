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

	//static void LoadNamingConventions();
	static TMap<FString, FString> ReadCSVFile(const FString& FilePath);
	static TSharedRef<FExtender> ExtendAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
	static void AssetExtenderFunc(FMenuBuilder& MenuBuilder, const TArray<FAssetData> SelectedAssets);
	static void RenameAssets(const TArray<FAssetData> SelectedAssets);
	static FString GetAssetPrefix(const FTopLevelAssetPath ClassPath);
	static bool DoesPrefixExistInName(const FAssetData& AssetData);

private:
	static TMap<FString, FString>& GetNamingConventions();
};
