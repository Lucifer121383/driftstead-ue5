#include "InventoryComponent.h"
#include "Driftstead.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::InitializeGrid(int32 Columns, int32 Rows)
{
    GridColumns = FMath::Clamp(Columns, 1, 20);
    GridRows = FMath::Clamp(Rows, 1, 20);

    // A debug level change or a loaded progression state can shrink the grid.
    // Repack displaced entries and preserve overflow in the recovery basket
    // instead of silently deleting player-owned items.
    TArray<FInventoryEntry> PreviousEntries = MoveTemp(Entries);
    Entries.Reset();
    for (FInventoryEntry& Entry : PreviousEntries)
    {
        if (CanPlace(Entry, Entry.GridPosition, Entry.bRotated))
        {
            Entries.Add(Entry);
            continue;
        }

        FIntPoint NewPosition;
        bool bNewRotation = false;
        if (FindFirstFit(Entry.ItemId, NewPosition, bNewRotation))
        {
            Entry.GridPosition = NewPosition;
            Entry.bRotated = bNewRotation;
            Entries.Add(Entry);
        }
        else
        {
            Entry.GridPosition = FIntPoint::ZeroValue;
            Entry.bRotated = false;
            RecoveryBasket.Add(Entry);
        }
    }
    OnInventoryChanged.Broadcast();
}

FIntPoint UInventoryComponent::GetEffectiveFootprint(const FInventoryEntry& Entry, bool bRotated) const
{
    const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(Entry.ItemId);
    if (!Definition) return FIntPoint(1, 1);
    return bRotated ? FIntPoint(Definition->Footprint.Y, Definition->Footprint.X) : Definition->Footprint;
}

bool UInventoryComponent::CanPlace(const FInventoryEntry& Entry, FIntPoint Position, bool bRotated, const FGuid& IgnoreInstance) const
{
    const FIntPoint Size = GetEffectiveFootprint(Entry, bRotated);
    if (Position.X < 0 || Position.Y < 0 || Position.X + Size.X > GridColumns || Position.Y + Size.Y > GridRows)
    {
        return false;
    }

    const FIntRect Candidate(Position, Position + Size);
    for (const FInventoryEntry& Existing : Entries)
    {
        if (IgnoreInstance.IsValid() && Existing.InstanceId == IgnoreInstance) continue;
        const FIntPoint ExistingSize = GetEffectiveFootprint(Existing, Existing.bRotated);
        const FIntRect Occupied(Existing.GridPosition, Existing.GridPosition + ExistingSize);
        if (Candidate.Intersect(Occupied)) return false;
    }
    return true;
}

bool UInventoryComponent::FindFirstFit(FName ItemId, FIntPoint& OutPosition, bool& bOutRotated) const
{
    if (!FDriftsteadItemCatalog::Find(ItemId)) return false;
    FInventoryEntry Candidate;
    Candidate.ItemId = ItemId;
    for (int32 RotationPass = 0; RotationPass < 2; ++RotationPass)
    {
        const bool bRotated = RotationPass == 1;
        for (int32 Y = 0; Y < GridRows; ++Y)
        {
            for (int32 X = 0; X < GridColumns; ++X)
            {
                if (CanPlace(Candidate, FIntPoint(X, Y), bRotated))
                {
                    OutPosition = FIntPoint(X, Y);
                    bOutRotated = bRotated;
                    return true;
                }
            }
        }
    }
    return false;
}

