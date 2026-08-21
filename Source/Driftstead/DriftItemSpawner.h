#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DriftItemSpawner.generated.h"

class ADriftItemActor;

UCLASS(Blueprintable)
class DRIFTSTEAD_API ADriftItemSpawner : public AActor
{
    GENERATED_BODY()

public:
    ADriftItemSpawner();
    virtual void BeginPlay() override;
    UFUNCTION(BlueprintCallable, Category="Spawning") void SpawnBatch(int32 Count = 8);
    UFUNCTION(BlueprintPure, Category="Spawning") int32 GetActiveCount() const;

protected:
    UPROPERTY(EditDefaultsOnly, Category="Spawning") TSubclassOf<ADriftItemActor> ItemClass;
    UPROPERTY(EditDefaultsOnly, Category="Spawning") int32 MaxActiveItems = 24;
    UPROPERTY(EditDefaultsOnly, Category="Spawning") float SpawnInterval = 1.35f;
    UPROPERTY(EditDefaultsOnly, Category="Spawning") FVector2D SpawnXRange = FVector2D(-900.0f, 900.0f);
    UPROPERTY(EditDefaultsOnly, Category="Spawning") float SpawnY = 1500.0f;
    UPROPERTY(EditDefaultsOnly, Category="Spawning") float CleanupY = -1700.0f;

private:
    void SpawnOne();
    void CleanupItems();
    FName ChooseWeightedItem() const;
    UPROPERTY() TArray<TObjectPtr<ADriftItemActor>> ActiveItems;
    FTimerHandle SpawnTimer;
    FTimerHandle CleanupTimer;
};
