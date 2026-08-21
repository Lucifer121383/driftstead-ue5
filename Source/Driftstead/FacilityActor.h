#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "DriftsteadTypes.h"
#include "FacilityActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UInventoryComponent;

UCLASS(Blueprintable)
class DRIFTSTEAD_API AFacilityActor : public AActor, public IInteractableInterface
{
    GENERATED_BODY()

public:
    AFacilityActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="Facility") void Configure(EFacilityType NewType, int32 NewFloorIndex);
    UFUNCTION(BlueprintPure, Category="Facility") EFacilityType GetFacilityType() const { return FacilityType; }
    FFacilitySaveState CaptureSaveState() const;
    void RestoreSaveState(const FFacilitySaveState& State);

    virtual bool Interact_Implementation(ADriftsteadCharacter* Character) override;
    virtual FText GetInteractionPrompt_Implementation() const override;
    virtual int32 GetInteractionFloor_Implementation() const override { return FloorIndex; }

protected:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionBounds;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BodyMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> AccentMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInventoryComponent> StorageInventory;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Facility") EFacilityType FacilityType = EFacilityType::Workbench;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Facility") int32 FloorIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Facility") int32 Capacity = 5;
    UPROPERTY(VisibleInstanceOnly, Category="Facility") int32 StoredOutput = 0;
    UPROPERTY(VisibleInstanceOnly, Category="Facility") bool bProducing = false;

private:
    void ApplyVisuals();
    void ConfigureProductionTimer();
    void PerformProduction();
    void FinishFarmProduction();
    void RunAutoCrane();
    FText FacilityName() const;
    FTimerHandle ProductionTimer;
    FTimerHandle FarmTimer;
    float VisualTime = 0.0f;
};
