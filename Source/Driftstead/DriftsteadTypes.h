#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DriftsteadTypes.generated.h"

UENUM(BlueprintType)
enum class EDriftItemRarity : uint8
{
    Common,
    Uncommon,
    Rare,
    Epic
};

UENUM(BlueprintType)
enum class EHookState : uint8
{
    Idle,
    Charging,
    Flying,
    Attached,
    Returning,
    Cooldown
};

UENUM(BlueprintType)
enum class EInventoryAddResult : uint8
{
    Placed,
    Stacked,
    RecoveryBasket,
    InvalidItem
};

UENUM(BlueprintType)
enum class EFacilityType : uint8
{
    Workbench,
    RainBarrel,
    FarmPlot,
    ChickenCoop,
    CollectionNet,
    StorageLocker,
    WindTurbine,
    AutoCrane,
    TradingDock,
    Desalinator,
    Lighthouse
};

UENUM(BlueprintType)
enum class EDriftsteadQuestStep : uint8
{
    Move,
    ChargeHook,
    SalvageItem,
    OpenInventory,
    RotateItem,
    OpenBarrel,
    UpgradeLevel2,
    HarvestCrop,
    UpgradeLevel4,
    ReachSecondFloor,
    Complete
};

USTRUCT(BlueprintType)
struct FDriftItemDefinition : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ItemId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FIntPoint Footprint = FIntPoint(1, 1);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Weight = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 StackLimit = 9;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EDriftItemRarity Rarity = EDriftItemRarity::Common;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BaseValue = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector WorldCollisionSize = FVector(60.0, 60.0, 60.0);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName MeshStyle = TEXT("Crate");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor PrimaryColor = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor SecondaryColor = FLinearColor::Gray;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float SpawnWeight = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector2D DriftSpeedRange = FVector2D(35.0, 65.0);
};

USTRUCT(BlueprintType)
struct FInventoryEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid InstanceId = FGuid();
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ItemId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FIntPoint GridPosition = FIntPoint::ZeroValue;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Quantity = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bRotated = false;

};

USTRUCT(BlueprintType)
struct FFacilitySaveState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) EFacilityType FacilityType = EFacilityType::Workbench;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 FloorIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 StoredOutput = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bProducing = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 StorageColumns = 8;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 StorageRows = 4;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FInventoryEntry> StorageEntries;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FInventoryEntry> StorageRecoveryBasket;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, int32> StorageResources;
};

USTRUCT(BlueprintType)
struct FRaftLevelDefinition : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Level = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText ThemeName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FIntPoint FirstFloor = FIntPoint(4, 4);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FIntPoint SecondFloor = FIntPoint::ZeroValue;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FIntPoint ThirdFloor = FIntPoint::ZeroValue;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<EFacilityType> Facilities;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, int32> UpgradeCost;
};

class DRIFTSTEAD_API FDriftsteadItemCatalog
{
public:
    static const TArray<FDriftItemDefinition>& GetDefinitions();
    static const FDriftItemDefinition* Find(FName ItemId);
};

class DRIFTSTEAD_API FRaftProgressionCatalog
{
public:
    static const TArray<FRaftLevelDefinition>& GetDefinitions();
    static const FRaftLevelDefinition* Find(int32 Level);
};
