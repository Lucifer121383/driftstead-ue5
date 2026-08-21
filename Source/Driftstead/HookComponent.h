#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DriftsteadTypes.h"
#include "HookComponent.generated.h"

class AHookActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHookStateChangedSignature, EHookState, PreviousState, EHookState, NewState);

UCLASS(ClassGroup=(Driftstead), BlueprintType, meta=(BlueprintSpawnableComponent))
class DRIFTSTEAD_API UHookComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHookComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="Hook") void StartCharging();
    UFUNCTION(BlueprintCallable, Category="Hook") void ReleaseHook();
    UFUNCTION(BlueprintCallable, Category="Hook") void RecallHook();
    UFUNCTION(BlueprintCallable, Category="Hook") void SetAimDirection(FVector NewDirection);
    UFUNCTION(BlueprintPure, Category="Hook") EHookState GetHookState() const { return State; }
    UFUNCTION(BlueprintPure, Category="Hook") float GetChargeAlpha() const;
    UFUNCTION(BlueprintPure, Category="Hook") FVector GetEstimatedLandingPoint() const;
    UFUNCTION(BlueprintPure, Category="Hook") FVector GetRopeOrigin() const;
    UFUNCTION(BlueprintPure, Category="Hook") static bool IsTransitionAllowed(EHookState From, EHookState To);

    void NotifyHookOverlap(AActor* OtherActor);

    UPROPERTY(BlueprintAssignable, Category="Hook") FHookStateChangedSignature OnHookStateChanged;

protected:
    UPROPERTY(EditDefaultsOnly, Category="Hook") float MinimumChargeSeconds = 0.15f;
    UPROPERTY(EditDefaultsOnly, Category="Hook") float FullChargeSeconds = 1.2f;
    UPROPERTY(EditDefaultsOnly, Category="Hook") float MinimumRange = 350.0f;
    UPROPERTY(EditDefaultsOnly, Category="Hook") float MaximumRange = 1250.0f;
    UPROPERTY(EditDefaultsOnly, Category="Hook") float FlightSpeed = 1200.0f;
    UPROPERTY(EditDefaultsOnly, Category="Hook") float ReturnSpeed = 1550.0f;
    UPROPERTY(EditDefaultsOnly, Category="Hook") float HookCapacity = 6.0f;
    UPROPERTY(EditDefaultsOnly, Category="Hook") TSubclassOf<AHookActor> HookActorClass;

private:
    void SetState(EHookState NewState);
    void BeginReturn(bool bHitSomething);
    void FinishReturn();
    void EnterIdle();
    void NotifyPlayer(const FText& Message, FLinearColor Color) const;

    UPROPERTY(VisibleInstanceOnly, Category="Hook") EHookState State = EHookState::Idle;
    UPROPERTY() TObjectPtr<AHookActor> ActiveHook;
    UPROPERTY() TObjectPtr<AActor> AttachedActor;
    FVector AimDirection = FVector::ForwardVector;
    FVector FlightDirection = FVector::ForwardVector;
    FVector LaunchOrigin = FVector::ZeroVector;
    float ChargeSeconds = 0.0f;
    float TargetRange = 0.0f;
};
