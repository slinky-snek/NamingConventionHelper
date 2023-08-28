#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AutoNamePrefixer.generated.h"

class UFactory;

UCLASS()
class NAMINGCONVENTIONHELPER_API UAutoNamePrefixer : public UEditorSubsystem
{
	GENERATED_BODY()

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void OnAssetCreated(UFactory* Factory);
	void OnAssetAdded(const FAssetData& AssetData);
	static void RenameAsset(const FAssetData& AssetData);
	static FString GetAssetPrefix(const FTopLevelAssetPath ClassPath);
	static bool DoesPrefixExistInName(const FAssetData& AssetData);

private:
	inline static bool IsEditorFullyLoaded = false;
	inline static bool RenamingInProgress = false;
};
