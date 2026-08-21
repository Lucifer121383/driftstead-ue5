#include "DriftsteadTypes.h"

#define LOCTEXT_NAMESPACE "DriftsteadCatalog"

namespace
{
FDriftItemDefinition MakeItem(
    const TCHAR* Id,
    const FText& Name,
    const FText& Description,
    FIntPoint Footprint,
    int32 Weight,
    int32 StackLimit,
    EDriftItemRarity Rarity,
    int32 Value,
    FVector Collision,
    const TCHAR* MeshStyle,
    FLinearColor Primary,
    FLinearColor Secondary,
    float SpawnWeight,
    FVector2D DriftSpeed)
{
    FDriftItemDefinition Item;
    Item.ItemId = FName(Id);
    Item.DisplayName = Name;
    Item.Description = Description;
    Item.Footprint = Footprint;
    Item.Weight = Weight;
    Item.StackLimit = StackLimit;
    Item.Rarity = Rarity;
    Item.BaseValue = Value;
    Item.WorldCollisionSize = Collision;
    Item.MeshStyle = FName(MeshStyle);
    Item.PrimaryColor = Primary;
    Item.SecondaryColor = Secondary;
    Item.SpawnWeight = SpawnWeight;
    Item.DriftSpeedRange = DriftSpeed;
    return Item;
}

FRaftLevelDefinition MakeRaft(
    int32 Level,
    const FText& Theme,
    FIntPoint First,
    FIntPoint Second,
    FIntPoint Third,
    std::initializer_list<EFacilityType> Facilities)
{
    FRaftLevelDefinition Definition;
    Definition.Level = Level;
    Definition.ThemeName = Theme;
    Definition.FirstFloor = First;
    Definition.SecondFloor = Second;
    Definition.ThirdFloor = Third;
    Definition.Facilities = Facilities;
    if (Level > 1)
    {
        Definition.UpgradeCost.Add(TEXT("Wood"), 4 + Level * 3);
        Definition.UpgradeCost.Add(TEXT("Rope"), 1 + Level);
        if (Level >= 4) Definition.UpgradeCost.Add(TEXT("Metal"), Level - 2);
        if (Level >= 7) Definition.UpgradeCost.Add(TEXT("Parts"), Level - 5);
    }
    return Definition;
}
}

