#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DriftsteadTypes.h"
#include "DriftsteadSaveGame.generated.h"

UCLASS()
class DRIFTSTEAD_API UDriftsteadSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentSaveVersion = 2;
    static bool IsSupportedVersion(int32 Version) { return Version >= 1 && Version <= CurrentSaveVersion; }

    UPROPERTY() int32 SaveVersion = CurrentSaveVersion;
    UPROPERTY() bool bShowcaseMode = false;
    UPROPERTY() int32 RaftLevel = 1;
    UPROPERTY() int32 CurrentFloor = 0;
    UPROPERTY() FVector PlayerLocation = FVector::ZeroVector;
    UPROPERTY() int32 InventoryColumns = 6;
    UPROPERTY() int32 InventoryRows = 4;
    UPROPERTY() TArray<FInventoryEntry> InventoryEntries;
    UPROPERTY() TArray<FInventoryEntry> RecoveryBasket;
    UPROPERTY() TMap<FName, int32> Resources;
    UPROPERTY() TArray<FName> UnlockedFacilities;
    UPROPERTY() TMap<FName, int32> FacilityProductionState;
    UPROPERTY() TArray<FFacilitySaveState> FacilityStates;
    UPROPERTY() EDriftsteadQuestStep CurrentQuest = EDriftsteadQuestStep::Move;
    UPROPERTY() bool bTutorialComplete = false;
};
