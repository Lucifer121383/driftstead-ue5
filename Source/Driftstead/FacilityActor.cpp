#include "FacilityActor.h"
#include "DriftsteadCharacter.h"
#include "DriftItemActor.h"
#include "InventoryComponent.h"
#include "DriftsteadGameMode.h"
#include "DriftsteadQuestSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "DemoArt.h"
#include "DriftsteadGameInstance.h"

AFacilityActor::AFacilityActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
    InteractionBounds->InitBoxExtent(FVector(90.0f, 90.0f, 100.0f));
    InteractionBounds->SetCollisionObjectType(ECC_WorldDynamic);
    InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBounds->SetCollisionResponseToAllChannels(ECR_Overlap);
    SetRootComponent(InteractionBounds);

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    BodyMesh->SetupAttachment(RootComponent);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AccentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Accent"));
    AccentMesh->SetupAttachment(RootComponent);
    AccentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StorageInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("StorageInventory"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cube.Succeeded()) BodyMesh->SetStaticMesh(Cube.Object);
    if (Cylinder.Succeeded()) AccentMesh->SetStaticMesh(Cylinder.Object);
    SignalGlow=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SignalGlow"));
    SignalGlow->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded()) SignalGlow->SetStaticMesh(Sphere.Object);
    SignalGlow->SetRelativeLocation(FVector(0,0,213));
    SignalGlow->SetRelativeScale3D(FVector(.27f));
    SignalGlow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SignalGlow->SetVisibility(false);
    BodyMesh->SetRelativeScale3D(FVector(1.1f, 0.9f, 0.7f));
    BodyMesh->SetRelativeLocation(FVector(0, 0, 55));
    AccentMesh->SetRelativeScale3D(FVector(0.32f, 0.32f, 0.75f));
    AccentMesh->SetRelativeLocation(FVector(0, 0, 135));
}

void AFacilityActor::BeginPlay()
{
    Super::BeginPlay();
    StorageInventory->InitializeGrid(8, 4);
    ApplyVisuals();
    ConfigureProductionTimer();
    UpdateTickEnabled();
}

void AFacilityActor::ConfigureProductionTimer()
{
    GetWorldTimerManager().ClearTimer(ProductionTimer);
    if (FacilityType == EFacilityType::RainBarrel || FacilityType == EFacilityType::ChickenCoop || FacilityType == EFacilityType::CollectionNet || FacilityType == EFacilityType::WindTurbine || FacilityType == EFacilityType::Desalinator)
    {
        GetWorldTimerManager().SetTimer(ProductionTimer, this, &AFacilityActor::PerformProduction, 5.0f, true, 2.0f);
    }
    else if (FacilityType == EFacilityType::AutoCrane)
    {
        GetWorldTimerManager().SetTimer(ProductionTimer, this, &AFacilityActor::RunAutoCrane, 3.0f, true, 2.0f);
    }
}

void AFacilityActor::UpdateTickEnabled()
{
    SetActorTickEnabled(bProducing || FacilityType == EFacilityType::Lighthouse || FacilityType == EFacilityType::WindTurbine || FacilityType == EFacilityType::AutoCrane);
}

void AFacilityActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    VisualTime += DeltaSeconds;
    if (FacilityType == EFacilityType::Lighthouse)
    {
        const auto* Quest=GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>();
        bSignalLit=Quest && Quest->GetEventCount(EDriftsteadQuestStep::RepairSignal)>0;
        SignalGlow->SetVisibility(bSignalLit);
        if (bSignalLit) SignalGlow->SetRelativeScale3D(FVector(.27f+FMath::Sin(VisualTime*2.4f)*.025f));
    }
    if (FacilityType == EFacilityType::WindTurbine || FacilityType == EFacilityType::AutoCrane)
    {
        AccentMesh->AddLocalRotation(FRotator(0, 95.0f * DeltaSeconds, 0));
    }
    else if (bProducing)
    {
        const float Pulse = 1.0f + FMath::Sin(VisualTime * 4.0f) * 0.08f;
        AccentMesh->SetRelativeRotation(FRotator(0, FMath::Sin(VisualTime * 2.0f) * 5.0f, 0));
    }
}

void AFacilityActor::Configure(EFacilityType NewType, int32 NewFloorIndex)
{
    FacilityType = NewType;
    FloorIndex = NewFloorIndex;
    if (HasActorBegunPlay())
    {
        ApplyVisuals();
        ConfigureProductionTimer();
        UpdateTickEnabled();
    }
}