EInventoryAddResult UInventoryComponent::TryAddItem(FName ItemId, int32 Quantity)
{
    const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(ItemId);
    if (!Definition || Quantity <= 0)
    {
        OnInventoryFeedback.Broadcast(EInventoryAddResult::InvalidItem, ItemId);
        return EInventoryAddResult::InvalidItem;
    }

    bool bStackedAny = false;
    int32 Remaining = Quantity;
    for (FInventoryEntry& Entry : Entries)
    {
        if (Entry.ItemId == ItemId && Entry.Quantity < Definition->StackLimit)
        {
            const int32 Added = FMath::Min(Remaining, Definition->StackLimit - Entry.Quantity);
            Entry.Quantity += Added;
            Remaining -= Added;
            bStackedAny = bStackedAny || Added > 0;
            if (Remaining == 0) break;
        }
    }

    EInventoryAddResult Result = bStackedAny ? EInventoryAddResult::Stacked : EInventoryAddResult::Placed;
    while (Remaining > 0)
    {
        FIntPoint Position;
        bool bRotated = false;
        if (!FindFirstFit(ItemId, Position, bRotated))
        {
            while (Remaining > 0)
            {
                FInventoryEntry Recovery;
                Recovery.InstanceId = FGuid::NewGuid();
                Recovery.ItemId = ItemId;
                Recovery.Quantity = FMath::Min(Remaining, Definition->StackLimit);
                RecoveryBasket.Add(Recovery);
                Remaining -= Recovery.Quantity;
            }
            Result = EInventoryAddResult::RecoveryBasket;
            break;
        }
        FInventoryEntry NewEntry;
        NewEntry.InstanceId = FGuid::NewGuid();
        NewEntry.ItemId = ItemId;
        NewEntry.GridPosition = Position;
        NewEntry.bRotated = bRotated;
        NewEntry.Quantity = FMath::Min(Remaining, Definition->StackLimit);
        Remaining -= NewEntry.Quantity;
        Entries.Add(NewEntry);
    }

    OnInventoryChanged.Broadcast();
    OnInventoryFeedback.Broadcast(Result, ItemId);
    return Result;
}

FInventoryEntry* UInventoryComponent::FindEntry(FGuid InstanceId)
{
    return Entries.FindByPredicate([InstanceId](const FInventoryEntry& Entry) { return Entry.InstanceId == InstanceId; });
}

bool UInventoryComponent::MoveItem(FGuid InstanceId, FIntPoint NewPosition)
{
    FInventoryEntry* Entry = FindEntry(InstanceId);
    if (!Entry) return false;
    if (CanPlace(*Entry, NewPosition, Entry->bRotated, InstanceId))
    {
        Entry->GridPosition = NewPosition;
        OnInventoryChanged.Broadcast();
        return true;
    }

    const FIntPoint MovingSize = GetEffectiveFootprint(*Entry, Entry->bRotated);
    if (NewPosition.X < 0 || NewPosition.Y < 0 || NewPosition.X + MovingSize.X > GridColumns || NewPosition.Y + MovingSize.Y > GridRows) return false;
    const FIntRect Destination(NewPosition, NewPosition + MovingSize);
    FInventoryEntry* Target = nullptr;
    for (FInventoryEntry& Existing : Entries)
    {
        if (Existing.InstanceId == InstanceId) continue;
        const FIntPoint ExistingSize = GetEffectiveFootprint(Existing, Existing.bRotated);
        if (!Destination.Intersect(FIntRect(Existing.GridPosition, Existing.GridPosition + ExistingSize))) continue;
        if (Target) return false;
        Target = &Existing;
    }
    if (!Target) return false;

    const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(Entry->ItemId);
    if (Definition && Target->ItemId == Entry->ItemId && Target->Quantity < Definition->StackLimit)
    {
        const int32 Moved = FMath::Min(Entry->Quantity, Definition->StackLimit - Target->Quantity);
        Target->Quantity += Moved;
        Entry->Quantity -= Moved;
        if (Entry->Quantity == 0) Entries.RemoveAll([InstanceId](const FInventoryEntry& Candidate) { return Candidate.InstanceId == InstanceId; });
        OnInventoryChanged.Broadcast();
        return Moved > 0;
    }

    const FGuid TargetId = Target->InstanceId;
    const FIntPoint OriginalPosition = Entry->GridPosition;
    auto CanPlaceIgnoringPair = [this, InstanceId, TargetId](const FInventoryEntry& Candidate, FIntPoint Position)
    {
        const FIntPoint Size = GetEffectiveFootprint(Candidate, Candidate.bRotated);
        if (Position.X < 0 || Position.Y < 0 || Position.X + Size.X > GridColumns || Position.Y + Size.Y > GridRows) return false;
        const FIntRect CandidateRect(Position, Position + Size);
        for (const FInventoryEntry& Existing : Entries)
        {
            if (Existing.InstanceId == InstanceId || Existing.InstanceId == TargetId) continue;
            const FIntPoint ExistingSize = GetEffectiveFootprint(Existing, Existing.bRotated);
            if (CandidateRect.Intersect(FIntRect(Existing.GridPosition, Existing.GridPosition + ExistingSize))) return false;
        }
        return true;
    };
    if (!CanPlaceIgnoringPair(*Entry, NewPosition) || !CanPlaceIgnoringPair(*Target, OriginalPosition)) return false;
    Entry->GridPosition = NewPosition;
    Target->GridPosition = OriginalPosition;
    OnInventoryChanged.Broadcast();
    return true;
}

