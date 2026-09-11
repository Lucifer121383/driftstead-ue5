// Deterministic integration test: actual hook, interactions, upgrade gates and
// the real 45-second crop timer. Seeded resources avoid measuring grind speed.
#include "DriftsteadGameMode.h"
#include "Driftstead.h"
#include "DriftsteadCharacter.h"
#include "DriftsteadGameInstance.h"
#include "DriftsteadQuestSubsystem.h"
#include "DriftsteadHUD.h"
#include "DriftsteadSaveGame.h"
#include "DriftItemActor.h"
#include "DriftItemSpawner.h"
#include "FacilityActor.h"
#include "StairActor.h"
#include "HookComponent.h"
#include "InventoryComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void ADriftsteadGameMode::RunOpeningStep()
{
    auto* Player = Cast<ADriftsteadCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
    auto* Instance = Cast<UDriftsteadGameInstance>(GetGameInstance());
    auto* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>();
    auto Check = [this](bool Passed, const TCHAR* Message)
    {
        if (!Passed) { bOpeningFailed = true; UE_LOG(LogDriftstead,Error,TEXT("DRIFTSTEAD_OPENING FAIL: %s"),Message); }
        else UE_LOG(LogDriftstead,Display,TEXT("DRIFTSTEAD_OPENING checked: %s"),Message);
    };
    if (!Player || !Instance || !Quest)
    {
        Check(false,TEXT("required runtime objects"));
        FGenericPlatformMisc::RequestExit(false); return;
    }
    auto* Inventory = Player->GetInventory();
    auto* Hook = Player->GetHook();
    auto FindStation = [this](EFacilityType Type) -> AFacilityActor*
    {
        for (TActorIterator<AFacilityActor> It(GetWorld()); It; ++It)
            if (IsValid(*It) && It->GetFacilityType() == Type) return *It;
        return nullptr;
    };
    auto Use = [&FindStation, Player](EFacilityType Type)
    {
        auto* Station = FindStation(Type);
        return Station && IInteractableInterface::Execute_Interact(Station,Player);
    };
    switch (OpeningStep++)
    {
    case 0:
    {
        if (auto* HUD = Cast<ADriftsteadHUD>(Player->GetController<APlayerController>()->GetHUD())) HUD->CloseMainMenu();
        int32 Count=0; bool Reachable=true, Barrel=false, Seeds=false;
        for (TActorIterator<ADriftItemActor> It(GetWorld()); It; ++It)
        {
            ++Count; Reachable &= It->GetActorLocation().Size2D() < 1250;
            Barrel |= It->GetItemId() == TEXT("SealedBarrel"); Seeds |= It->GetItemId() == TEXT("SeedCrate");
        }
        Check(Count>=12 && Reachable && Barrel && Seeds,TEXT("opening manifest contains reachable barrel, seeds and supplies"));
        if (ItemSpawner) ItemSpawner->Destroy(); ItemSpawner=nullptr;
        Quest->ResetQuest();
        Quest->NotifyEvent(EDriftsteadQuestStep::Move);
        Quest->NotifyEvent(EDriftsteadQuestStep::OpenInventory); // deliberately before salvage
        Inventory->AddTestResources(300);
        const TArray<FName> Cargo = {TEXT("Driftwood"),TEXT("Rope"),TEXT("ScrapMetal"),TEXT("Cloth"),TEXT("SealedBarrel"),TEXT("SeedCrate")};
        for (int32 I=0; I<Cargo.Num(); ++I)
        {
            auto* Item=GetWorld()->SpawnActor<ADriftItemActor>(FVector(0,320+I*90,35),FRotator::ZeroRotator);
            if (Item) Item->ConfigureItem(Cargo[I],FVector::ZeroVector);
        }
        Hook->SetAimDirection(FVector(0,1,0)); Hook->StartCharging();
        Check(!Instance->SaveCurrentGame(),TEXT("saving in-flight cargo is rejected without UI mutation"));
        OpeningWait=0;
        break;
    }
    case 1:
        if (OpeningWait++<5) { --OpeningStep; break; }
        Hook->ReleaseHook(); OpeningWait=0; break;
    case 2:
        if (Hook->GetHookState()!=EHookState::Idle && OpeningWait++<40) { --OpeningStep; break; }
        Check(Quest->GetEventCount(EDriftsteadQuestStep::SalvageItem)==6,TEXT("six items physically returned and credited"));
        Check(Quest->GetCurrentStep()==EDriftsteadQuestStep::OpenBarrel,TEXT("early inventory action is remembered"));
        Check(Use(EFacilityType::Workbench),TEXT("workbench opens recovered barrel"));
        {
            const int32 Wood=Inventory->GetResource(TEXT("Wood")), Rope=Inventory->GetResource(TEXT("Rope"));
            Check(TryUpgradeRaft(Inventory) && GetRaftLevel()==2,TEXT("paid upgrade to level 2"));
            Check(Inventory->GetResource(TEXT("Wood"))==Wood-18 && Inventory->GetResource(TEXT("Rope"))==Rope-8,TEXT("upgrade deducts exact cost once"));
            const int32 Before=Inventory->GetResource(TEXT("Wood"));
            Check(!TryUpgradeRaft(Inventory) && Inventory->GetResource(TEXT("Wood"))==Before,TEXT("water lesson gate blocks without taking resources"));
        }
        OpeningWait=0; break;
    case 3:
        if (OpeningWait++<10) { --OpeningStep; break; }
        Check(Use(EFacilityType::RainBarrel),TEXT("rain barrel timer produces collectable fresh water"));
        Check(TryUpgradeRaft(Inventory) && GetRaftLevel()==3,TEXT("water lesson unlocks paid level 3 upgrade"));
        Check(Use(EFacilityType::FarmPlot),TEXT("plant consumes seed and water"));
        {
            const int32 Before=Inventory->GetResource(TEXT("Wood"));
            Check(!TryUpgradeRaft(Inventory) && Inventory->GetResource(TEXT("Wood"))==Before,TEXT("harvest gate blocks premature second floor"));
        }
        OpeningWait=0; break;
    case 4:
        if (OpeningWait++<12) { --OpeningStep; break; }
        if (auto* Farm=FindStation(EFacilityType::FarmPlot)) OpeningGrowthRemaining=Farm->CaptureSaveState().GrowthSecondsRemaining;
        Check(OpeningGrowthRemaining>38 && OpeningGrowthRemaining<44,TEXT("crop is running its real 45-second timer"));
        Check(!Use(EFacilityType::FarmPlot),TEXT("repeated planting while growing does not consume resources"));
        Check(Instance->SaveCurrentGame() && Instance->LoadCurrentGame(),TEXT("mid-growth save/load succeeds"));
        if (auto* Farm=FindStation(EFacilityType::FarmPlot))
            Check(FMath::Abs(Farm->CaptureSaveState().GrowthSecondsRemaining-OpeningGrowthRemaining)<1,TEXT("save restores remaining crop time, not a fresh timer"));
        OpeningWait=0; break;
    case 5:
        if (auto* Farm=FindStation(EFacilityType::FarmPlot))
            if (Farm->CaptureSaveState().StoredOutput==0 && OpeningWait++<200) { --OpeningStep; break; }
        Check(Use(EFacilityType::FarmPlot),TEXT("mature crop can be harvested"));
        Check(TryUpgradeRaft(Inventory) && GetRaftLevel()==4,TEXT("harvest unlocks paid second-floor expansion"));
        {
            bool Climbed=false;
            for (TActorIterator<AStairActor> It(GetWorld()); It; ++It)
                if (It->GetFromFloor()==0) { Climbed=IInteractableInterface::Execute_Interact(*It,Player); break; }
            Check(Climbed && Player->GetCurrentFloor()==1,TEXT("stairs reach second floor through interaction"));
        }
        break;
    case 6:
    {
        Inventory->AddResource(TEXT("Parts"),-Inventory->GetResource(TEXT("Parts")));
        const int32 Wood=Inventory->GetResource(TEXT("Wood"));
        Check(!Use(EFacilityType::Lighthouse) && Inventory->GetResource(TEXT("Wood"))==Wood,TEXT("insufficient signal resources leave all other resources untouched"));
        Inventory->AddResource(TEXT("Parts"),6);
        Check(Use(EFacilityType::Lighthouse) && Quest->IsComplete(),TEXT("signal completes all 12 objectives"));
        const int32 After=Inventory->GetResource(TEXT("Wood"));
        Check(After==Wood-30 && Use(EFacilityType::Lighthouse) && Inventory->GetResource(TEXT("Wood"))==After,TEXT("repeated signal interaction never charges twice"));
        Check(Instance->SaveCurrentGame(),TEXT("completed voyage saves"));
        Quest->ResetQuest();
        Check(Instance->LoadCurrentGame() && Quest->IsComplete() && GetRaftLevel()==4 && Player->GetCurrentFloor()==1,TEXT("ending, level, floor and quests survive a full round-trip"));
        auto* Corrupt=NewObject<UDriftsteadSaveGame>(); Corrupt->SaveVersion=999;
        UGameplayStatics::SaveGameToSlot(Corrupt,TEXT("Driftstead_Automation"),0);
        Check(Instance->LoadCurrentGame() && Quest->IsComplete(),TEXT("corrupt active save falls back to previous valid checkpoint"));
        CaptureFrame(TEXT("07_SignalEnding.png"));
        break;
    }
    case 7:
        Instance->DeleteAutomationSave();
        UE_LOG(LogDriftstead,Display,TEXT("DRIFTSTEAD_OPENING %s: real multi-catch, gates, paid upgrades, 45s farm, stairs, atomic signal repair and save/load; seeded resources, not a human playtime benchmark"),bOpeningFailed?TEXT("FAIL"):TEXT("PASS"));
        GetWorldTimerManager().ClearTimer(OpeningTimer);
        FGenericPlatformMisc::RequestExit(false); break;
    }
}