FFacilitySaveState AFacilityActor::CaptureSaveState() const
{
    FFacilitySaveState State;
    State.FacilityType = FacilityType;
    State.FloorIndex = FloorIndex;
    State.StoredOutput = StoredOutput;
    State.bProducing = bProducing;
    State.GrowthSecondsRemaining = bProducing ? GetWorldTimerManager().GetTimerRemaining(FarmTimer) : -1.0f;
    if (StorageInventory)
    {
        State.StorageColumns = StorageInventory->GetColumns();
        State.StorageRows = StorageInventory->GetRows();
        State.StorageEntries = StorageInventory->GetEntries();
        State.StorageRecoveryBasket = StorageInventory->GetRecoveryBasket();
        State.StorageResources = StorageInventory->GetResources();
    }
    return State;
}

void AFacilityActor::RestoreSaveState(const FFacilitySaveState& State)
{
    if (State.FacilityType != FacilityType || State.FloorIndex != FloorIndex) return;
    StoredOutput = FMath::Clamp(State.StoredOutput, 0, Capacity);
    bProducing = State.bProducing && FacilityType == EFacilityType::FarmPlot;
    if (StorageInventory)
    {
        StorageInventory->RestoreState(State.StorageColumns, State.StorageRows, State.StorageEntries, State.StorageRecoveryBasket, State.StorageResources);
    }
    GetWorldTimerManager().ClearTimer(FarmTimer);
    if (bProducing) GetWorldTimerManager().SetTimer(FarmTimer, this, &AFacilityActor::FinishFarmProduction,
        FMath::IsFinite(State.GrowthSecondsRemaining) && State.GrowthSecondsRemaining >= 0 ? FMath::Clamp(State.GrowthSecondsRemaining, .1f, 45.0f) : 45.0f, false);
    UpdateTickEnabled();
}

void AFacilityActor::ApplyVisuals()
{
    if (FacilityType == EFacilityType::Lighthouse)
        SignalGlow->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Driftstead/Art/M_Beacon.M_Beacon")));
    const TCHAR* MeshName = TEXT("structure_platform");
    float Size = 180.0f;
    switch (FacilityType)
    {
    case EFacilityType::Workbench: MeshName = TEXT("structure"); Size = 195; break;
    case EFacilityType::RainBarrel: MeshName = TEXT("barrel"); Size = 112; break;
    case EFacilityType::FarmPlot: MeshName = TEXT("platform_planks"); Size = 160; break;
    case EFacilityType::ChickenCoop: MeshName = TEXT("structure_roof"); Size = 155; break;
    case EFacilityType::StorageLocker: MeshName = TEXT("chest"); Size = 125; break;
    case EFacilityType::CollectionNet: MeshName = TEXT("boat_row_small"); Size = 145; break;
    case EFacilityType::WindTurbine: MeshName = TEXT("mast_ropes"); Size = 210; break;
    case EFacilityType::AutoCrane: MeshName = TEXT("mast"); Size = 190; break;
    case EFacilityType::TradingDock: MeshName = TEXT("crate_bottles"); Size = 135; break;
    case EFacilityType::Desalinator: MeshName = TEXT("bottle_large"); Size = 120; break;
    case EFacilityType::Lighthouse: MeshName = TEXT("tower_complete_small"); Size = 215; break;
    }
    if (DriftsteadArt::Fit(BodyMesh, MeshName, Size, FVector(0,0,-15)))
    {
        AccentMesh->SetVisibility(FacilityType == EFacilityType::FarmPlot || FacilityType == EFacilityType::ChickenCoop);
        if (FacilityType == EFacilityType::FarmPlot) DriftsteadArt::Fit(AccentMesh, TEXT("grass_plant"), 75, FVector(0,0,8));
        else if (FacilityType == EFacilityType::ChickenCoop) DriftsteadArt::Fit(AccentMesh, TEXT("crate"), 65, FVector(0,0,-10));
        return;
    }
    static const TMap<EFacilityType, FLinearColor> Colors = {
        {EFacilityType::Workbench, FLinearColor(0.55f,0.28f,0.08f)}, {EFacilityType::RainBarrel, FLinearColor(0.10f,0.46f,0.72f)},
        {EFacilityType::FarmPlot, FLinearColor(0.24f,0.62f,0.14f)}, {EFacilityType::ChickenCoop, FLinearColor(0.90f,0.62f,0.16f)},
        {EFacilityType::CollectionNet, FLinearColor(0.42f,0.68f,0.72f)}, {EFacilityType::StorageLocker, FLinearColor(0.48f,0.32f,0.18f)},
        {EFacilityType::WindTurbine, FLinearColor(0.82f,0.88f,0.90f)}, {EFacilityType::AutoCrane, FLinearColor(0.95f,0.48f,0.08f)},
        {EFacilityType::TradingDock, FLinearColor(0.60f,0.24f,0.72f)}, {EFacilityType::Desalinator, FLinearColor(0.10f,0.72f,0.76f)},
        {EFacilityType::Lighthouse, FLinearColor(0.92f,0.16f,0.12f)}
    };
    AccentMesh->SetRelativeScale3D(FVector(0.32f, 0.32f, 0.75f));
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Driftstead/Materials/M_WoodLight.M_WoodLight"));
    if (!Base) return;
    UMaterialInstanceDynamic* BodyMaterial = UMaterialInstanceDynamic::Create(Base, this);
    BodyMaterial->SetVectorParameterValue(TEXT("BaseColor"), Colors.FindRef(FacilityType));
    BodyMesh->SetMaterial(0, BodyMaterial);
    UMaterialInstanceDynamic* AccentMaterial = UMaterialInstanceDynamic::Create(Base, this);
    AccentMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.0f, 0.76f, 0.23f));
    AccentMesh->SetMaterial(0, AccentMaterial);
