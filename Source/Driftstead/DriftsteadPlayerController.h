#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "DriftsteadPlayerController.generated.h"

UCLASS()
class DRIFTSTEAD_API ADriftsteadPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ADriftsteadPlayerController();
    virtual void BeginPlay() override;
    virtual bool InputKey(const FInputKeyParams& Params) override;
    virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
    bool GetLastPointerPress(float& X, float& Y) const;

private:
    float LastPointerPressX = 0.0f;
    float LastPointerPressY = 0.0f;
    bool bLastPointerPressValid = false;
    TSet<FKey> PressedSinceLastInput;
    TArray<FInputKeyParams> DeferredTapReleases;
};
