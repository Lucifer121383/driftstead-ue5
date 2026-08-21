#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CatchableInterface.generated.h"

UINTERFACE(BlueprintType)
class DRIFTSTEAD_API UCatchableInterface : public UInterface
{
    GENERATED_BODY()
};

class DRIFTSTEAD_API ICatchableInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Salvage") bool CanBeCaught(float HookCapacity) const;
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Salvage") float GetCatchWeight() const;
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Salvage") void OnCaught(AActor* HookActor);
};
