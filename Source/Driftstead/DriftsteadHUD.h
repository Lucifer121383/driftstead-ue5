#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DriftsteadHUD.generated.h"

UCLASS()
class DRIFTSTEAD_API ADriftsteadHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
    void ShowFeedback(const FText& Message, FLinearColor Color);
    void ToggleInventory() { bInventoryOpen = !bInventoryOpen; }
    void SetInventoryOpen(bool bOpen) { bInventoryOpen = bOpen; }
    bool IsInventoryOpen() const { return bInventoryOpen; }
    bool IsDraggingInventoryItem() const { return DraggedInstanceId.IsValid(); }
    bool BeginInventoryDrag();
    bool EndInventoryDrag();
    void ToggleDeveloperPanel() { bDeveloperPanelOpen = !bDeveloperPanelOpen; }
    void TogglePausePanel() { bPausePanelOpen = !bPausePanelOpen; }
    void CloseMainMenu() { bMainMenuOpen = false; }
    bool IsMainMenuOpen() const { return bMainMenuOpen; }

private:
    void DrawPanel(float X, float Y, float W, float H, FLinearColor Color);
    void DrawStatus(float ScaleX, float ScaleY);
    void DrawInventory(float ScaleX, float ScaleY);
    void DrawHelp(float ScaleX, float ScaleY);

    bool bInventoryOpen = false;
    bool bDeveloperPanelOpen = true;
    bool bPausePanelOpen = false;
    bool bMainMenuOpen = true;
    FGuid DraggedInstanceId;
    FIntPoint DragCellOffset = FIntPoint::ZeroValue;
    FText FeedbackMessage;
    FLinearColor FeedbackColor = FLinearColor::White;
    double FeedbackExpiry = 0.0;

    UPROPERTY(Transient)
    TObjectPtr<class UFont> InterfaceFont;
};
