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
    if (UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(GetGameInstance()))
    {
        GI->SetAutomationMode(bSmokeTest || bCaptureScreenshots);
        if (bSmokeTest || bCaptureScreenshots) GI->DeleteAutomationSave();
    }
    if (bSmokeTest)
    {
        if (IsValid(ItemSpawner)) ItemSpawner->Destroy();
        ItemSpawner = nullptr;
        GetWorldTimerManager().SetTimer(SmokeTimer, this, &ADriftsteadGameMode::RunSmokeStep, 0.25f, true, 0.25f);
    }
    else if (bCaptureScreenshots)
    {
        if (IsValid(ItemSpawner)) ItemSpawner->Destroy();
        ItemSpawner = nullptr;
        GetWorldTimerManager().SetTimer(CaptureTimer, this, &ADriftsteadGameMode::RunCaptureStep, 0.45f, true, 1.0f);
    }
}

void ADriftsteadGameMode::BuildRuntimeWorld()
{
    RaftManager = GetWorld()->SpawnActor<ARaftManager>(ARaftManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    ItemSpawner = GetWorld()->SpawnActor<ADriftItemSpawner>(ADriftItemSpawner::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);

    AStaticMeshActor* Ocean = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(0, 0, -65), FRotator::ZeroRotator);
    if (Ocean)
    {
        UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        Ocean->GetStaticMeshComponent()->SetStaticMesh(Cube);
        Ocean->GetStaticMeshComponent()->SetWorldScale3D(FVector(45, 45, 0.12f));
        Ocean->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Driftstead/Materials/M_Water.M_Water"));
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
        Sun->GetLightComponent()->SetIntensity(7.0f);
    }
}

int32 ADriftsteadGameMode::GetRaftLevel() const { return RaftManager ? RaftManager->GetRaftLevel() : 1; }
int32 ADriftsteadGameMode::GetMaximumFloor() const { return RaftManager ? RaftManager->GetMaximumFloor() : 0; }

bool ADriftsteadGameMode::TryUpgradeRaft(UInventoryComponent* Inventory)
{
    if (!RaftManager || !RaftManager->TryUpgrade(Inventory)) return false;
    ApplyInventoryCapacityForLevel();
    if (ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
    {
        Character->ShowFeedback(FText::Format(NSLOCTEXT("Driftstead", "UpgradeSuccess", "木筏已升级到 {0} 级！"), FText::AsNumber(GetRaftLevel())), FLinearColor::Green);
        if (GetRaftLevel() == 2)
            if (UDriftsteadQuestSubsystem* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::UpgradeLevel2);
        if (GetRaftLevel() == 4)
            if (UDriftsteadQuestSubsystem* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::UpgradeLevel4);
    }
    if (UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(GetGameInstance())) GI->SaveCurrentGame();
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
        // Deliberately separate Z by 800 units: hook targeting is planar (X/Y)
        // and must not miss because of the orthographic presentation height.
        const FVector TargetLocation = Character->GetActorLocation() + FVector(0, 480, 800);
        ADriftItemActor* Item = GetWorld()->SpawnActor<ADriftItemActor>(ADriftItemActor::StaticClass(), TargetLocation, FRotator::ZeroRotator);
        if (Item) Item->ConfigureItem(TEXT("Driftwood"), FVector::ZeroVector);
        SmokeTarget = Item;
        ADriftItemActor* Decoy = GetWorld()->SpawnActor<ADriftItemActor>(ADriftItemActor::StaticClass(), TargetLocation + FVector(80, 0, -1200), FRotator::ZeroRotator);
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
        if (Character->GetInventory()->CountItem(TEXT("Driftwood")) != SmokeInitialDriftwood + 1 || IsValid(SmokeTarget) || !IsValid(SmokeDecoy))
        {
            UE_LOG(LogDriftstead, Error, TEXT("DRIFTSTEAD_SMOKE FAIL: hook did not recover exactly one closest designated Driftwood"));
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
        UE_LOG(LogDriftstead, Display, TEXT("DRIFTSTEAD_SMOKE %s: single-target planar hook recovery, inventory, levels 4/7/10, floors, passive production, facility storage and isolated save/load"), bSmokeFailed ? TEXT("FAIL") : TEXT("PASS"));
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
        ADriftItemActor* Item = GetWorld()->SpawnActor<ADriftItemActor>(ADriftItemActor::StaticClass(), Character->GetActorLocation() + FVector(0, 450, 65), FRotator::ZeroRotator);
        if (Item) Item->ConfigureItem(TEXT("Driftwood"), FVector::ZeroVector);
        Character->GetHook()->StartCharging();
        break;
    }
    case 2:
        break;
    case 3:
        Character->GetHook()->ReleaseHook();
        break;
    case 4:
        CaptureFrame(TEXT("02_HookCatch.png"));
        break;
    case 5:
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
