#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DriftsteadGameInstance.generated.h"

UCLASS()
class DRIFTSTEAD_API UDriftsteadGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Save") bool SaveCurrentGame();
    UFUNCTION(BlueprintCallable, Category="Save") bool LoadCurrentGame();
    UFUNCTION(BlueprintCallable, Category="Save") void StartNewGame(bool bInShowcaseMode);
    UFUNCTION(BlueprintCallable, Category="Save") void ResetAllSaves();
    UFUNCTION(BlueprintPure, Category="Save") bool IsShowcaseMode() const { return bShowcaseMode; }
    void SetAutomationMode(bool bEnabled) { bAutomationMode = bEnabled; }
    void DeleteAutomationSave();

private:
    FString GetActiveSlot() const;
    bool bShowcaseMode = false;
    bool bAutomationMode = false;
};
