#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CatchableInterface.h"
#include "DriftItemActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;

UCLASS(Blueprintable)
class DRIFTSTEAD_API ADriftItemActor : public AActor, public ICatchableInterface
{
    GENERATED_BODY()

public:
    ADriftItemActor();
    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="Drift Item") void ConfigureItem(FName NewItemId, FVector NewDriftVelocity);
    UFUNCTION(BlueprintPure, Category="Drift Item") FName GetItemId() const { return ItemId; }
    UFUNCTION(BlueprintCallable, Category="Drift Item") void PrepareForRecovery();

    virtual bool CanBeCaught_Implementation(float HookCapacity) const override;
    virtual float GetCatchWeight_Implementation() const override;
    virtual void OnCaught_Implementation(AActor* HookActor) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> VisualMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPointLightComponent> RareLight;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drift Item") FName ItemId = TEXT("Driftwood");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drift Item") FVector DriftVelocity = FVector(0.0, -50.0, 0.0);

private:
    void ApplyDefinitionVisuals();
    float BaseHeight = 0.0f;
    float BobPhase = 0.0f;
    bool bCaught = false;
};
