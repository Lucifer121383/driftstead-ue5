#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DriftsteadTypes.h"
#include "DriftsteadQuestSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQuestStepChangedSignature, EDriftsteadQuestStep, NewStep);

UCLASS()
class DRIFTSTEAD_API UDriftsteadQuestSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Quest") void NotifyEvent(EDriftsteadQuestStep Event);
    UFUNCTION(BlueprintCallable, Category="Quest") void ResetQuest();
    UFUNCTION(BlueprintCallable, Category="Quest") void RestoreQuest(EDriftsteadQuestStep SavedStep, bool bSavedComplete);
    UFUNCTION(BlueprintPure, Category="Quest") EDriftsteadQuestStep GetCurrentStep() const { return CurrentStep; }
    UFUNCTION(BlueprintPure, Category="Quest") bool IsComplete() const { return bComplete; }
    UFUNCTION(BlueprintPure, Category="Quest") FText GetCurrentInstruction() const;

    UPROPERTY(BlueprintAssignable) FQuestStepChangedSignature OnQuestStepChanged;

private:
    UPROPERTY() EDriftsteadQuestStep CurrentStep = EDriftsteadQuestStep::Move;
    UPROPERTY() bool bComplete = false;
};
