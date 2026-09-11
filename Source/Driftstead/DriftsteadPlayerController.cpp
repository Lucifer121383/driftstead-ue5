#include "DriftsteadPlayerController.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool ADriftsteadPlayerController::InputKey(const FInputKeyParams& Params)
{
    const bool bManualQA = FParse::Param(FCommandLine::Get(), TEXT("DriftsteadManualQA"));
    if (Params.Key == EKeys::LeftMouseButton && Params.Event == IE_Pressed)
    {
        // FSceneViewport refreshes its event-local cursor cache before InputKey.
        // Enhanced Input may dispatch Started only after later move/up events,
        // so preserve this down position rather than querying the live endpoint.
        LastPointerPressX = LastPointerPressY = 0.0f;
        bLastPointerPressValid = GetMousePosition(LastPointerPressX, LastPointerPressY);
        if (bManualQA)
            UE_LOG(LogTemp, Display, TEXT("[ManualQA] PointerPress valid=%d x=%.1f y=%.1f"), bLastPointerPressValid ? 1 : 0, LastPointerPressX, LastPointerPressY);
    }
    if (bManualQA && Params.Event != IE_Axis)
        UE_LOG(LogTemp, Display, TEXT("[ManualQA] Input %s event=%d"), *Params.Key.ToString(), static_cast<int32>(Params.Event));
    if (!Params.Key.IsAnalog())
    {
        if (Params.Event == IE_Pressed)
        {
            DeferredTapReleases.RemoveAll([&Params](const FInputKeyParams& Release) { return Release.Key == Params.Key; });
            PressedSinceLastInput.Add(Params.Key);
        }
        else if (Params.Event == IE_Released && PressedSinceLastInput.Contains(Params.Key))
        {
            // UE 5.3 Enhanced Input can miss digital down/up events arriving in
            // one frame. Let it sample the press once before applying release.
            DeferredTapReleases.Add(Params);
            return true;
        }
    }
    return Super::InputKey(Params);
}

bool ADriftsteadPlayerController::GetLastPointerPress(float& X, float& Y) const
{
    if (!bLastPointerPressValid) return false;
    X = LastPointerPressX;
    Y = LastPointerPressY;
    return true;
}

void ADriftsteadPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
    Super::PostProcessInput(DeltaTime, bGamePaused);
    PressedSinceLastInput.Reset();
    const TArray<FInputKeyParams> Releases = MoveTemp(DeferredTapReleases);
    DeferredTapReleases.Reset();
    for (const FInputKeyParams& Release : Releases) Super::InputKey(Release);
}

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