#if WITH_EDITOR
    SetActorLabel(FString::Printf(TEXT("Facility_%s_Floor%d"), *UEnum::GetValueAsString(FacilityType), FloorIndex + 1));
#endif
}

void AFacilityActor::PerformProduction()
{
    if (StoredOutput < Capacity) ++StoredOutput;
}

void AFacilityActor::FinishFarmProduction()
{
    bProducing = false;
    StoredOutput = FMath::Min(Capacity, StoredOutput + 4);
    AccentMesh->SetRelativeRotation(FRotator::ZeroRotator);
    UpdateTickEnabled();
}

void AFacilityActor::RunAutoCrane()
{
    TArray<FOverlapResult> Results;
    FCollisionObjectQueryParams Objects(ECC_WorldDynamic);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DriftsteadAutoCrane), false, this);
    GetWorld()->OverlapMultiByObjectType(Results, GetActorLocation(), FQuat::Identity, Objects, FCollisionShape::MakeSphere(650.0f), Params);
    ADriftItemActor* Nearest = nullptr;
    float BestDistance = TNumericLimits<float>::Max();
    for (const FOverlapResult& Result : Results)
    {
        ADriftItemActor* Item = Cast<ADriftItemActor>(Result.GetActor());
        if (!Item) continue;
        const float Distance = FVector::DistSquared(GetActorLocation(), Item->GetActorLocation());
        if (Distance < BestDistance) { Nearest = Item; BestDistance = Distance; }
    }
    ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr);
    if (Nearest && Character)
    {
        Character->GetInventory()->TryAddItem(Nearest->GetItemId(), 1);
        Nearest->Destroy();
        Character->ShowFeedback(NSLOCTEXT("Driftstead", "CraneCatch", "自动吊机已回收附近物资。"), FLinearColor::Green);
    }
}

