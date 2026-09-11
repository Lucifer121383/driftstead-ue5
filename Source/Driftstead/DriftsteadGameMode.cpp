#include "DriftsteadGameMode.h"
#include "DriftsteadCharacter.h"
#include "DriftsteadPlayerController.h"
#include "DriftsteadHUD.h"
#include "DriftsteadGameInstance.h"
#include "DriftsteadQuestSubsystem.h"
#include "RaftManager.h"
#include "DriftItemSpawner.h"
#include "DriftItemActor.h"
#include "HookComponent.h"
#include "InventoryComponent.h"
#include "Driftstead.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "DemoArt.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "FacilityActor.h"
#include "StairActor.h"
#include "EngineUtils.h"

ADriftsteadGameMode::ADriftsteadGameMode()
{
    DefaultPawnClass = ADriftsteadCharacter::StaticClass();
    PlayerControllerClass = ADriftsteadPlayerController::StaticClass();
    HUDClass = ADriftsteadHUD::StaticClass();
}

void ADriftsteadGameMode::BeginPlay()
{
    Super::BeginPlay();
    BuildRuntimeWorld();
    if (ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
    {
        Character->SetActorLocation(FVector(0, 0, 125), false, nullptr, ETeleportType::TeleportPhysics);
    }
    const bool bSmokeTest = FParse::Param(FCommandLine::Get(), TEXT("DriftsteadSmokeTest"));
    const bool bCaptureScreenshots = FParse::Param(FCommandLine::Get(), TEXT("DriftsteadCapture"));
    const bool bOpeningTest = FParse::Param(FCommandLine::Get(), TEXT("DriftsteadOpeningTest"));
    const bool bMenuTest = FParse::Param(FCommandLine::Get(), TEXT("DriftsteadMenuTest"));
    const bool bManualQA = FParse::Param(FCommandLine::Get(), TEXT("DriftsteadManualQA"));
    if (UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(GetGameInstance()))
    {
        GI->SetAutomationMode(bSmokeTest || bCaptureScreenshots || bOpeningTest || bMenuTest || bManualQA);
        if (bSmokeTest || bCaptureScreenshots || bOpeningTest || bMenuTest) GI->DeleteAutomationSave();
    }
    if (bMenuTest) BeginMenuValidation();
    else if (bOpeningTest)
        GetWorldTimerManager().SetTimer(OpeningTimer, this, &ADriftsteadGameMode::RunOpeningStep, .25f, true, .25f);
    else if (bSmokeTest)
    {
        if (IsValid(ItemSpawner)) ItemSpawner->Destroy();
        ItemSpawner = nullptr;
        GetWorldTimerManager().SetTimer(SmokeTimer, this, &ADriftsteadGameMode::RunSmokeStep, 0.25f, true, 0.25f);
    }
    else if (bCaptureScreenshots)
    {
        GetWorldTimerManager().SetTimer(CaptureTimer, this, &ADriftsteadGameMode::RunCaptureStep, 0.2f, true, 10.0f);
    }
}

void ADriftsteadGameMode::BuildRuntimeWorld()
{
    RaftManager = GetWorld()->SpawnActor<ARaftManager>(ARaftManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    ItemSpawner = GetWorld()->SpawnActor<ADriftItemSpawner>(ADriftItemSpawner::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);

    AStaticMeshActor* Ocean = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(0, 0, -32), FRotator::ZeroRotator);
    if (Ocean)
    {
        UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        Ocean->GetStaticMeshComponent()->SetStaticMesh(Cube);
        Ocean->GetStaticMeshComponent()->SetWorldScale3D(FVector(180, 180, 0.12f));
        Ocean->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Driftstead/Art/M_Ocean.M_Ocean"));
        if (Base)
        {
            UMaterialInstanceDynamic* Water = UMaterialInstanceDynamic::Create(Base, Ocean);
            Water->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.03f, 0.46f, 0.58f));
            Ocean->GetStaticMeshComponent()->SetMaterial(0, Water);
        }
#if WITH_EDITOR
        Ocean->SetActorLabel(TEXT("Runtime_Ocean"));
#endif
    }

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0,0,1000), FRotator(-50,-35,0));
    if (Sun)
    {
        Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sun->GetLightComponent()->SetIntensity(3.0f);
    }
    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>();
    if (Sky) { Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable); Sky->GetLightComponent()->SetIntensity(.7f); }
    ADirectionalLight* Fill = GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0,0,800), FRotator(-65,140,0));
    if (Fill) { Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable); Fill->GetLightComponent()->SetIntensity(1.1f); Fill->GetLightComponent()->SetCastShadows(false); }
    auto Prop = [this](const TCHAR* Name, FVector Location, float Size, float Yaw)
    {
        auto* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, FRotator(0,Yaw,0));
        if (Actor) { Actor->SetMobility(EComponentMobility::Movable); DriftsteadArt::Fit(Actor->GetStaticMeshComponent(), Name, Size); Actor->SetActorLocation(Location); Actor->SetActorEnableCollision(false); }
    };
    Prop(TEXT("patch_sand"), FVector(-730,-820,-15), 680, 0);
    Prop(TEXT("palm_bend"), FVector(-670,-760,0), 390, 25);
    Prop(TEXT("palm_straight"), FVector(-860,-880,0), 300, -40);
    Prop(TEXT("rocks_a"), FVector(-540,-840,-10), 190, 20);
    Prop(TEXT("ship_wreck"), FVector(550,850,-15), 480, -28);
}