const TArray<FDriftItemDefinition>& FDriftsteadItemCatalog::GetDefinitions()
{
    static const TArray<FDriftItemDefinition> Items = {
        MakeItem(TEXT("Driftwood"), LOCTEXT("DriftwoodName", "漂流木捆"), LOCTEXT("DriftwoodDesc", "风化木材，可用于扩建木筏。"), FIntPoint(2, 1), 2, 8, EDriftItemRarity::Common, 2, FVector(90, 45, 42), TEXT("Bundle"), FLinearColor(0.55f, 0.25f, 0.08f), FLinearColor(0.82f, 0.55f, 0.22f), 10.0f, FVector2D(38, 62)),
        MakeItem(TEXT("Rope"), LOCTEXT("RopeName", "绳索卷"), LOCTEXT("RopeDesc", "建造和修理用的耐盐绳索。"), FIntPoint(1, 1), 1, 12, EDriftItemRarity::Common, 2, FVector(45), TEXT("Coil"), FLinearColor(0.68f, 0.46f, 0.22f), FLinearColor(0.95f, 0.77f, 0.42f), 9.0f, FVector2D(40, 68)),
        MakeItem(TEXT("ScrapMetal"), LOCTEXT("MetalName", "废铁块"), LOCTEXT("MetalDesc", "制作工具和结构升级所需的金属。"), FIntPoint(1, 1), 2, 10, EDriftItemRarity::Common, 3, FVector(48), TEXT("Metal"), FLinearColor(0.22f, 0.32f, 0.38f), FLinearColor(0.58f, 0.72f, 0.75f), 8.0f, FVector2D(34, 55)),
        MakeItem(TEXT("Cloth"), LOCTEXT("ClothName", "布料卷"), LOCTEXT("ClothDesc", "可用于床铺、风帆和温室。"), FIntPoint(2, 1), 2, 8, EDriftItemRarity::Common, 3, FVector(86, 44, 44), TEXT("Roll"), FLinearColor(0.92f, 0.62f, 0.34f), FLinearColor(0.96f, 0.86f, 0.68f), 7.0f, FVector2D(40, 66)),
        MakeItem(TEXT("SeedCrate"), LOCTEXT("SeedsName", "种子箱"), LOCTEXT("SeedsDesc", "装有耐涝作物种子的珍贵木箱。"), FIntPoint(2, 1), 2, 6, EDriftItemRarity::Uncommon, 5, FVector(86, 46, 48), TEXT("Crate"), FLinearColor(0.27f, 0.65f, 0.25f), FLinearColor(0.78f, 0.89f, 0.35f), 5.5f, FVector2D(36, 58)),
        MakeItem(TEXT("FoodCrate"), LOCTEXT("FoodName", "食物箱"), LOCTEXT("FoodDesc", "船员与动物所需的耐储食物。"), FIntPoint(2, 2), 4, 4, EDriftItemRarity::Uncommon, 7, FVector(88, 88, 62), TEXT("Crate"), FLinearColor(0.80f, 0.32f, 0.16f), FLinearColor(0.95f, 0.78f, 0.28f), 4.5f, FVector2D(30, 48)),
        MakeItem(TEXT("SealedBarrel"), LOCTEXT("BarrelName", "密封木桶"), LOCTEXT("BarrelDesc", "可在工作台打开，获得混合物资。"), FIntPoint(2, 2), 5, 3, EDriftItemRarity::Uncommon, 9, FVector(80, 80, 90), TEXT("Barrel"), FLinearColor(0.18f, 0.42f, 0.58f), FLinearColor(0.82f, 0.58f, 0.19f), 3.5f, FVector2D(28, 45)),
        MakeItem(TEXT("MachineryCrate"), LOCTEXT("MachineryName", "机械箱"), LOCTEXT("MachineryDesc", "高级动力设施所需的沉重零件。"), FIntPoint(2, 2), 6, 3, EDriftItemRarity::Rare, 12, FVector(90, 90, 72), TEXT("Machinery"), FLinearColor(0.19f, 0.26f, 0.31f), FLinearColor(0.95f, 0.57f, 0.12f), 2.3f, FVector2D(24, 40)),
        MakeItem(TEXT("Electronics"), LOCTEXT("ElectronicsName", "电子元件盒"), LOCTEXT("ElectronicsDesc", "导航与自动化所需的干燥电路。"), FIntPoint(1, 1), 1, 8, EDriftItemRarity::Rare, 14, FVector(48), TEXT("Electronics"), FLinearColor(0.18f, 0.72f, 0.78f), FLinearColor(0.72f, 0.95f, 1.0f), 2.0f, FVector2D(38, 60)),
        MakeItem(TEXT("AnimalCrate"), LOCTEXT("AnimalName", "动物运输箱"), LOCTEXT("AnimalDesc", "安全装载着鸡或幼年山羊。"), FIntPoint(2, 2), 5, 2, EDriftItemRarity::Epic, 18, FVector(96, 96, 82), TEXT("Animal"), FLinearColor(0.56f, 0.24f, 0.62f), FLinearColor(0.96f, 0.70f, 0.95f), 1.2f, FVector2D(25, 42))
    };
    return Items;
}

const FDriftItemDefinition* FDriftsteadItemCatalog::Find(FName ItemId)
{
    return GetDefinitions().FindByPredicate([ItemId](const FDriftItemDefinition& Item) { return Item.ItemId == ItemId; });
}

