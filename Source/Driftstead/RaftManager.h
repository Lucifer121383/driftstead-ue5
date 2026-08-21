#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DriftsteadTypes.h"
#include "RaftManager.generated.h"

class UInstancedStaticMeshComponent;
class USceneComponent;
class UInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRaftLevelChangedSignature, int32, NewLevel);

UCLASS(Blueprintable)
class DRIFTSTEAD_API ARaftManager : public AActor
{
    GENERATED_BODY()

public:
    ARaftManager();
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="Raft") bool TryUpgrade(UInventoryComponent* Inventory);
    UFUNCTION(BlueprintCallable, Category="Raft") void ForceLevel(int32 NewLevel);
    UFUNCTION(BlueprintCallable, Category="Raft") void SetViewedFloor(int32 FloorIndex);
    UFUNCTION(BlueprintPure, Category="Raft") int32 GetRaftLevel() const { return RaftLevel; }
    UFUNCTION(BlueprintPure, Category="Raft") int32 GetMaximumFloor() const;
    TArray<FFacilitySaveState> CaptureFacilityStates() const;
    void RestoreFacilityStates(const TArray<FFacilitySaveState>& States);

    UPROPERTY(BlueprintAssignable, Category="Raft") FRaftLevelChangedSignature OnRaftLevelChanged;

private:
    void GenerateRaft();
    void GenerateFloor(UInstancedStaticMeshComponent* Floor, UInstancedStaticMeshComponent* Rails, FIntPoint Dimensions, int32 FloorIndex);
    void SpawnFacilities(const FRaftLevelDefinition& Definition);
    void DestroyGeneratedActors();

    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> FloorOne;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> FloorTwo;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> FloorThree;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> RailsOne;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> RailsTwo;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> RailsThree;
    UPROPERTY() TArray<TObjectPtr<AActor>> GeneratedActors;
    UPROPERTY(EditAnywhere, Category="Raft") int32 RaftLevel = 1;
    UPROPERTY(EditDefaultsOnly, Category="Raft") float TileSize = 130.0f;
    UPROPERTY(EditDefaultsOnly, Category="Raft") float FloorHeight = 340.0f;
    int32 ViewedFloor = 0;
};