void ADriftsteadGameMode::ResetOpeningSupplies() { if (ItemSpawner) ItemSpawner->ResetOpening(); }

FString ADriftsteadGameMode::GetUpgradeSummary(UInventoryComponent* Inventory) const
{
    const auto* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>();
    if (GetRaftLevel() == 4 && Quest && !Quest->IsComplete() && Inventory)
        return FString::Printf(TEXT("修复二层求救信标\n木材 %d/30  金属 %d/16\n布料 %d/12  零件 %d/6\n淡水 %d/8  食物 %d/8"),Inventory->GetResource(TEXT("Wood")),Inventory->GetResource(TEXT("Metal")),Inventory->GetResource(TEXT("Cloth")),Inventory->GetResource(TEXT("Parts")),Inventory->GetResource(TEXT("Water")),Inventory->GetResource(TEXT("Food")));
    const auto* Next = FRaftProgressionCatalog::Find(GetRaftLevel() + 1);
    if (!Next || !Inventory) return TEXT("木筏已达到最高等级");
    FString Text = FString::Printf(TEXT("%d → %d 级  %s"), GetRaftLevel(), Next->Level, *Next->ThemeName.ToString());
    const TArray<TPair<FName,FString>> Names = {{TEXT("Wood"),TEXT("木材")},{TEXT("Rope"),TEXT("绳索")},{TEXT("Metal"),TEXT("金属")},{TEXT("Cloth"),TEXT("布料")},{TEXT("Parts"),TEXT("零件")}};
    int32 Index = 0;
    for (const auto& Pair : Names) if (const int32* Need = Next->UpgradeCost.Find(Pair.Key))
    {
        Text += FString::Printf(TEXT("%s%s %d/%d"), Index++ % 2 == 0 ? TEXT("\n") : TEXT("   "), *Pair.Value, Inventory->GetResource(Pair.Key), *Need);
    }
    return Text;
}

int32 ADriftsteadGameMode::GetRaftLevel() const { return RaftManager ? RaftManager->GetRaftLevel() : 1; }
int32 ADriftsteadGameMode::GetMaximumFloor() const { return RaftManager ? RaftManager->GetMaximumFloor() : 0; }

