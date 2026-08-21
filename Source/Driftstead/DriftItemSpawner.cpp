#include "DriftItemSpawner.h"
#include "DriftItemActor.h"
#include "DriftsteadTypes.h"
#include "TimerManager.h"

ADriftItemSpawner::ADriftItemSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
    ItemClass = ADriftItemActor::StaticClass();
}

void ADriftItemSpawner::BeginPlay()
{
    Super::BeginPlay();
    SpawnBatch(10);
    GetWorldTimerManager().SetTimer(SpawnTimer, this, &ADriftItemSpawner::SpawnOne, SpawnInterval, true);
    GetWorldTimerManager().SetTimer(CleanupTimer, this, &ADriftItemSpawner::CleanupItems, 2.0f, true);
}

int32 ADriftItemSpawner::GetActiveCount() const
{
    int32 Count = 0;
    for (const ADriftItemActor* Item : ActiveItems) if (IsValid(Item)) ++Count;
    return Count;
}

void ADriftItemSpawner::SpawnBatch(int32 Count)
{
    for (int32 Index = 0; Index < Count && GetActiveCount() < MaxActiveItems; ++Index) SpawnOne();
}

void ADriftItemSpawner::SpawnOne()
{
    CleanupItems();
    if (GetActiveCount() >= MaxActiveItems) return;

    const FVector Location(FMath::FRandRange(SpawnXRange.X, SpawnXRange.Y), SpawnY + FMath::FRandRange(-120.0f, 180.0f), 45.0f);
    ADriftItemActor* Item = GetWorld()->SpawnActor<ADriftItemActor>(ItemClass, Location, FRotator::ZeroRotator);
    if (!Item) return;
    const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(ChooseWeightedItem());
    if (!Definition)
    {
        Item->Destroy();
        return;
    }
    const float Speed = FMath::FRandRange(Definition->DriftSpeedRange.X, Definition->DriftSpeedRange.Y);
    Item->ConfigureItem(Definition->ItemId, FVector(0.0f, -Speed, 0.0f));
    ActiveItems.Add(Item);
}

void ADriftItemSpawner::CleanupItems()
{
    for (int32 Index = ActiveItems.Num() - 1; Index >= 0; --Index)
    {
        ADriftItemActor* Item = ActiveItems[Index];
        if (!IsValid(Item))
        {
            ActiveItems.RemoveAtSwap(Index);
        }
        else if (Item->GetActorLocation().Y < CleanupY)
        {
            Item->Destroy();
            ActiveItems.RemoveAtSwap(Index);
        }
    }
}

FName ADriftItemSpawner::ChooseWeightedItem() const
{
    const TArray<FDriftItemDefinition>& Definitions = FDriftsteadItemCatalog::GetDefinitions();
    float TotalWeight = 0.0f;
    for (const FDriftItemDefinition& Definition : Definitions) TotalWeight += Definition.SpawnWeight;
    float Roll = FMath::FRandRange(0.0f, TotalWeight);
    for (const FDriftItemDefinition& Definition : Definitions)
    {
        Roll -= Definition.SpawnWeight;
        if (Roll <= 0.0f) return Definition.ItemId;
    }
    return Definitions.Last().ItemId;
}
