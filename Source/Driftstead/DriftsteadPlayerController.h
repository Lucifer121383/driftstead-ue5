#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DriftsteadPlayerController.generated.h"

UCLASS()
class DRIFTSTEAD_API ADriftsteadPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ADriftsteadPlayerController();
    virtual void BeginPlay() override;
};