bool AFacilityActor::Interact_Implementation(ADriftsteadCharacter* Character)
{
    if (!Character) return false;
    UInventoryComponent* Inventory = Character->GetInventory();
    switch (FacilityType)
    {
    case EFacilityType::Workbench:
        if (Inventory->RemoveQuantity(TEXT("SealedBarrel"), 1))
        {
            Inventory->AddResource(TEXT("Wood"), 5); Inventory->AddResource(TEXT("Metal"), 3); Inventory->AddResource(TEXT("Rope"), 3);
            Inventory->AddResource(TEXT("Seeds"), 2);
            Character->ShowFeedback(NSLOCTEXT("Driftstead", "BarrelOpened", "密封木桶已打开：获得混合物资。"), FLinearColor::Green);
            if (UDriftsteadQuestSubsystem* Quest = Character->GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::OpenBarrel);
            return true;
        }
        Character->ShowFeedback(NSLOCTEXT("Driftstead", "WorkbenchHelp", "E 打开密封木桶；U 按清单扩建木筏。"), FLinearColor::Yellow);
        return false;
    case EFacilityType::FarmPlot:
        if (bProducing)
        {
            Character->ShowFeedback(GetInteractionPrompt_Implementation(), FLinearColor::Yellow);
            return false;
        }
        if (StoredOutput > 0)
        {
            Inventory->AddResource(TEXT("Food"), StoredOutput); StoredOutput = 0;
            DriftsteadArt::Play(this,TEXT("Recover"));
            Character->ShowFeedback(NSLOCTEXT("Driftstead", "Harvest", "作物已收获。"), FLinearColor::Green);
            if (UDriftsteadQuestSubsystem* Quest = Character->GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::HarvestCrop);
            return true;
        }
        if (!bProducing && Inventory->GetResource(TEXT("Seeds")) >= 1 && Inventory->GetResource(TEXT("Water")) >= 1)
        {
            TMap<FName,int32> Cost{{TEXT("Seeds"),1},{TEXT("Water"),1}};
            Inventory->ConsumeResourcesTransactional(Cost); bProducing = true;
            DriftsteadArt::Play(this,TEXT("Plant"));
            GetWorldTimerManager().SetTimer(FarmTimer, this, &AFacilityActor::FinishFarmProduction, 45.0f, false);
            UpdateTickEnabled();
            Character->ShowFeedback(NSLOCTEXT("Driftstead", "FarmStarted", "种子已播下，作物开始生长。"), FLinearColor::Green);
            return true;
        }
        Character->ShowFeedback(NSLOCTEXT("Driftstead", "FarmNeeds", "种植田需要 1 份种子和 1 份淡水。"), FLinearColor::Yellow);
        return false;
    case EFacilityType::TradingDock:
    {
        TMap<FName,int32> Cost{{TEXT("Wood"),5},{TEXT("Rope"),2}};
        if (!Inventory->ConsumeResourcesTransactional(Cost)) { Character->ShowFeedback(NSLOCTEXT("Driftstead", "TradeFail", "交易需要 5 份木材和 2 份绳索。"), FLinearColor::Red); return false; }
        Inventory->AddResource(TEXT("Metal"), 3);
        Character->ShowFeedback(NSLOCTEXT("Driftstead", "TradeSuccess", "交易完成：获得 3 份金属。"), FLinearColor::Green);
        return true;
    }
    case EFacilityType::StorageLocker:
        if (StorageInventory->GetEntries().Num() > 0 || StorageInventory->GetRecoveryBasket().Num() > 0)
        {
            const FInventoryEntry& Stored = StorageInventory->GetEntries().Num() > 0 ? StorageInventory->GetEntries()[0] : StorageInventory->GetRecoveryBasket()[0];
            const FName ItemId = Stored.ItemId;
            if (StorageInventory->RemoveQuantity(ItemId, 1))
            {
                Inventory->TryAddItem(ItemId, 1);
                Character->ShowFeedback(NSLOCTEXT("Driftstead", "StorageRetrieved", "已从储物柜取回 1 件物品。"), FLinearColor::Green);
                return true;
            }
        }
        if (Inventory->GetEntries().Num() > 0 || Inventory->GetRecoveryBasket().Num() > 0)
        {
            const FInventoryEntry& Carried = Inventory->GetEntries().Num() > 0 ? Inventory->GetEntries()[0] : Inventory->GetRecoveryBasket()[0];
            const FName ItemId = Carried.ItemId;
            if (Inventory->RemoveQuantity(ItemId, 1))
            {
                StorageInventory->TryAddItem(ItemId, 1);
                Character->ShowFeedback(NSLOCTEXT("Driftstead", "StorageStored", "已存入 1 件物品；再次交互可取回。"), FLinearColor::Green);
                return true;
            }
        }
        Character->ShowFeedback(NSLOCTEXT("Driftstead", "StorageEmpty", "储物柜和背包中都没有可转移的物品。"), FLinearColor::Yellow);
        return false;
    case EFacilityType::Lighthouse:
    {
        UDriftsteadQuestSubsystem* Quest = Character->GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>();
        if (Quest && Quest->GetEventCount(EDriftsteadQuestStep::RepairSignal) > 0)
        {
            Character->ShowFeedback(FText::FromString(TEXT("信标持续发光。远方的船队正在向你驶来。")), FLinearColor::Green);
            return true;
        }
        const TMap<FName,int32> Cost{{TEXT("Wood"),30},{TEXT("Metal"),16},{TEXT("Cloth"),12},{TEXT("Parts"),6},{TEXT("Water"),8},{TEXT("Food"),8}};
        if (Inventory->ConsumeResourcesTransactional(Cost))
        {
            if (Quest) Quest->NotifyEvent(EDriftsteadQuestStep::RepairSignal);
            DriftsteadArt::Play(this,TEXT("Signal"));
            bool bSaved=false;
            if (UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(Character->GetGameInstance())) bSaved=GI->SaveCurrentGame();
            Character->ShowFeedback(FText::FromString(bSaved?TEXT("收到回应！你不再独自漂流。航程已保存。"):TEXT("收到回应！待钩子回到脚下，请按 F5 保存航程。")), FLinearColor(1,.8f,.3f));
            return true;
        }
        Character->ShowFeedback(FText::FromString(TEXT("信标需：木材30 / 金属16 / 布料12 / 零件6 / 淡水8 / 食物8。")), FLinearColor::Yellow);
        return false;
    }
    default:
        if (StoredOutput > 0)
        {
            const FName Resource = FacilityType == EFacilityType::ChickenCoop ? TEXT("Food") : FacilityType == EFacilityType::WindTurbine ? TEXT("Power") : FacilityType == EFacilityType::CollectionNet ? TEXT("Wood") : TEXT("Water");
            Inventory->AddResource(Resource, StoredOutput); StoredOutput = 0;
            if (Resource == TEXT("Water"))
                if (auto* Quest = Character->GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::CollectWater);
            Character->ShowFeedback(FText::Format(NSLOCTEXT("Driftstead", "Collected", "已收取{0}的产出。"), FacilityName()), FLinearColor::Green);
            return true;
        }
        Character->ShowFeedback(FText::Format(NSLOCTEXT("Driftstead", "NotReady", "{0}仍在工作。"), FacilityName()), FLinearColor::Yellow);
        return false;
    }
}