bool UInventoryComponent::RotateItem(FGuid InstanceId)
{
    FInventoryEntry* Entry = FindEntry(InstanceId);
    if (!Entry || !CanPlace(*Entry, Entry->GridPosition, !Entry->bRotated, InstanceId)) return false;
    Entry->bRotated = !Entry->bRotated;
    OnInventoryChanged.Broadcast();
    return true;
}

bool UInventoryComponent::SplitStack(FGuid InstanceId, int32 Quantity)
{
    FInventoryEntry* Entry = FindEntry(InstanceId);
    if (!Entry || Entry->Quantity < 2) return false;
    const int32 SplitQuantity = Quantity > 0 ? FMath::Clamp(Quantity, 1, Entry->Quantity - 1) : Entry->Quantity / 2;
    FIntPoint Position;
    bool bRotated = false;
    if (!FindFirstFit(Entry->ItemId, Position, bRotated)) return false;
    FInventoryEntry Split = *Entry;
    Split.InstanceId = FGuid::NewGuid();
    Split.GridPosition = Position;
    Split.bRotated = bRotated;
    Split.Quantity = SplitQuantity;
    Entry->Quantity -= SplitQuantity;
    Entries.Add(Split);
    OnInventoryChanged.Broadcast();
    return true;
}

bool UInventoryComponent::RecoverFromBasket(FGuid InstanceId)
{
    const int32 Index = RecoveryBasket.IndexOfByPredicate([InstanceId](const FInventoryEntry& Entry)
    {
        return Entry.InstanceId == InstanceId;
    });
    if (Index == INDEX_NONE) return false;

    FInventoryEntry Candidate = RecoveryBasket[Index];
    FIntPoint Position;
    bool bRotated = false;
    if (!FindFirstFit(Candidate.ItemId, Position, bRotated)) return false;
    Candidate.GridPosition = Position;
    Candidate.bRotated = bRotated;
    Entries.Add(Candidate);
    RecoveryBasket.RemoveAt(Index);
    OnInventoryChanged.Broadcast();
    return true;
}

int32 UInventoryComponent::CountItem(FName ItemId) const
{
    int32 Total = 0;
    for (const FInventoryEntry& Entry : Entries) if (Entry.ItemId == ItemId) Total += Entry.Quantity;
    for (const FInventoryEntry& Entry : RecoveryBasket) if (Entry.ItemId == ItemId) Total += Entry.Quantity;
    return Total;
}

bool UInventoryComponent::RemoveQuantity(FName ItemId, int32 Quantity)
{
    if (Quantity <= 0 || CountItem(ItemId) < Quantity) return false;
    int32 Remaining = Quantity;
    for (int32 Index = Entries.Num() - 1; Index >= 0 && Remaining > 0; --Index)
    {
        FInventoryEntry& Entry = Entries[Index];
        if (Entry.ItemId != ItemId) continue;
        const int32 Removed = FMath::Min(Remaining, Entry.Quantity);
        Entry.Quantity -= Removed;
        Remaining -= Removed;
        if (Entry.Quantity == 0) Entries.RemoveAt(Index);
    }
    for (int32 Index = RecoveryBasket.Num() - 1; Index >= 0 && Remaining > 0; --Index)
    {
        FInventoryEntry& Entry = RecoveryBasket[Index];
        if (Entry.ItemId != ItemId) continue;
        const int32 Removed = FMath::Min(Remaining, Entry.Quantity);
        Entry.Quantity -= Removed;
        Remaining -= Removed;
        if (Entry.Quantity == 0) RecoveryBasket.RemoveAt(Index);
    }
    OnInventoryChanged.Broadcast();
    return Remaining == 0;
}