bool ADriftsteadGameMode::TryUpgradeRaft(UInventoryComponent* Inventory)
{
    auto* Character = Cast<ADriftsteadCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    auto* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>();
    auto* GI = Cast<UDriftsteadGameInstance>(GetGameInstance());
    if (Quest && (!GI || !GI->IsShowcaseMode()))
    {
        FString Gate;
        if (GetRaftLevel() == 2 && Quest->GetEventCount(EDriftsteadQuestStep::CollectWater) == 0) Gate = TEXT("先从雨水桶收取一次淡水，再建立菜园。");
        if (GetRaftLevel() == 3 && Quest->GetEventCount(EDriftsteadQuestStep::HarvestCrop) == 0) Gate = TEXT("先完成一次种植与收获，再扩建二层。");
        if (GetRaftLevel() == 4 && Quest->GetEventCount(EDriftsteadQuestStep::RepairSignal) == 0) Gate = TEXT("先登上二层修复求救信标，完成本次航程。");
        if (!Gate.IsEmpty()) { if (Character) Character->ShowFeedback(FText::FromString(Gate), FLinearColor::Yellow); return false; }
    }
    if (!RaftManager || !RaftManager->TryUpgrade(Inventory))
    {
        if (Character) Character->ShowFeedback(FText::FromString(TEXT("资源尚未备齐，请查看右侧建造清单。")), FLinearColor::Yellow);
        return false;
    }
    if (Quest && GetRaftLevel() == 3) Quest->NotifyEvent(EDriftsteadQuestStep::UpgradeLevel3);
    DriftsteadArt::Play(this,TEXT("Build"));
    ApplyInventoryCapacityForLevel();
    if (Character)
    {
        Character->ShowFeedback(FText::Format(NSLOCTEXT("Driftstead", "UpgradeSuccess", "木筏已升级到 {0} 级！"), FText::AsNumber(GetRaftLevel())), FLinearColor::Green);
        if (Quest && GetRaftLevel() == 2) Quest->NotifyEvent(EDriftsteadQuestStep::UpgradeLevel2);
        if (Quest && GetRaftLevel() == 4) Quest->NotifyEvent(EDriftsteadQuestStep::UpgradeLevel4);
    }
    if (GI) GI->SaveCurrentGame();
    return true;
}

void ADriftsteadGameMode::DebugChangeRaftLevel(int32 Delta)
{
    if (!RaftManager) return;
    RaftManager->ForceLevel(RaftManager->GetRaftLevel() + Delta);
    ApplyInventoryCapacityForLevel();
    if (ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
    {
        const int32 Level = RaftManager->GetRaftLevel();
        Character->SetCurrentFloor(FMath::Min(Character->GetCurrentFloor(), GetMaximumFloor()));
        Character->ShowFeedback(FText::Format(NSLOCTEXT("Driftstead", "ShowcaseLevel", "展示：木筏 {0} 级"), FText::AsNumber(Level)), FLinearColor::Yellow);
    }
}

void ADriftsteadGameMode::SetRaftLevelFromSave(int32 Level) { if (RaftManager) { RaftManager->ForceLevel(Level); ApplyInventoryCapacityForLevel(); } }
void ADriftsteadGameMode::SetViewedFloor(int32 Floor) { if (RaftManager) RaftManager->SetViewedFloor(Floor); }
void ADriftsteadGameMode::SpawnDebugItems() { if (ItemSpawner) ItemSpawner->SpawnBatch(10); }
TArray<FFacilitySaveState> ADriftsteadGameMode::CaptureFacilityStates() const { return RaftManager ? RaftManager->CaptureFacilityStates() : TArray<FFacilitySaveState>(); }
void ADriftsteadGameMode::RestoreFacilityStates(const TArray<FFacilitySaveState>& States) { if (RaftManager) RaftManager->RestoreFacilityStates(States); }

void ADriftsteadGameMode::ApplyInventoryCapacityForLevel()
{
    ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Character || !RaftManager) return;
    const int32 Level = RaftManager->GetRaftLevel();
    if (Level >= 8) Character->GetInventory()->InitializeGrid(12, 8);
    else if (Level >= 5) Character->GetInventory()->InitializeGrid(10, 6);
    else if (Level >= 3) Character->GetInventory()->InitializeGrid(8, 5);
    else Character->GetInventory()->InitializeGrid(6, 4);
}

