#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DriftsteadTypes.h"
#include "DriftsteadGameMode.generated.h"

class ARaftManager;
class ADriftItemSpawner;
class UInventoryComponent;

UCLASS()
class DRIFTSTEAD_API ADriftsteadGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADriftsteadGameMode();
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintPure, Category="Raft") int32 GetRaftLevel() const;
    UFUNCTION(BlueprintPure, Category="Raft") int32 GetMaximumFloor() const;
    UFUNCTION(BlueprintCallable, Category="Raft") bool TryUpgradeRaft(UInventoryComponent* Inventory);
    UFUNCTION(BlueprintCallable, Category="Raft") void DebugChangeRaftLevel(int32 Delta);
    UFUNCTION(BlueprintCallable, Category="Raft") void SetRaftLevelFromSave(int32 Level);
    UFUNCTION(BlueprintCallable, Category="Raft") void SetViewedFloor(int32 Floor);
    UFUNCTION(BlueprintCallable, Category="Debug") void SpawnDebugItems();
    TArray<FFacilitySaveState> CaptureFacilityStates() const;
    void RestoreFacilityStates(const TArray<FFacilitySaveState>& States);
    void ResetOpeningSupplies();
    FString GetUpgradeSummary(UInventoryComponent* Inventory) const;

private:
    void BuildRuntimeWorld();
    void ApplyInventoryCapacityForLevel();
    void RunSmokeStep();
    void RunOpeningStep();
    void BeginMenuValidation();
    void RunCaptureStep();
    void CaptureFrame(const FString& Filename);
    UPROPERTY() TObjectPtr<ARaftManager> RaftManager;
    UPROPERTY() TObjectPtr<ADriftItemSpawner> ItemSpawner;
    UPROPERTY() TObjectPtr<AActor> SmokeTarget;
    UPROPERTY() TObjectPtr<AActor> SmokeDecoy;
    int32 SmokeStep = 0;
    int32 SmokeWaitTicks = 0;
    int32 SmokeInitialDriftwood = 0;
    int32 SmokeInitialPassiveOutput = -1;
    int32 SmokeProductionWaitTicks = 0;
    bool bSmokeFailed = false;
    FTimerHandle SmokeTimer;
    int32 CaptureStep = 0;
    int32 CaptureHookWaitTicks = 0;
    FTimerHandle CaptureTimer;
    int32 OpeningStep = 0;
    int32 OpeningWait = 0;
    float OpeningGrowthRemaining = 0;
    bool bOpeningFailed = false;
    FTimerHandle OpeningTimer;
};