void UInventoryComponent::AddResource(FName ResourceId, int32 Amount)
{
    if (ResourceId.IsNone() || Amount == 0) return;
    Resources.FindOrAdd(ResourceId) = FMath::Max(0, Resources.FindRef(ResourceId) + Amount);
    OnInventoryChanged.Broadcast();
}

int32 UInventoryComponent::GetResource(FName ResourceId) const
{
    return Resources.FindRef(ResourceId);
}

bool UInventoryComponent::HasResources(const TMap<FName, int32>& Cost) const
{
    for (const TPair<FName, int32>& Pair : Cost)
    {
        if (Pair.Value < 0 || Resources.FindRef(Pair.Key) < Pair.Value) return false;
    }
    return true;
}

bool UInventoryComponent::ConsumeResourcesTransactional(const TMap<FName, int32>& Cost)
{
    if (!HasResources(Cost)) return false;
    for (const TPair<FName, int32>& Pair : Cost) Resources.FindOrAdd(Pair.Key) -= Pair.Value;
    OnInventoryChanged.Broadcast();
    return true;
}

void UInventoryComponent::AddTestResources(int32 Amount)
{
    for (const FName Resource : {FName(TEXT("Wood")), FName(TEXT("Rope")), FName(TEXT("Metal")), FName(TEXT("Cloth")), FName(TEXT("Seeds")), FName(TEXT("Food")), FName(TEXT("Water")), FName(TEXT("Parts")), FName(TEXT("Power"))})
    {
        AddResource(Resource, Amount);
    }
}

void UInventoryComponent::RestoreState(int32 Columns, int32 Rows, const TArray<FInventoryEntry>& SavedEntries, const TArray<FInventoryEntry>& SavedRecovery, const TMap<FName, int32>& SavedResources)
{
    GridColumns = FMath::Clamp(Columns, 1, 20);
    GridRows = FMath::Clamp(Rows, 1, 20);
    Entries.Reset();
    RecoveryBasket.Reset();
    Resources.Reset();

    for (const TPair<FName, int32>& Pair : SavedResources)
    {
        if (!Pair.Key.IsNone() && Pair.Value > 0)
        {
            Resources.Add(Pair.Key, FMath::Clamp(Pair.Value, 0, 1000000));
        }
    }

    TSet<FGuid> UsedIds;
    auto NormalizeId = [&UsedIds](FGuid Candidate)
    {
        if (!Candidate.IsValid() || UsedIds.Contains(Candidate)) Candidate = FGuid::NewGuid();
        UsedIds.Add(Candidate);
        return Candidate;
    };

    for (const FInventoryEntry& Saved : SavedEntries)
    {
        const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(Saved.ItemId);
        if (!Definition || Saved.Quantity <= 0) continue;
        FInventoryEntry Candidate = Saved;
        Candidate.InstanceId = NormalizeId(Candidate.InstanceId);
        Candidate.Quantity = FMath::Clamp(Candidate.Quantity, 1, Definition->StackLimit);
        if (CanPlace(Candidate, Candidate.GridPosition, Candidate.bRotated))
        {
            Entries.Add(Candidate);
        }
        else
        {
            Candidate.GridPosition = FIntPoint::ZeroValue;
            Candidate.bRotated = false;
            RecoveryBasket.Add(Candidate);
        }
    }

    for (const FInventoryEntry& Saved : SavedRecovery)
    {
        const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(Saved.ItemId);
        if (!Definition || Saved.Quantity <= 0) continue;
        int32 Remaining = FMath::Clamp(Saved.Quantity, 1, 1000000);
        while (Remaining > 0)
        {
            FInventoryEntry Candidate = Saved;
            Candidate.InstanceId = NormalizeId(Remaining == Saved.Quantity ? Candidate.InstanceId : FGuid());
            Candidate.GridPosition = FIntPoint::ZeroValue;
            Candidate.bRotated = false;
            Candidate.Quantity = FMath::Min(Remaining, Definition->StackLimit);
            RecoveryBasket.Add(Candidate);
            Remaining -= Candidate.Quantity;
        }
    }
    OnInventoryChanged.Broadcast();
}
