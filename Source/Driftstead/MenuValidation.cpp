// Runtime menu/save regression. The core ticker continues while the game world
// is paused, so observing a returned main menu cannot deadlock this test.
// This flag always uses the isolated Automation slot, never player saves.
#include "DriftsteadGameMode.h"
#include "Driftstead.h"
#include "DriftsteadCharacter.h"
#include "DriftsteadHUD.h"
#include "DriftsteadGameInstance.h"
#include "DriftsteadQuestSubsystem.h"
#include "DriftsteadSaveGame.h"
#include "DriftItemSpawner.h"
#include "DriftItemActor.h"
#include "HookComponent.h"
#include "InventoryComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"

namespace
{
struct FMenuValidationState
{
    int32 Step = 0;
    bool bFailed = false;
    double StepStartedAt = FPlatformTime::Seconds();
    int32 Wood = 0, Rope = 0, Driftwood = 0, Metal = 0, ScrapMetal = 0;
    int32 Salvaged = 0, Completed = 0;
    TArray<TWeakObjectPtr<ADriftItemActor>> Cargo;
};
}

void ADriftsteadGameMode::BeginMenuValidation()
{
    const TSharedRef<FMenuValidationState> State = MakeShared<FMenuValidationState>();
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this, State](float)
    {
        auto* Player = Cast<ADriftsteadCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
        auto* Instance = Cast<UDriftsteadGameInstance>(GetGameInstance());
        auto* Quest = Instance ? Instance->GetSubsystem<UDriftsteadQuestSubsystem>() : nullptr;
        auto* Controller = Player ? Player->GetController<APlayerController>() : nullptr;
        auto* HUD = Controller ? Cast<ADriftsteadHUD>(Controller->GetHUD()) : nullptr;
        auto Check = [State](bool bPassed, const TCHAR* Message)
        {
            if (!bPassed) { State->bFailed = true; UE_LOG(LogDriftstead, Error, TEXT("DRIFTSTEAD_MENU FAIL: %s"), Message); }
            else UE_LOG(LogDriftstead, Display, TEXT("DRIFTSTEAD_MENU checked: %s"), Message);
        };
        auto Finish = [this, State, Instance]()
        {
            UGameplayStatics::SetGamePaused(this, false);
            if (Instance) Instance->DeleteAutomationSave();
            UE_LOG(LogDriftstead, Display, TEXT("DRIFTSTEAD_MENU %s: pause/save, safe multi-cargo return, suspended-session resume, fresh-session panels and 10 cooked icons; not a mouse-drag GUI test"), State->bFailed ? TEXT("FAIL") : TEXT("PASS"));
            FGenericPlatformMisc::RequestExit(false);
            return false;
        };
        const double InStepSeconds = FPlatformTime::Seconds() - State->StepStartedAt;
        if (InStepSeconds > 15.0)
        {
            Check(false, *FString::Printf(TEXT("step %d timed out without a safe transition"), State->Step));
            return Finish();
        }
        if (!Player || !Instance || !Quest || !HUD) return true;
        auto Advance = [State]() { ++State->Step; State->StepStartedAt = FPlatformTime::Seconds(); };
        auto* Inventory = Player->GetInventory();
        auto* Hook = Player->GetHook();
        auto Pause = [this, HUD]()
        {
            if (!HUD->IsPausePanelOpen()) HUD->TogglePausePanel();
            UGameplayStatics::SetGamePaused(this, true);
        };
        auto CheckReturned = [this, State, Player, HUD, Hook, &Check]()
        {
            Check(HUD->IsMainMenuOpen() && !HUD->IsPausePanelOpen() && !HUD->IsInventoryOpen(), TEXT("return opens only the main menu"));
            Check(UGameplayStatics::IsGamePaused(this) && Player->HasSuspendedSession() && !Player->IsReturnToMenuPending(), TEXT("returned session is paused and resumable"));
            Check(Hook->GetAttachedCount() == 0 && (Hook->GetHookState() == EHookState::Idle || Hook->GetHookState() == EHookState::Cooldown), TEXT("returned menu has no in-flight hook cargo"));
        };
        auto Resume = [this, Player, HUD, &Check]()
        {
            Player->SelectMenuOption(1);
            Check(!HUD->IsMainMenuOpen() && !HUD->IsPausePanelOpen() && !UGameplayStatics::IsGamePaused(this) && !Player->IsReturnToMenuPending(), TEXT("continue resumes gameplay without leftover pause"));
        };
        auto Snapshot = [State, Inventory, Quest]()
        {
            State->Wood = Inventory->GetResource(TEXT("Wood"));
            State->Rope = Inventory->CountItem(TEXT("Rope"));
            State->Driftwood = Inventory->CountItem(TEXT("Driftwood"));
            State->Metal = Inventory->GetResource(TEXT("Metal"));
            State->ScrapMetal = Inventory->CountItem(TEXT("ScrapMetal"));
            State->Salvaged = Quest->GetEventCount(EDriftsteadQuestStep::SalvageItem);
            State->Completed = Quest->GetCompletedObjectives();
        };
        auto SpawnCargo = [this, State, &Check](FName FirstId, float FirstY, FName SecondId, float SecondY)
        {
            State->Cargo.Reset();
            for (const auto& Pair : TArray<TPair<FName, float>>{{FirstId, FirstY}, {SecondId, SecondY}})
            {
                auto* Item = GetWorld()->SpawnActor<ADriftItemActor>(FVector(0, Pair.Value, 35), FRotator::ZeroRotator);
                Check(IsValid(Item), TEXT("deterministic return-path cargo spawned"));
                if (Item) { Item->ConfigureItem(Pair.Key, FVector::ZeroVector); State->Cargo.Add(Item); }
            }
        };
        auto CheckCargoGone = [State, &Check]()
        {
            for (const auto& Item : State->Cargo) Check(!Item.IsValid(), TEXT("recovered world cargo is removed exactly once"));
        };
        auto CheckSavedCargo = [Inventory, &Check]()
        {
            const auto* Saved = Cast<UDriftsteadSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("Driftstead_Automation"), 0));
            Check(Saved != nullptr, TEXT("return writes the isolated automation checkpoint"));
            if (!Saved) return;
            for (FName Id : {FName(TEXT("Rope")), FName(TEXT("Driftwood")), FName(TEXT("ScrapMetal"))})
            {
                int32 Quantity = 0;
                for (const auto& Entry : Saved->InventoryEntries) if (Entry.ItemId == Id) Quantity += Entry.Quantity;
                for (const auto& Entry : Saved->RecoveryBasket) if (Entry.ItemId == Id) Quantity += Entry.Quantity;
                Check(Quantity == Inventory->CountItem(Id), TEXT("checkpoint includes all recovered inventory and basket items"));
            }
        };

        switch (State->Step)
        {
        case 0:
        {
            Player->SelectMenuOption(0);
            if (ItemSpawner) ItemSpawner->Destroy();
            ItemSpawner = nullptr;
            Check(!HUD->IsMainMenuOpen() && !UGameplayStatics::IsGamePaused(this), TEXT("new voyage starts without paused world"));
            int32 IconCount = 0;
            for (const auto& Definition : FDriftsteadItemCatalog::GetDefinitions())
            {
                const FString Name = TEXT("T_Icon_") + Definition.ItemId.ToString();
                const FString Path = FString::Printf(TEXT("/Game/Driftstead/UI/Icons/%s.%s"), *Name, *Name);
                const auto* Texture = LoadObject<UTexture2D>(nullptr, *Path);
                Check(Texture && Texture->GetSizeX() > 0 && Texture->GetSizeY() > 0, *FString::Printf(TEXT("cooked inventory icon is available: %s"), *Definition.ItemId.ToString()));
                if (Texture && Texture->GetSizeX() > 0 && Texture->GetSizeY() > 0) ++IconCount;
            }
            Check(IconCount == 10, TEXT("all ten inventory item icons load"));
            SetRaftLevelFromSave(4);
            Player->SetCurrentFloor(1);
            Quest->NotifyEvent(EDriftsteadQuestStep::Move);
            Quest->NotifyEvent(EDriftsteadQuestStep::ChargeHook);
            Inventory->AddResource(TEXT("Wood"), 37);
            Inventory->TryAddItem(TEXT("Rope"), 2);
            Snapshot();
            Pause();
            Player->SelectPauseOption(1);
            Check(HUD->IsPausePanelOpen() && UGameplayStatics::IsGamePaused(this), TEXT("save action keeps the existing paused scene"));
            CheckSavedCargo();
            Player->SelectPauseOption(0);
            Check(!HUD->IsPausePanelOpen() && !UGameplayStatics::IsGamePaused(this), TEXT("resume action unpauses without returning to menu"));
            Pause();
            Player->SelectPauseOption(2);
            Advance();
            break;
        }
        case 1:
            if (!HUD->IsMainMenuOpen()) break;
            CheckReturned();
            CheckSavedCargo();
            Resume();
            Check(GetRaftLevel() == 4 && Player->GetCurrentFloor() == 1 && Inventory->GetResource(TEXT("Wood")) == State->Wood && Inventory->CountItem(TEXT("Rope")) == State->Rope, TEXT("idle return/continue preserves floor, raft and resources"));
            Check(Quest->GetCompletedObjectives() == State->Completed && !Instance->IsShowcaseMode(), TEXT("idle continue preserves quests and normal mode"));
            SetRaftLevelFromSave(1);
            Player->SetCurrentFloor(0);
            Player->SetActorLocation(FVector(0, 0, 125));
            Hook->StartCharging();
            Check(Hook->GetHookState() == EHookState::Charging, TEXT("charging branch begins with a real charged hook"));
            Pause();
            Player->SelectPauseOption(2);
            Advance();
            break;
        case 2:
            if (!HUD->IsMainMenuOpen()) break;
            CheckReturned();
            Check(Inventory->GetResource(TEXT("Wood")) == State->Wood && Inventory->CountItem(TEXT("Rope")) == State->Rope, TEXT("cancelling charge does not create or lose inventory"));
            Resume();
            Advance();
            break;
        case 3:
            if (Hook->GetHookState() != EHookState::Idle) break;
            Snapshot();
            SpawnCargo(TEXT("Rope"), 310, TEXT("Driftwood"), 490);
            Hook->SetAimDirection(FVector(0, 1, 0));
            Hook->StartCharging();
            Advance();
            break;
        case 4:
            if (Hook->GetChargeAlpha() < .99f) break;
            Hook->ReleaseHook();
            Check(Hook->GetHookState() == EHookState::Flying, TEXT("flying return test launches a full-range hook"));
            Advance();
            break;
        case 5:
            if (InStepSeconds < .5) break;
            Check(Hook->GetHookState() == EHookState::Flying && Hook->GetAttachedCount() == 0, TEXT("return request interrupts outbound flight before attachment"));
            Pause();
            Player->SelectPauseOption(2);
            Check(Player->IsReturnToMenuPending() && !UGameplayStatics::IsGamePaused(this), TEXT("flying return resumes world until physical recovery completes"));
            Advance();
            break;
        case 6:
            if (!HUD->IsMainMenuOpen()) break;
            CheckReturned();
            Check(Inventory->CountItem(TEXT("Rope")) == State->Rope + 1 && Inventory->CountItem(TEXT("Driftwood")) == State->Driftwood + 1 && Inventory->GetResource(TEXT("Wood")) == State->Wood + 2, TEXT("outbound return collects both crossed items before saving"));
            Check(Quest->GetEventCount(EDriftsteadQuestStep::SalvageItem) == State->Salvaged + 2, TEXT("outbound return credits each salvage exactly once"));
            CheckCargoGone();
            CheckSavedCargo();
            Resume();
            Advance();
            break;
        case 7:
            if (Hook->GetHookState() != EHookState::Idle) break;
            Snapshot();
            SpawnCargo(TEXT("Rope"), 710, TEXT("ScrapMetal"), 420);
            Hook->SetAimDirection(FVector(0, 1, 0));
            Hook->StartCharging();
            Advance();
            break;
        case 8:
            if (Hook->GetChargeAlpha() < .99f) break;
            Hook->ReleaseHook();
            Advance();
            break;
        case 9:
            if (Hook->GetAttachedCount() < 1) break;
            Check(Hook->GetHookState() == EHookState::Attached, TEXT("attached return test already carries physical cargo"));
            Pause();
            Player->SelectPauseOption(2);
            Check(Player->IsReturnToMenuPending() && !UGameplayStatics::IsGamePaused(this), TEXT("attached cargo keeps travelling before the menu opens"));
            Advance();
            break;
        case 10:
            if (!HUD->IsMainMenuOpen()) break;
            CheckReturned();
            Check(Inventory->CountItem(TEXT("Rope")) == State->Rope + 1 && Inventory->CountItem(TEXT("ScrapMetal")) == State->ScrapMetal + 1 && Inventory->GetResource(TEXT("Metal")) == State->Metal + 1, TEXT("attached return retains first cargo and catches remaining cargo"));
            Check(Quest->GetEventCount(EDriftsteadQuestStep::SalvageItem) == State->Salvaged + 2, TEXT("attached return credits cargo once"));
            CheckCargoGone();
            CheckSavedCargo();
            Resume();
            HUD->SetInventoryOpen(true);
            HUD->OpenMainMenu();
            Player->SelectMenuOption(0);
            Check(!HUD->IsInventoryOpen() && !HUD->IsPausePanelOpen() && !HUD->IsMainMenuOpen() && !HUD->IsDraggingInventoryItem() && !HUD->GetSelectedInventoryItemId().IsValid(), TEXT("new voyage resets panels, drag and selection state"));
            Check(!Player->HasSuspendedSession() && !Player->IsReturnToMenuPending() && !UGameplayStatics::IsGamePaused(this), TEXT("new voyage drops suspended/pending flags"));
            Check(GetRaftLevel() == 1 && Player->GetCurrentFloor() == 0 && Inventory->GetEntries().IsEmpty() && Inventory->GetRecoveryBasket().IsEmpty() && Inventory->GetResource(TEXT("Wood")) == 2 && Quest->GetCompletedObjectives() == 0, TEXT("new voyage restores starter inventory, floor and quests"));
            HUD->OpenMainMenu();
            Player->SelectMenuOption(2);
            Check(Instance->IsShowcaseMode() && GetRaftLevel() == 10, TEXT("showcase starts as its own mode"));
            Inventory->AddResource(TEXT("Wood"), 7);
            Snapshot();
            Pause();
            Player->SelectPauseOption(2);
            Advance();
            break;
        case 11:
            if (!HUD->IsMainMenuOpen()) break;
            CheckReturned();
            Resume();
            Check(Instance->IsShowcaseMode() && GetRaftLevel() == 10 && Inventory->GetResource(TEXT("Wood")) == State->Wood, TEXT("continue resumes suspended showcase instead of loading normal mode"));
            return Finish();
        default:
            Check(false, TEXT("unrecognized menu test step"));
            return Finish();
        }
        return true;
    }), .1f);
}
