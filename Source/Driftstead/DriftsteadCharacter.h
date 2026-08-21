#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DriftsteadCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UInventoryComponent;
class UHookComponent;
class UInputAction;
class UInputMappingContext;

UCLASS(Blueprintable)
class DRIFTSTEAD_API ADriftsteadCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ADriftsteadCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintPure, Category="Driftstead") UInventoryComponent* GetInventory() const { return Inventory; }
    UFUNCTION(BlueprintPure, Category="Driftstead") UHookComponent* GetHook() const { return Hook; }
    UFUNCTION(BlueprintPure, Category="Driftstead") int32 GetCurrentFloor() const { return CurrentFloor; }
    UFUNCTION(BlueprintCallable, Category="Driftstead") void SetCurrentFloor(int32 NewFloor, bool bTeleport = true);
    void ShowFeedback(const FText& Message, FLinearColor Color = FLinearColor::White) const;

private:
    UInputAction* CreateBooleanAction(FKey Key);
    bool IsShiftDown() const;
    bool IsGameplayInputBlocked(bool bIncludeInventory = false) const;
    void UpdateAim();
    void MoveForwardOn(); void MoveForwardOff();
    void MoveBackwardOn(); void MoveBackwardOff();
    void MoveLeftOn(); void MoveLeftOff();
    void MoveRightOn(); void MoveRightOff();
    void StartHook(); void ReleaseHook(); void RecallHook();
    void Interact(); void ToggleInventory(); void RotateSelection(); void TogglePause();
    void RecoverFirstBasketItem();
    void AddDebugResources(); void ChangeRaftLevel(); void SpawnDebugItems(); void ChangeFloor();
    void QuickSave(); void ToggleDeveloperPanel(); void ConfirmResetSave(); void StartGameFromMenu(); void QuitFromMenu();
    void StartNewNormalGame(); void ContinueNormalGame(); void StartShowcaseGame();
    void ResetRuntimeForMode(bool bShowcase);

    UPROPERTY(VisibleAnywhere, Category="Camera") TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere, Category="Camera") TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere, Category="Gameplay") TObjectPtr<UInventoryComponent> Inventory;
    UPROPERTY(VisibleAnywhere, Category="Gameplay") TObjectPtr<UHookComponent> Hook;
    UPROPERTY(VisibleAnywhere, Category="Visual") TObjectPtr<UStaticMeshComponent> BodyMesh;
    UPROPERTY(VisibleAnywhere, Category="Visual") TObjectPtr<UStaticMeshComponent> HeadMesh;
    UPROPERTY(VisibleAnywhere, Category="Visual") TObjectPtr<UStaticMeshComponent> HatMesh;
    UPROPERTY(VisibleAnywhere, Category="Visual") TObjectPtr<UStaticMeshComponent> BackpackMesh;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> RuntimeMappingContext;
    UPROPERTY(Transient) TArray<TObjectPtr<UInputAction>> RuntimeActions;

    bool bForward = false;
    bool bBackward = false;
    bool bLeft = false;
    bool bRight = false;
    bool bResetArmed = false;
    bool bMoveQuestNotified = false;
    double ResetArmTime = 0.0;
    int32 CurrentFloor = 0;
};
