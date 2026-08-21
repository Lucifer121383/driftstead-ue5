#include "DriftsteadPlayerController.h"

ADriftsteadPlayerController::ADriftsteadPlayerController()
{
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ADriftsteadPlayerController::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
}