const TArray<FRaftLevelDefinition>& FRaftProgressionCatalog::GetDefinitions()
{
    static const TArray<FRaftLevelDefinition> Levels = {
        MakeRaft(1, LOCTEXT("Raft1", "漂流木台"), FIntPoint(4,4), FIntPoint::ZeroValue, FIntPoint::ZeroValue, {EFacilityType::Workbench}),
        MakeRaft(2, LOCTEXT("Raft2", "拾荒甲板"), FIntPoint(6,5), FIntPoint::ZeroValue, FIntPoint::ZeroValue, {EFacilityType::Workbench, EFacilityType::RainBarrel}),
        MakeRaft(3, LOCTEXT("Raft3", "萌芽木筏"), FIntPoint(8,6), FIntPoint::ZeroValue, FIntPoint::ZeroValue, {EFacilityType::Workbench, EFacilityType::RainBarrel, EFacilityType::FarmPlot, EFacilityType::ChickenCoop, EFacilityType::CollectionNet}),
        MakeRaft(4, LOCTEXT("Raft4", "高脚谷仓"), FIntPoint(9,7), FIntPoint(5,4), FIntPoint::ZeroValue, {EFacilityType::Workbench, EFacilityType::RainBarrel, EFacilityType::FarmPlot, EFacilityType::ChickenCoop, EFacilityType::CollectionNet, EFacilityType::StorageLocker}),
        MakeRaft(5, LOCTEXT("Raft5", "海上家园"), FIntPoint(11,8), FIntPoint(7,5), FIntPoint::ZeroValue, {EFacilityType::Workbench, EFacilityType::FarmPlot, EFacilityType::ChickenCoop, EFacilityType::StorageLocker, EFacilityType::TradingDock}),
        MakeRaft(6, LOCTEXT("Raft6", "风帆牧场"), FIntPoint(13,9), FIntPoint(9,6), FIntPoint::ZeroValue, {EFacilityType::Workbench, EFacilityType::FarmPlot, EFacilityType::ChickenCoop, EFacilityType::StorageLocker, EFacilityType::WindTurbine, EFacilityType::RainBarrel}),
        MakeRaft(7, LOCTEXT("Raft7", "潮汐农塔"), FIntPoint(14,10), FIntPoint(10,7), FIntPoint(6,5), {EFacilityType::Workbench, EFacilityType::FarmPlot, EFacilityType::StorageLocker, EFacilityType::WindTurbine, EFacilityType::TradingDock}),
        MakeRaft(8, LOCTEXT("Raft8", "蓝海庄园"), FIntPoint(16,11), FIntPoint(12,8), FIntPoint(8,6), {EFacilityType::Workbench, EFacilityType::RainBarrel, EFacilityType::FarmPlot, EFacilityType::ChickenCoop, EFacilityType::CollectionNet, EFacilityType::StorageLocker, EFacilityType::WindTurbine, EFacilityType::TradingDock, EFacilityType::Desalinator, EFacilityType::AutoCrane}),
        MakeRaft(9, LOCTEXT("Raft9", "方舟牧区"), FIntPoint(18,12), FIntPoint(14,9), FIntPoint(10,7), {EFacilityType::Workbench, EFacilityType::RainBarrel, EFacilityType::FarmPlot, EFacilityType::ChickenCoop, EFacilityType::CollectionNet, EFacilityType::StorageLocker, EFacilityType::WindTurbine, EFacilityType::TradingDock, EFacilityType::Desalinator, EFacilityType::AutoCrane}),
        MakeRaft(10, LOCTEXT("Raft10", "海上绿洲城"), FIntPoint(20,14), FIntPoint(16,11), FIntPoint(12,9), {EFacilityType::Workbench, EFacilityType::RainBarrel, EFacilityType::FarmPlot, EFacilityType::ChickenCoop, EFacilityType::CollectionNet, EFacilityType::StorageLocker, EFacilityType::WindTurbine, EFacilityType::TradingDock, EFacilityType::Desalinator, EFacilityType::AutoCrane, EFacilityType::Lighthouse})
    };
    return Levels;
}

const FRaftLevelDefinition* FRaftProgressionCatalog::Find(int32 Level)
{
    return GetDefinitions().FindByPredicate([Level](const FRaftLevelDefinition& Definition) { return Definition.Level == Level; });
}

#undef LOCTEXT_NAMESPACE
