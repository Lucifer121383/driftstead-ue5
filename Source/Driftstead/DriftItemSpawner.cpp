#include "DriftItemSpawner.h"
#include "DriftItemActor.h"
#include "DriftsteadTypes.h"
#include "TimerManager.h"
#include "DriftsteadGameMode.h"

ADriftItemSpawner::ADriftItemSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
    ItemClass = ADriftItemActor::StaticClass();
}

void ADriftItemSpawner::BeginPlay()
{
    Super::BeginPlay();
    ResetOpening();
    GetWorldTimerManager().SetTimer(SpawnTimer, this, &ADriftItemSpawner::SpawnOne, SpawnInterval, true);
    GetWorldTimerManager().SetTimer(CleanupTimer, this, &ADriftItemSpawner::CleanupItems, 2.0f, true);
}

void ADriftItemSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearAllTimersForObject(this);
    for (ADriftItemActor* Item : ActiveItems) if (IsValid(Item) && !Item->IsCaught()) Item->Destroy();
    Super::EndPlay(EndPlayReason);
}

void ADriftItemSpawner::ResetOpening()
{
    for (ADriftItemActor* Item : ActiveItems) if (IsValid(Item) && !Item->IsCaught()) Item->Destroy();
    ActiveItems.Reset(); SpawnSequence = 0;
    SpawnBatch(12);
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

    const ADriftsteadGameMode* GM = GetWorld()->GetAuthGameMode<ADriftsteadGameMode>();
    const int32 Level = GM ? GM->GetRaftLevel() : 1;
    const float Edge = Level < 3 ? 450.0f : 690.0f;
    const float Angle = FMath::DegreesToRadians(FMath::FRandRange(15.0f, 165.0f));
    const float Radius = FMath::FRandRange(Edge + 100.0f, Edge + 510.0f);
    const FVector Location(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius + 200.0f, 35.0f);
    ADriftItemActor* Item = GetWorld()->SpawnActor<ADriftItemActor>(ItemClass, Location, FRotator::ZeroRotator);
    if (!Item) return;
    // A repeating supply manifest guarantees essentials and barrels without
    // making the tutorial depend on lucky random rolls.
    static const TArray<FName> Manifest = {TEXT("Driftwood"),TEXT("Rope"),TEXT("Driftwood"),TEXT("ScrapMetal"),TEXT("Rope"),TEXT("Cloth"),TEXT("SealedBarrel"),TEXT("SeedCrate"),TEXT("Driftwood"),TEXT("Rope"),TEXT("FoodCrate"),TEXT("Electronics")};
    const FName Id = Manifest[SpawnSequence++ % Manifest.Num()];
    const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(Id);
    if (!Definition)
    {
        Item->Destroy();
        return;
    }
    const float Speed = FMath::FRandRange(18.0f, 28.0f);
    Item->ConfigureItem(Definition->ItemId, FVector(8.0f, -Speed, 0.0f));
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
        else if (!Item->IsCaught() && Item->GetActorLocation().Y < CleanupY)
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
