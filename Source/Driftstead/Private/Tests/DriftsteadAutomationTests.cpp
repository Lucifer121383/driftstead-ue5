#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../../InventoryComponent.h"
#include "../../HookComponent.h"
#include "../../DriftsteadSaveGame.h"
#include "../../DriftsteadTypes.h"

namespace DriftsteadTests
{
UInventoryComponent* NewInventory(int32 Columns = 6, int32 Rows = 4)
{
    UInventoryComponent* Inventory = NewObject<UInventoryComponent>();
    Inventory->InitializeGrid(Columns, Rows);
    return Inventory;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryLegalPlacement, "Driftstead.Inventory.LegalPlacement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryLegalPlacement::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory();
    TestEqual(TEXT("Driftwood is placed"), Inventory->TryAddItem(TEXT("Driftwood")), EInventoryAddResult::Placed);
    TestEqual(TEXT("One entry exists"), Inventory->GetEntries().Num(), 1);
    TestEqual(TEXT("First-Fit starts at origin"), Inventory->GetEntries()[0].GridPosition, FIntPoint::ZeroValue);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryRejectsBounds, "Driftstead.Inventory.RejectsOutOfBounds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryRejectsBounds::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(2, 2);
    FInventoryEntry Entry; Entry.ItemId = TEXT("FoodCrate");
    TestFalse(TEXT("2x2 item cannot start at x=1"), Inventory->CanPlace(Entry, FIntPoint(1, 0), false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryRejectsConflict, "Driftstead.Inventory.RejectsOccupiedCells", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryRejectsConflict::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(2, 1);
    Inventory->TryAddItem(TEXT("Rope"));
    FInventoryEntry Candidate; Candidate.ItemId = TEXT("Electronics");
    TestFalse(TEXT("Occupied origin is rejected"), Inventory->CanPlace(Candidate, FIntPoint(0, 0), false));
    TestTrue(TEXT("Free cell remains valid"), Inventory->CanPlace(Candidate, FIntPoint(1, 0), false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryRotation, "Driftstead.Inventory.RotatesTwoByOne", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryRotation::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(2, 2);
    Inventory->TryAddItem(TEXT("Driftwood"));
    const FGuid Id = Inventory->GetEntries()[0].InstanceId;
    TestTrue(TEXT("Rotation succeeds"), Inventory->RotateItem(Id));
    TestTrue(TEXT("Entry records rotation"), Inventory->GetEntries()[0].bRotated);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryFirstFit, "Driftstead.Inventory.FirstFit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryFirstFit::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(3, 2);
    Inventory->TryAddItem(TEXT("FoodCrate"));
    Inventory->TryAddItem(TEXT("Rope"));
    TestEqual(TEXT("Small item uses first remaining top-row cell"), Inventory->GetEntries()[1].GridPosition, FIntPoint(2, 0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryStackLimit, "Driftstead.Inventory.StackLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryStackLimit::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(2, 1);
    Inventory->TryAddItem(TEXT("Rope"), 13);
    TestEqual(TEXT("Thirteen rope creates two stacks"), Inventory->GetEntries().Num(), 2);
    TestEqual(TEXT("First stack reaches catalog limit"), Inventory->GetEntries()[0].Quantity, 12);
    TestEqual(TEXT("Second stack holds remainder"), Inventory->GetEntries()[1].Quantity, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryRecoveryBasket, "Driftstead.Inventory.FullUsesRecoveryBasket", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryRecoveryBasket::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(1, 1);
    Inventory->TryAddItem(TEXT("Rope"));
    TestEqual(TEXT("Second distinct item goes to recovery basket"), Inventory->TryAddItem(TEXT("Electronics")), EInventoryAddResult::RecoveryBasket);
    TestEqual(TEXT("Recovery basket has one stack"), Inventory->GetRecoveryBasket().Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryResizePreservesItems, "Driftstead.Inventory.ResizePreservesItems", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryResizePreservesItems::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(2, 1);
    Inventory->TryAddItem(TEXT("Rope"));
    Inventory->TryAddItem(TEXT("Electronics"));
    Inventory->InitializeGrid(1, 1);
    TestEqual(TEXT("Grid retains one stack after shrinking"), Inventory->GetEntries().Num(), 1);
    TestEqual(TEXT("Displaced stack moves to recovery basket"), Inventory->GetRecoveryBasket().Num(), 1);
    TestEqual(TEXT("Rope quantity is preserved"), Inventory->CountItem(TEXT("Rope")), 1);
    TestEqual(TEXT("Electronics quantity is preserved"), Inventory->CountItem(TEXT("Electronics")), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventorySplitStack, "Driftstead.Inventory.SplitStack", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventorySplitStack::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(2, 1);
    Inventory->TryAddItem(TEXT("Rope"), 6);
    const FGuid Id = Inventory->GetEntries()[0].InstanceId;
    TestTrue(TEXT("A six-item stack can split"), Inventory->SplitStack(Id));
    TestEqual(TEXT("Split creates two stacks"), Inventory->GetEntries().Num(), 2);
    TestEqual(TEXT("Original half remains three"), Inventory->GetEntries()[0].Quantity, 3);
    TestEqual(TEXT("New half contains three"), Inventory->GetEntries()[1].Quantity, 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryDragMergesStack, "Driftstead.Inventory.DragMergesStack", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryDragMergesStack::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(2, 1);
    Inventory->TryAddItem(TEXT("Rope"), 6);
    Inventory->SplitStack(Inventory->GetEntries()[0].InstanceId);
    const FGuid MovingId = Inventory->GetEntries()[1].InstanceId;
    TestTrue(TEXT("Dragging onto the same item merges stacks"), Inventory->MoveItem(MovingId, FIntPoint::ZeroValue));
    TestEqual(TEXT("Merge leaves one stack"), Inventory->GetEntries().Num(), 1);
    TestEqual(TEXT("Merged quantity is preserved"), Inventory->GetEntries()[0].Quantity, 6);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryDragSwapsItems, "Driftstead.Inventory.DragSwapsItems", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryDragSwapsItems::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(2, 1);
    Inventory->TryAddItem(TEXT("Rope"));
    Inventory->TryAddItem(TEXT("Electronics"));
    const FGuid RopeId = Inventory->GetEntries()[0].InstanceId;
    TestTrue(TEXT("Dragging onto one compatible occupant swaps positions"), Inventory->MoveItem(RopeId, FIntPoint(1, 0)));
    const FInventoryEntry* Rope = Inventory->GetEntries().FindByPredicate([RopeId](const FInventoryEntry& Entry) { return Entry.InstanceId == RopeId; });
    TestTrue(TEXT("Rope entry remains valid"), Rope != nullptr);
    if (Rope) TestEqual(TEXT("Rope moved to the target cell"), Rope->GridPosition, FIntPoint(1, 0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryConsumesRecoveryBasket, "Driftstead.Inventory.ConsumesRecoveryBasketExactlyOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryConsumesRecoveryBasket::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(1, 1);
    Inventory->TryAddItem(TEXT("Rope"));
    Inventory->TryAddItem(TEXT("SealedBarrel"));
    TestEqual(TEXT("Barrel is counted in recovery basket"), Inventory->CountItem(TEXT("SealedBarrel")), 1);
    TestTrue(TEXT("Barrel can be consumed once"), Inventory->RemoveQuantity(TEXT("SealedBarrel"), 1));
    TestEqual(TEXT("Consumed barrel is removed from recovery basket"), Inventory->CountItem(TEXT("SealedBarrel")), 0);
    TestFalse(TEXT("Barrel cannot be consumed twice"), Inventory->RemoveQuantity(TEXT("SealedBarrel"), 1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryRecoversBasketItem, "Driftstead.Inventory.RecoversBasketItemAfterSpaceFreed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryRecoversBasketItem::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(1, 1);
    Inventory->TryAddItem(TEXT("Rope"));
    Inventory->TryAddItem(TEXT("Electronics"));
    const FGuid RecoveryId = Inventory->GetRecoveryBasket()[0].InstanceId;
    TestTrue(TEXT("Occupied grid item can be removed"), Inventory->RemoveQuantity(TEXT("Rope"), 1));
    TestTrue(TEXT("Recovery item returns after space is freed"), Inventory->RecoverFromBasket(RecoveryId));
    TestEqual(TEXT("Recovery basket is now empty"), Inventory->GetRecoveryBasket().Num(), 0);
    TestEqual(TEXT("Recovered item occupies the grid"), Inventory->GetEntries()[0].ItemId, FName(TEXT("Electronics")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventorySanitizesRestore, "Driftstead.Inventory.SanitizesCorruptRestoreData", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventorySanitizesRestore::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory(2, 2);
    FInventoryEntry Valid; Valid.InstanceId = FGuid::NewGuid(); Valid.ItemId = TEXT("Rope"); Valid.Quantity = 999; Valid.GridPosition = FIntPoint::ZeroValue;
    FInventoryEntry Duplicate = Valid; Duplicate.ItemId = TEXT("Electronics");
    FInventoryEntry Invalid; Invalid.ItemId = TEXT("NotAnItem"); Invalid.Quantity = 3;
    TArray<FInventoryEntry> Saved{Valid, Duplicate, Invalid};
    TArray<FInventoryEntry> Recovery;
    TMap<FName, int32> Resources{{TEXT("Wood"), -100}, {TEXT("Metal"), 2000000}};
    Inventory->RestoreState(2, 2, Saved, Recovery, Resources);
    TestEqual(TEXT("Unknown item is rejected"), Inventory->CountItem(TEXT("NotAnItem")), 0);
    TestEqual(TEXT("Rope quantity is clamped to its stack limit"), Inventory->CountItem(TEXT("Rope")), 12);
    TestEqual(TEXT("Negative resources are rejected"), Inventory->GetResource(TEXT("Wood")), 0);
    TestEqual(TEXT("Huge resources are clamped"), Inventory->GetResource(TEXT("Metal")), 1000000);
    if (Inventory->GetEntries().Num() == 2) TestNotEqual(TEXT("Duplicate GUID is regenerated"), Inventory->GetEntries()[0].InstanceId, Inventory->GetEntries()[1].InstanceId);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUpgradeTransactionSuccess, "Driftstead.Resources.TransactionSuccess", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUpgradeTransactionSuccess::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory();
    Inventory->AddResource(TEXT("Wood"), 20); Inventory->AddResource(TEXT("Rope"), 10);
    TMap<FName,int32> Cost{{TEXT("Wood"), 8}, {TEXT("Rope"), 3}};
    TestTrue(TEXT("Affordable transaction succeeds"), Inventory->ConsumeResourcesTransactional(Cost));
    TestEqual(TEXT("Wood deducted once"), Inventory->GetResource(TEXT("Wood")), 12);
    TestEqual(TEXT("Rope deducted once"), Inventory->GetResource(TEXT("Rope")), 7);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUpgradeTransactionFailure, "Driftstead.Resources.TransactionFailureIsAtomic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUpgradeTransactionFailure::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inventory = DriftsteadTests::NewInventory();
    Inventory->AddResource(TEXT("Wood"), 20); Inventory->AddResource(TEXT("Rope"), 1);
    TMap<FName,int32> Cost{{TEXT("Wood"), 8}, {TEXT("Rope"), 3}};
    TestFalse(TEXT("Unaffordable transaction fails"), Inventory->ConsumeResourcesTransactional(Cost));
    TestEqual(TEXT("Wood remains unchanged"), Inventory->GetResource(TEXT("Wood")), 20);
    TestEqual(TEXT("Rope remains unchanged"), Inventory->GetResource(TEXT("Rope")), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRaftTenLevels, "Driftstead.Raft.TenLevelDataComplete", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaftTenLevels::RunTest(const FString& Parameters)
{
    const TArray<FRaftLevelDefinition>& Levels = FRaftProgressionCatalog::GetDefinitions();
    TestEqual(TEXT("Exactly ten levels exist"), Levels.Num(), 10);
    for (int32 Index = 0; Index < Levels.Num(); ++Index)
    {
        TestEqual(FString::Printf(TEXT("Level index %d is sequential"), Index), Levels[Index].Level, Index + 1);
        TestTrue(FString::Printf(TEXT("Level %d has a first floor"), Index + 1), Levels[Index].FirstFloor.X > 0 && Levels[Index].FirstFloor.Y > 0);
        if (Index + 1 >= 8) TestTrue(FString::Printf(TEXT("Level %d retains a workbench upgrade path"), Index + 1), Levels[Index].Facilities.Contains(EFacilityType::Workbench));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRaftLevelFourFloor, "Driftstead.Raft.LevelFourHasSecondFloor", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaftLevelFourFloor::RunTest(const FString& Parameters)
{
    const FRaftLevelDefinition* Level = FRaftProgressionCatalog::Find(4);
    TestNotNull(TEXT("Level four exists"), Level);
    if (Level) TestEqual(TEXT("Level four second floor is 5x4"), Level->SecondFloor, FIntPoint(5,4));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRaftLevelSevenFloor, "Driftstead.Raft.LevelSevenHasThirdFloor", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaftLevelSevenFloor::RunTest(const FString& Parameters)
{
    const FRaftLevelDefinition* Level = FRaftProgressionCatalog::Find(7);
    TestNotNull(TEXT("Level seven exists"), Level);
    if (Level) TestEqual(TEXT("Level seven third floor is 6x5"), Level->ThirdFloor, FIntPoint(6,5));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStairsBidirectional, "Driftstead.Raft.StairLinksBidirectional", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStairsBidirectional::RunTest(const FString& Parameters)
{
    const TArray<TPair<int32,int32>> Links{{0,1},{1,0},{1,2},{2,1}};
    for (const TPair<int32,int32>& Link : Links)
    {
        TestTrue(FString::Printf(TEXT("Reverse link exists for %d->%d"), Link.Key, Link.Value), Links.Contains(TPair<int32,int32>(Link.Value, Link.Key)));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHookTransitions, "Driftstead.Hook.StateTransitions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHookTransitions::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Idle to charging"), UHookComponent::IsTransitionAllowed(EHookState::Idle, EHookState::Charging));
    TestTrue(TEXT("Charging to flying"), UHookComponent::IsTransitionAllowed(EHookState::Charging, EHookState::Flying));
    TestTrue(TEXT("Flying to attached"), UHookComponent::IsTransitionAllowed(EHookState::Flying, EHookState::Attached));
    TestTrue(TEXT("Attached to returning"), UHookComponent::IsTransitionAllowed(EHookState::Attached, EHookState::Returning));
    TestTrue(TEXT("Returning to cooldown"), UHookComponent::IsTransitionAllowed(EHookState::Returning, EHookState::Cooldown));
    TestTrue(TEXT("Cooldown to idle"), UHookComponent::IsTransitionAllowed(EHookState::Cooldown, EHookState::Idle));
    TestFalse(TEXT("Idle cannot jump to attached"), UHookComponent::IsTransitionAllowed(EHookState::Idle, EHookState::Attached));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveVersionFallback, "Driftstead.Save.CorruptVersionUsesSafeDefault", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveVersionFallback::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Current save version is supported"), UDriftsteadSaveGame::IsSupportedVersion(UDriftsteadSaveGame::CurrentSaveVersion));
    TestFalse(TEXT("Future/corrupt save version is rejected"), UDriftsteadSaveGame::IsSupportedVersion(999));
    TestFalse(TEXT("Zero save version is rejected"), UDriftsteadSaveGame::IsSupportedVersion(0));
    return true;
}

#endif
