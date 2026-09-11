#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DriftsteadHUD.generated.h"

UCLASS()
class DRIFTSTEAD_API ADriftsteadHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void DrawHUD() override;
    void ShowFeedback(const FText& Message, FLinearColor Color);
    void ToggleInventory() { SetInventoryOpen(!bInventoryOpen); }
    void SetInventoryOpen(bool bOpen);
    bool IsInventoryOpen() const { return bInventoryOpen; }
    bool IsDraggingInventoryItem() const { return DraggedInstanceId.IsValid(); }
    FGuid GetSelectedInventoryItemId() const { return SelectedInstanceId; }
    bool BeginInventoryDrag();
    bool EndInventoryDrag();
    void ToggleDeveloperPanel() { bDeveloperPanelOpen = !bDeveloperPanelOpen; }
    void TogglePausePanel() { bPausePanelOpen = !bPausePanelOpen; }
    bool IsPausePanelOpen() const { return bPausePanelOpen; }
    void CloseMainMenu() { bMainMenuOpen = false; }
    void OpenMainMenu();
    void ResetTransientUI();
    bool IsMainMenuOpen() const { return bMainMenuOpen; }
    void HandleMenuClick();
    void HandlePauseClick();

private:
    bool GetPointerInDesignSpace(float& X, float& Y, bool bUsePressPosition = false) const;
    void DrawPanel(float X, float Y, float W, float H, FLinearColor Color);
    void DrawStatus(float ScaleX, float ScaleY);
    void DrawInventory(float ScaleX, float ScaleY);
    void DrawHelp(float ScaleX, float ScaleY);
    void DrawLabel(const FString& Text, float X, float Y, float Size, FLinearColor Color = FLinearColor::White);
    void Card(float X, float Y, float W, float H, FLinearColor Color);
    void DrawItemIcon(FName ItemId, float X, float Y, float W, float H);

    bool bInventoryOpen = false;
    bool bDeveloperPanelOpen = false;
    bool bPausePanelOpen = false;
    bool bMainMenuOpen = true;
    FGuid DraggedInstanceId;
    FGuid SelectedInstanceId;
    FIntPoint DragCellOffset = FIntPoint::ZeroValue;
    FText FeedbackMessage;
    FLinearColor FeedbackColor = FLinearColor::White;
    double FeedbackExpiry = 0.0;
    double CompletionShownAt = -1;

    UPROPERTY(Transient)
    TObjectPtr<class UFont> InterfaceFont;
    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<class UTexture2D>> ItemIcons;
};
