#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

class ADriftsteadCharacter;

UINTERFACE(BlueprintType)
class DRIFTSTEAD_API UInteractableInterface : public UInterface
{
    GENERATED_BODY()
};

class DRIFTSTEAD_API IInteractableInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction") bool Interact(ADriftsteadCharacter* Character);
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction") FText GetInteractionPrompt() const;
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction") int32 GetInteractionFloor() const;
};
