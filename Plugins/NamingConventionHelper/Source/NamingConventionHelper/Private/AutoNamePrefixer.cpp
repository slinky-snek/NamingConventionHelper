#include "AutoNamePrefixer.h"
#include "Editor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/Factory.h"
#include "EditorScriptingUtilities/Public/EditorAssetLibrary.h"

void UAutoNamePrefixer::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().OnAssetAdded().AddUObject(this, &UAutoNamePrefixer::OnAssetAdded);
	FEditorDelegates::OnNewAssetCreated.AddUObject(this, &UAutoNamePrefixer::OnAssetCreated);
}

void UAutoNamePrefixer::Deinitialize()
{
	Super::Deinitialize();
}

void UAutoNamePrefixer::OnAssetAdded(const FAssetData& AssetData)
{
	if(IsEditorFullyLoaded)
		RenameAsset(AssetData);
}

void UAutoNamePrefixer::OnAssetCreated(UFactory* Factory)
{
	if(!IsEditorFullyLoaded)
		IsEditorFullyLoaded = true;
}

void UAutoNamePrefixer::RenameAsset(const FAssetData& AssetData)
{
	if(AssetData.GetAsset() && AssetData.AssetClassPath.GetAssetName() != FName("BlueprintGeneratedClass"))
	{
		const FString PackageName = AssetData.PackagePath.ToString();
		const FString AssetName = AssetData.AssetName.ToString();
		const FString OldAssetPath = FString::Printf(TEXT("%s/%s"), *PackageName, *AssetName);
		const FString NewName = GetAssetPrefix(AssetData.AssetClassPath).Append(AssetName);
		const FString NewAssetPath = FString::Printf(TEXT("%s/%s"), *PackageName, *NewName);
		if(!DoesPrefixExistInName(AssetData))
			if(UEditorAssetLibrary::RenameAsset(OldAssetPath, NewAssetPath)) //loops forever because renaming causes the OnAssetAdded event to fire
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10, FColor::Red, TEXT("Successfully renamed asset"));
			else
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10, FColor::Red, TEXT("Failed to rename asset"));
	}
}

FString UAutoNamePrefixer::GetAssetPrefix(const FTopLevelAssetPath ClassPath)
{
	FString Prefix = "BP_";
	return Prefix;
}

bool UAutoNamePrefixer::DoesPrefixExistInName(const FAssetData& AssetData)
{
	TArray<FString> AssetNameParts;
	const FString AssetName = AssetData.AssetName.ToString();
	const FString Delimiter = "_";
	const FString Prefix = GetAssetPrefix(AssetData.AssetClassPath);
	AssetName.ParseIntoArray(AssetNameParts, *Delimiter);
	FString CurrentPrefix;
	if(AssetNameParts.Num() > 0)
		CurrentPrefix = AssetNameParts[0].Append("_");
	if(Prefix == CurrentPrefix) //why does this always return false???
		return true;
	return false;
}
