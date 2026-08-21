#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "StairActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class DRIFTSTEAD_API AStairActor : public AActor, public IInteractableInterface
{
    GENERATED_BODY()

public:
    AStairActor();
    void Configure(int32 InFromFloor, int32 InToFloor);
    virtual bool Interact_Implementation(ADriftsteadCharacter* Character) override;
    virtual FText GetInteractionPrompt_Implementation() const override;
    virtual int32 GetInteractionFloor_Implementation() const override { return FromFloor; }

    UFUNCTION(BlueprintPure) int32 GetFromFloor() const { return FromFloor; }
    UFUNCTION(BlueprintPure) int32 GetToFloor() const { return ToFloor; }

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionBounds;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> StairMesh;
    UPROPERTY(EditAnywhere, Category="Stair") int32 FromFloor = 0;
    UPROPERTY(EditAnywhere, Category="Stair") int32 ToFloor = 1;
};
