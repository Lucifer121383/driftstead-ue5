#include "DriftsteadGameInstance.h"
#include "DriftsteadSaveGame.h"
#include "DriftsteadCharacter.h"
#include "DriftsteadGameMode.h"
#include "DriftsteadQuestSubsystem.h"
#include "InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HookComponent.h"

FString UDriftsteadGameInstance::GetActiveSlot() const
{
    if (bAutomationMode) return TEXT("Driftstead_Automation");
    return bShowcaseMode ? TEXT("Driftstead_Showcase") : TEXT("Driftstead_Normal");
}

bool UDriftsteadGameInstance::SaveCurrentGame()
{
    UWorld* World = GetWorld();
    if (!World) return false;
    ADriftsteadGameMode* GameMode = World->GetAuthGameMode<ADriftsteadGameMode>();
    ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
    if (!GameMode || !Character) return false;

    const EHookState HookState = Character->GetHook()->GetHookState();
    if (HookState != EHookState::Idle && HookState != EHookState::Cooldown)
    {
        Character->ShowFeedback(FText::FromString(TEXT("请等钩子与物资回到脚下，再保存航程。")), FLinearColor::Yellow);
        return false;
    }

    UDriftsteadSaveGame* Save = Cast<UDriftsteadSaveGame>(UGameplayStatics::CreateSaveGameObject(UDriftsteadSaveGame::StaticClass()));
    if (!Save) return false;
    Save->bShowcaseMode = bShowcaseMode;
    Save->RaftLevel = GameMode->GetRaftLevel();
    Save->CurrentFloor = Character->GetCurrentFloor();
    Save->PlayerLocation = Character->GetActorLocation();
    Save->InventoryColumns = Character->GetInventory()->GetColumns();
    Save->InventoryRows = Character->GetInventory()->GetRows();
    Save->InventoryEntries = Character->GetInventory()->GetEntries();
    Save->RecoveryBasket = Character->GetInventory()->GetRecoveryBasket();
    Save->Resources = Character->GetInventory()->GetResources();
    Save->FacilityStates = GameMode->CaptureFacilityStates();
    if (UDriftsteadQuestSubsystem* Quest = GetSubsystem<UDriftsteadQuestSubsystem>())
    {
        Save->CurrentQuest = Quest->GetCurrentStep();
        Save->bTutorialComplete = Quest->IsComplete();
        Save->QuestEvents = Quest->GetEventCounts();
    }
    // Retain the previous valid checkpoint before replacing the active slot.
    if (auto* Previous = Cast<UDriftsteadSaveGame>(UGameplayStatics::LoadGameFromSlot(GetActiveSlot(),0)))
        if (UDriftsteadSaveGame::IsSupportedVersion(Previous->SaveVersion))
            UGameplayStatics::SaveGameToSlot(Previous,GetActiveSlot()+TEXT("_Backup"),0);
    const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, GetActiveSlot(), 0);
    Character->ShowFeedback(bSaved ? NSLOCTEXT("Driftstead", "Saved", "游戏已保存。") : NSLOCTEXT("Driftstead", "SaveFailed", "保存失败。"), bSaved ? FLinearColor::Green : FLinearColor::Red);
    return bSaved;
}

bool UDriftsteadGameInstance::LoadCurrentGame()
{
    UDriftsteadSaveGame* Save = Cast<UDriftsteadSaveGame>(UGameplayStatics::LoadGameFromSlot(GetActiveSlot(), 0));
    bool bUsedBackup = false;
    if (!Save || !UDriftsteadSaveGame::IsSupportedVersion(Save->SaveVersion))
    {
        Save = Cast<UDriftsteadSaveGame>(UGameplayStatics::LoadGameFromSlot(GetActiveSlot()+TEXT("_Backup"),0));
        bUsedBackup = true;
    }
    if (!Save || !UDriftsteadSaveGame::IsSupportedVersion(Save->SaveVersion)) return false;
    UWorld* World = GetWorld();
    ADriftsteadGameMode* GameMode = World ? World->GetAuthGameMode<ADriftsteadGameMode>() : nullptr;
    ADriftsteadCharacter* Character = World ? Cast<ADriftsteadCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)) : nullptr;
    if (!GameMode || !Character) return false;
    GameMode->SetRaftLevelFromSave(Save->RaftLevel);
    if (Save->SaveVersion >= 2) GameMode->RestoreFacilityStates(Save->FacilityStates);
    Character->GetInventory()->RestoreState(Save->InventoryColumns, Save->InventoryRows, Save->InventoryEntries, Save->RecoveryBasket, Save->Resources);
    const int32 SafeFloor = FMath::Clamp(Save->CurrentFloor, 0, GameMode->GetMaximumFloor());
    FVector SafeLocation = Save->PlayerLocation;
    const bool bInvalidLocation = SafeLocation.ContainsNaN()
        || !FMath::IsFinite(SafeLocation.X) || !FMath::IsFinite(SafeLocation.Y) || !FMath::IsFinite(SafeLocation.Z)
        || FVector2D(SafeLocation.X, SafeLocation.Y).SizeSquared() > FMath::Square(2500.0f);
    if (bInvalidLocation)
    {
        SafeLocation.X = 0.0f;
        SafeLocation.Y = 0.0f;
    }
    SafeLocation.Z = 110.0f + SafeFloor * 340.0f;
    Character->SetActorLocation(SafeLocation, false, nullptr, ETeleportType::TeleportPhysics);
    Character->SetCurrentFloor(SafeFloor, false);
    if (UDriftsteadQuestSubsystem* Quest = GetSubsystem<UDriftsteadQuestSubsystem>())
    {
        if (Save->SaveVersion >= 3) Quest->RestoreEvents(Save->QuestEvents);
        else Quest->RestoreQuest(Save->CurrentQuest, Save->bTutorialComplete);
    }
    Character->ShowFeedback(bUsedBackup ? FText::FromString(TEXT("主存档不可用，已恢复上一份安全备份。")) : NSLOCTEXT("Driftstead", "Loaded", "存档已加载。"), FLinearColor::Green);
    return true;
}

void UDriftsteadGameInstance::StartNewGame(bool bInShowcaseMode)
{
    bShowcaseMode = bInShowcaseMode;
    if (UDriftsteadQuestSubsystem* Quest = GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->ResetQuest();
}

void UDriftsteadGameInstance::ResetAllSaves()
{
    UGameplayStatics::DeleteGameInSlot(TEXT("Driftstead_Normal"), 0);
    UGameplayStatics::DeleteGameInSlot(TEXT("Driftstead_Showcase"), 0);
    UGameplayStatics::DeleteGameInSlot(TEXT("Driftstead_Normal_Backup"),0);
    UGameplayStatics::DeleteGameInSlot(TEXT("Driftstead_Showcase_Backup"),0);
    if (ADriftsteadCharacter* Character = GetWorld() ? Cast<ADriftsteadCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)) : nullptr)
    {
        Character->ShowFeedback(NSLOCTEXT("Driftstead", "SavesReset", "普通模式与展示模式存档已重置。"), FLinearColor::Yellow);
    }
}

void UDriftsteadGameInstance::DeleteAutomationSave()
{
    UGameplayStatics::DeleteGameInSlot(TEXT("Driftstead_Automation"), 0);
    UGameplayStatics::DeleteGameInSlot(TEXT("Driftstead_Automation_Backup"), 0);
}
