#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DriftsteadTypes.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInventoryFeedbackSignature, EInventoryAddResult, Result, FName, ItemId);

UCLASS(ClassGroup=(Driftstead), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class DRIFTSTEAD_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    UFUNCTION(BlueprintCallable, Category="Inventory") void InitializeGrid(int32 Columns, int32 Rows);
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetColumns() const { return GridColumns; }
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetRows() const { return GridRows; }
    UFUNCTION(BlueprintPure, Category="Inventory") const TArray<FInventoryEntry>& GetEntries() const { return Entries; }
    UFUNCTION(BlueprintPure, Category="Inventory") const TArray<FInventoryEntry>& GetRecoveryBasket() const { return RecoveryBasket; }

    bool CanPlace(const FInventoryEntry& Entry, FIntPoint Position, bool bRotated, const FGuid& IgnoreInstance = FGuid()) const;
    bool FindFirstFit(FName ItemId, FIntPoint& OutPosition, bool& bOutRotated) const;

    UFUNCTION(BlueprintCallable, Category="Inventory") EInventoryAddResult TryAddItem(FName ItemId, int32 Quantity = 1);
    UFUNCTION(BlueprintCallable, Category="Inventory") bool MoveItem(FGuid InstanceId, FIntPoint NewPosition);
    UFUNCTION(BlueprintCallable, Category="Inventory") bool RotateItem(FGuid InstanceId);
    UFUNCTION(BlueprintCallable, Category="Inventory") bool SplitStack(FGuid InstanceId, int32 Quantity = 0);
    UFUNCTION(BlueprintCallable, Category="Inventory") bool RecoverFromBasket(FGuid InstanceId);
    UFUNCTION(BlueprintCallable, Category="Inventory") bool RemoveQuantity(FName ItemId, int32 Quantity);
    UFUNCTION(BlueprintPure, Category="Inventory") int32 CountItem(FName ItemId) const;

    UFUNCTION(BlueprintCallable, Category="Resources") void AddResource(FName ResourceId, int32 Amount);
    UFUNCTION(BlueprintPure, Category="Resources") int32 GetResource(FName ResourceId) const;
    UFUNCTION(BlueprintPure, Category="Resources") bool HasResources(const TMap<FName, int32>& Cost) const;
    UFUNCTION(BlueprintCallable, Category="Resources") bool ConsumeResourcesTransactional(const TMap<FName, int32>& Cost);
    UFUNCTION(BlueprintCallable, Category="Resources") void AddTestResources(int32 Amount = 50);

    void RestoreState(int32 Columns, int32 Rows, const TArray<FInventoryEntry>& SavedEntries, const TArray<FInventoryEntry>& SavedRecovery, const TMap<FName, int32>& SavedResources);
    const TMap<FName, int32>& GetResources() const { return Resources; }

    UPROPERTY(BlueprintAssignable, Category="Inventory") FInventoryChangedSignature OnInventoryChanged;
    UPROPERTY(BlueprintAssignable, Category="Inventory") FInventoryFeedbackSignature OnInventoryFeedback;

private:
    FIntPoint GetEffectiveFootprint(const FInventoryEntry& Entry, bool bRotated) const;
    FInventoryEntry* FindEntry(FGuid InstanceId);

    UPROPERTY(EditDefaultsOnly, Category="Inventory") int32 GridColumns = 6;
    UPROPERTY(EditDefaultsOnly, Category="Inventory") int32 GridRows = 4;
    UPROPERTY(VisibleInstanceOnly, Category="Inventory") TArray<FInventoryEntry> Entries;
    UPROPERTY(VisibleInstanceOnly, Category="Inventory") TArray<FInventoryEntry> RecoveryBasket;
    UPROPERTY(VisibleInstanceOnly, Category="Resources") TMap<FName, int32> Resources;
};
