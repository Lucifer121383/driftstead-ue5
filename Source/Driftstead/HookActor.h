#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HookActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UHookComponent;

UCLASS(Blueprintable)
class DRIFTSTEAD_API AHookActor : public AActor
{
    GENERATED_BODY()

public:
    AHookActor();
    virtual void Tick(float DeltaSeconds) override;
    void InitializeHook(UHookComponent* InOwnerComponent);

    UFUNCTION() void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HookMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RopeMesh;
    TWeakObjectPtr<UHookComponent> OwnerComponent;
};