void ADriftsteadGameMode::RunSmokeStep()
{
    ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Character || !RaftManager)
    {
        UE_LOG(LogDriftstead, Error, TEXT("DRIFTSTEAD_SMOKE FAIL: runtime character or raft manager missing"));
        bSmokeFailed = true;
        FGenericPlatformMisc::RequestExit(false);
        return;
    }

    switch (SmokeStep++)
    {
    case 0:
    {
        Character->GetInventory()->AddTestResources(100);
        SmokeInitialDriftwood = Character->GetInventory()->CountItem(TEXT("Driftwood"));
        Character->GetHook()->SetAimDirection(FVector(0, 1, 0));
        // Two targets lie along the return path at very different Z heights.
        // Outbound flight must ignore both, reach its charged range, then the
        // returning hook must carry both all the way back to the player.
        const FVector TargetLocation = Character->GetActorLocation() + FVector(0, 480, 800);
        ADriftItemActor* Item = GetWorld()->SpawnActor<ADriftItemActor>(ADriftItemActor::StaticClass(), TargetLocation, FRotator::ZeroRotator);
        if (Item) Item->ConfigureItem(TEXT("Driftwood"), FVector::ZeroVector);
        SmokeTarget = Item;
        ADriftItemActor* Decoy = GetWorld()->SpawnActor<ADriftItemActor>(ADriftItemActor::StaticClass(), Character->GetActorLocation() + FVector(0, 300, -400), FRotator::ZeroRotator);
        if (Decoy) Decoy->ConfigureItem(TEXT("Driftwood"), FVector::ZeroVector);
        SmokeDecoy = Decoy;
        Character->GetHook()->StartCharging();
        UE_LOG(LogDriftstead, Display, TEXT("DRIFTSTEAD_SMOKE: charge started"));
        break;
    }
    case 1:
        Character->GetHook()->SetAimDirection(FVector(0, 1, 0));
        Character->GetHook()->ReleaseHook();
        UE_LOG(LogDriftstead, Display, TEXT("DRIFTSTEAD_SMOKE: hook released"));
        break;
    case 2:
        if (Character->GetHook()->GetHookState() != EHookState::Idle && SmokeWaitTicks++ < 20)
        {
            --SmokeStep;
            break;
        }
        if (Character->GetInventory()->CountItem(TEXT("Driftwood")) != SmokeInitialDriftwood + 2 || IsValid(SmokeTarget) || IsValid(SmokeDecoy))
        {
            UE_LOG(LogDriftstead, Error, TEXT("DRIFTSTEAD_SMOKE FAIL: return-path hook did not recover both designated Driftwood actors"));
            bSmokeFailed = true;
        }
        if (IsValid(SmokeDecoy)) SmokeDecoy->Destroy();
        SmokeDecoy = nullptr;
        RaftManager->ForceLevel(4);
        Character->SetCurrentFloor(1);
        if (RaftManager->GetMaximumFloor() != 1 || Character->GetCurrentFloor() != 1)
        {
            UE_LOG(LogDriftstead, Error, TEXT("DRIFTSTEAD_SMOKE FAIL: level 4 second floor unavailable"));
            bSmokeFailed = true;
        }
        break;
    case 3:
        RaftManager->ForceLevel(7);
        Character->SetCurrentFloor(2);
        if (RaftManager->GetMaximumFloor() != 2 || Character->GetCurrentFloor() != 2)
        {
            UE_LOG(LogDriftstead, Error, TEXT("DRIFTSTEAD_SMOKE FAIL: level 7 third floor unavailable"));
            bSmokeFailed = true;
        }
        break;
    case 4:
    {
        RaftManager->ForceLevel(10);
        Character->GetInventory()->AddResource(TEXT("Power"), 10);
        for (const FFacilitySaveState& State : RaftManager->CaptureFacilityStates())
        {
            if (State.FacilityType == EFacilityType::RainBarrel)
            {
                SmokeInitialPassiveOutput = State.StoredOutput;
                break;
            }
        }
        if (SmokeInitialPassiveOutput < 0)
        {
            UE_LOG(LogDriftstead, Error, TEXT("DRIFTSTEAD_SMOKE FAIL: passive production facility missing"));
            bSmokeFailed = true;
        }
        break;
    }
    case 5:
    {
        if (SmokeProductionWaitTicks++ < 10)
        {
            --SmokeStep;
            break;
        }
        const TArray<FFacilitySaveState> ProducedStates = RaftManager->CaptureFacilityStates();
        const FFacilitySaveState* ProducedState = ProducedStates.FindByPredicate([](const FFacilitySaveState& State)
        {
            return State.FacilityType == EFacilityType::RainBarrel;
        });
        if (!ProducedState || ProducedState->StoredOutput <= SmokeInitialPassiveOutput)
        {
            UE_LOG(LogDriftstead, Error, TEXT("DRIFTSTEAD_SMOKE FAIL: passive facility timer did not produce output"));
            bSmokeFailed = true;
        }
        TArray<FFacilitySaveState> FacilityStates = RaftManager->CaptureFacilityStates();
        FFacilitySaveState ExpectedFacilityState;
        bool bHasExpectedFacilityState = false;
        for (FFacilitySaveState& State : FacilityStates)
        {
            if (State.FacilityType != EFacilityType::StorageLocker) continue;
            State.StoredOutput = 4;
            FInventoryEntry StoredRope;
            StoredRope.InstanceId = FGuid::NewGuid();
            StoredRope.ItemId = TEXT("Rope");
            StoredRope.Quantity = 2;
            StoredRope.GridPosition = FIntPoint::ZeroValue;
            State.StorageEntries = {StoredRope};
            ExpectedFacilityState = State;
            bHasExpectedFacilityState = true;
            break;
        }
        RaftManager->RestoreFacilityStates(FacilityStates);
        if (UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(GetGameInstance()))
        {
            if (!GI->SaveCurrentGame())
            {
                UE_LOG(LogDriftstead, Error, TEXT("DRIFTSTEAD_SMOKE FAIL: save failed"));
                bSmokeFailed = true;
            }
            RaftManager->ForceLevel(1);
            if (!GI->LoadCurrentGame() || RaftManager->GetRaftLevel() != 10)
            {
                UE_LOG(LogDriftstead, Error, TEXT("DRIFTSTEAD_SMOKE FAIL: isolated save/load round-trip failed"));
                bSmokeFailed = true;
            }
            if (bHasExpectedFacilityState)
            {
                const TArray<FFacilitySaveState> RestoredStates = RaftManager->CaptureFacilityStates();
                const FFacilitySaveState* Restored = RestoredStates.FindByPredicate([&ExpectedFacilityState](const FFacilitySaveState& State)
                {
                    return State.FacilityType == ExpectedFacilityState.FacilityType && State.FloorIndex == ExpectedFacilityState.FloorIndex;
                });
                if (!Restored || Restored->StoredOutput != 4 || Restored->StorageEntries.Num() != 1 || Restored->StorageEntries[0].ItemId != FName(TEXT("Rope")) || Restored->StorageEntries[0].Quantity != 2)
                {
                    UE_LOG(LogDriftstead, Error, TEXT("DRIFTSTEAD_SMOKE FAIL: facility production/storage state was not restored"));
                    bSmokeFailed = true;
                }
            }
            GI->DeleteAutomationSave();
        }
        UE_LOG(LogDriftstead, Display, TEXT("DRIFTSTEAD_SMOKE %s: charged-range outbound and multi-target return-path recovery, inventory, levels 4/7/10, floors, passive production, facility storage and isolated save/load"), bSmokeFailed ? TEXT("FAIL") : TEXT("PASS"));
        GetWorldTimerManager().ClearTimer(SmokeTimer);
        FGenericPlatformMisc::RequestExit(false);
        break;
    }
    default:
        break;
    }
}