FText AFacilityActor::FacilityName() const
{
    switch (FacilityType)
    {
    case EFacilityType::Workbench: return NSLOCTEXT("Driftstead", "FacilityWorkbench", "工作台");
    case EFacilityType::RainBarrel: return NSLOCTEXT("Driftstead", "FacilityRainBarrel", "雨水桶");
    case EFacilityType::FarmPlot: return NSLOCTEXT("Driftstead", "FacilityFarmPlot", "种植田");
    case EFacilityType::ChickenCoop: return NSLOCTEXT("Driftstead", "FacilityChickenCoop", "鸡舍");
    case EFacilityType::CollectionNet: return NSLOCTEXT("Driftstead", "FacilityCollectionNet", "收集网");
    case EFacilityType::StorageLocker: return NSLOCTEXT("Driftstead", "FacilityStorageLocker", "储物柜");
    case EFacilityType::WindTurbine: return NSLOCTEXT("Driftstead", "FacilityWindTurbine", "风力机");
    case EFacilityType::AutoCrane: return NSLOCTEXT("Driftstead", "FacilityAutoCrane", "自动吊机");
    case EFacilityType::TradingDock: return NSLOCTEXT("Driftstead", "FacilityTradingDock", "交易码头");
    case EFacilityType::Desalinator: return NSLOCTEXT("Driftstead", "FacilityDesalinator", "淡化器");
    case EFacilityType::Lighthouse: return NSLOCTEXT("Driftstead", "FacilityLighthouse", "求救信标");
    default: return NSLOCTEXT("Driftstead", "FacilityUnknown", "设施");
    }
}

FText AFacilityActor::GetInteractionPrompt_Implementation() const
{
    if (FacilityType == EFacilityType::Workbench) return FText::FromString(TEXT("E 开木桶   ·   U 扩建木筏"));
    if (FacilityType == EFacilityType::FarmPlot && bProducing)
        return FText::FromString(FString::Printf(TEXT("作物生长中 · 剩余 %d 秒"), FMath::CeilToInt(GetWorldTimerManager().GetTimerRemaining(FarmTimer))));
    if (StoredOutput > 0) return FText::FromString(FString::Printf(TEXT("E 收取 %s · %d 份"), *FacilityName().ToString(), StoredOutput));
    return FText::Format(NSLOCTEXT("Driftstead", "FacilityPrompt", "E — 使用{0}"), FacilityName());
}

bool AFacilityActor::TryUpgrade(ADriftsteadCharacter* Character)
{
    if (!Character || FacilityType != EFacilityType::Workbench) return false;
    if (auto* GM = GetWorld()->GetAuthGameMode<ADriftsteadGameMode>()) return GM->TryUpgradeRaft(Character->GetInventory());
    return false;
}