void ADriftsteadGameMode::CaptureFrame(const FString& Filename)
{
    const FString Directory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts"), TEXT("Screenshots"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString OutputPath = FPaths::Combine(Directory, Filename);
    FScreenshotRequest::RequestScreenshot(OutputPath, false, false);
    UE_LOG(LogDriftstead, Display, TEXT("DRIFTSTEAD_CAPTURE requested %s"), *OutputPath);
}

void ADriftsteadGameMode::RunCaptureStep()
{
    ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Character || !RaftManager) return;
    ADriftsteadHUD* HUD = Character->GetController<APlayerController>() ? Cast<ADriftsteadHUD>(Character->GetController<APlayerController>()->GetHUD()) : nullptr;

    switch (CaptureStep++)
    {
    case 0:
        RaftManager->ForceLevel(1); ApplyInventoryCapacityForLevel(); Character->SetCurrentFloor(0); if (HUD) HUD->CloseMainMenu();
        CaptureFrame(TEXT("01_Level1_Overview.png"));
        break;
    case 1:
    {
        Character->GetHook()->SetAimDirection(FVector(0, 1, 0));
        ADriftItemActor* FarItem = GetWorld()->SpawnActor<ADriftItemActor>(ADriftItemActor::StaticClass(), Character->GetActorLocation() + FVector(0, 600, 65), FRotator::ZeroRotator);
        if (FarItem) FarItem->ConfigureItem(TEXT("Driftwood"), FVector::ZeroVector);
        ADriftItemActor* NearItem = GetWorld()->SpawnActor<ADriftItemActor>(ADriftItemActor::StaticClass(), Character->GetActorLocation() + FVector(0, 450, -240), FRotator::ZeroRotator);
        if (NearItem) NearItem->ConfigureItem(TEXT("Rope"), FVector::ZeroVector);
        Character->GetHook()->StartCharging();
        break;
    }
    case 2:
        break;
    case 3:
        Character->GetHook()->ReleaseHook();
        break;
    case 4:
        if (Character->GetHook()->GetAttachedCount() < 2 && CaptureHookWaitTicks++ < 8)
        {
            --CaptureStep;
            break;
        }
        CaptureFrame(TEXT("02_HookCatch.png"));
        break;
    case 5:
        if (Character->GetHook()->GetHookState() != EHookState::Idle) { --CaptureStep; break; }
        if (HUD) HUD->SetInventoryOpen(true);
        CaptureFrame(TEXT("03_Inventory.png"));
        break;
    case 6:
        if (HUD) HUD->SetInventoryOpen(false); RaftManager->ForceLevel(4); ApplyInventoryCapacityForLevel(); Character->SetCurrentFloor(1);
        CaptureFrame(TEXT("04_Level4_SecondFloor.png"));
        break;
    case 7:
        RaftManager->ForceLevel(7); ApplyInventoryCapacityForLevel(); Character->SetCurrentFloor(2);
        CaptureFrame(TEXT("05_Level7_ThirdFloor.png"));
        break;
    case 8:
        RaftManager->ForceLevel(10); ApplyInventoryCapacityForLevel(); Character->SetCurrentFloor(2);
        CaptureFrame(TEXT("06_Level10_FullRaft.png"));
        break;
    case 9:
        if (UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(GetGameInstance())) GI->DeleteAutomationSave();
        GetWorldTimerManager().ClearTimer(CaptureTimer);
        FGenericPlatformMisc::RequestExit(false);
        break;
    default:
        break;
    }
}
