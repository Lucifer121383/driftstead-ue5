#include "DriftsteadHUD.h"
#include "DriftsteadCharacter.h"
#include "DriftsteadGameMode.h"
#include "DriftsteadQuestSubsystem.h"
#include "HookComponent.h"
#include "InventoryComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

namespace
{
constexpr float InventoryOriginX = 710.0f;
constexpr float InventoryOriginY = 270.0f;
constexpr float InventoryCellSize = 72.0f;
}

void ADriftsteadHUD::DrawPanel(float X, float Y, float W, float H, FLinearColor Color)
{
    DrawRect(Color, X, Y, W, H);
}

void ADriftsteadHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;
    // UE's default runtime composite font automatically selects its bundled CJK fallback.
    if (!InterfaceFont && GEngine) InterfaceFont = GEngine->GetSmallFont();
    const float SX = Canvas->SizeX / 1920.0f;
    const float SY = Canvas->SizeY / 1080.0f;
    DrawStatus(SX, SY);
    DrawHelp(SX, SY);
    if (bInventoryOpen) DrawInventory(SX, SY);

    if (FPlatformTime::Seconds() < FeedbackExpiry)
    {
        DrawPanel(590 * SX, 890 * SY, 740 * SX, 54 * SY, FLinearColor(0.02f, 0.04f, 0.06f, 0.82f));
        DrawText(FeedbackMessage.ToString(), FeedbackColor, 620 * SX, 902 * SY, InterfaceFont, 1.1f * FMath::Min(SX, SY));
    }

    if (bMainMenuOpen)
    {
        DrawPanel(540 * SX, 280 * SY, 840 * SX, 450 * SY, FLinearColor(0.015f, 0.035f, 0.055f, 0.94f));
        DrawText(TEXT("漂海牧场：DRIFTSTEAD"), FLinearColor(1.0f, 0.72f, 0.20f), 675 * SX, 340 * SY, InterfaceFont, 2.0f * FMath::Min(SX, SY));
        DrawText(TEXT("2.5D 海上生存建造演示"), FLinearColor(0.72f, 0.92f, 0.95f), 760 * SX, 410 * SY, InterfaceFont, 1.15f * FMath::Min(SX, SY));
        DrawText(TEXT("N 新游戏　|　C 继续游戏　|　H 展示模式　|　Q 退出"), FLinearColor::White, 665 * SX, 510 * SY, InterfaceFont, 1.02f * FMath::Min(SX, SY));
        DrawText(TEXT("Enter 快速开始　　F1 测试资源　　F2 木筏等级"), FLinearColor(0.75f, 0.78f, 0.82f), 700 * SX, 575 * SY, InterfaceFont, 0.95f * FMath::Min(SX, SY));
    }
    if (bPausePanelOpen)
    {
        DrawPanel(730 * SX, 360 * SY, 460 * SX, 260 * SY, FLinearColor(0.02f, 0.03f, 0.05f, 0.94f));
        DrawText(TEXT("暂停 / 设置"), FLinearColor(1.0f, 0.72f, 0.20f), 845 * SX, 410 * SY, InterfaceFont, 1.4f * FMath::Min(SX, SY));
        DrawText(TEXT("按 Esc 关闭此面板"), FLinearColor::White, 840 * SX, 500 * SY, InterfaceFont, 1.0f * FMath::Min(SX, SY));
    }
}

void ADriftsteadHUD::DrawStatus(float SX, float SY)
{
    const ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(GetOwningPawn());
    if (!Character) return;
    const UInventoryComponent* Inventory = Character->GetInventory();
    const ADriftsteadGameMode* GM = GetWorld()->GetAuthGameMode<ADriftsteadGameMode>();

    DrawPanel(28 * SX, 24 * SY, 520 * SX, 150 * SY, FLinearColor(0.02f, 0.05f, 0.07f, 0.82f));
    DrawText(FString::Printf(TEXT("木筏等级 %d　|　第 %d 层"), GM ? GM->GetRaftLevel() : 1, Character->GetCurrentFloor() + 1), FLinearColor(1.0f, 0.72f, 0.20f), 50 * SX, 45 * SY, InterfaceFont, 1.15f * FMath::Min(SX, SY));
    DrawText(FString::Printf(TEXT("木材 %d　金属 %d　绳索 %d　食物 %d　淡水 %d"), Inventory->GetResource(TEXT("Wood")), Inventory->GetResource(TEXT("Metal")), Inventory->GetResource(TEXT("Rope")), Inventory->GetResource(TEXT("Food")), Inventory->GetResource(TEXT("Water"))), FLinearColor::White, 50 * SX, 90 * SY, InterfaceFont, 0.95f * FMath::Min(SX, SY));
    FString Objective = TEXT("打捞物资、整理背包、扩建海上牧场");
    if (const UDriftsteadQuestSubsystem* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Objective = Quest->GetCurrentInstruction().ToString();
    DrawText(FString::Printf(TEXT("任务：%s"), *Objective), FLinearColor(0.60f, 0.88f, 0.92f), 50 * SX, 130 * SY, InterfaceFont, 0.85f * FMath::Min(SX, SY));

    const float Charge = Character->GetHook()->GetChargeAlpha();
    DrawPanel(770 * SX, 995 * SY, 380 * SX, 22 * SY, FLinearColor(0.04f, 0.06f, 0.08f, 0.9f));
    DrawPanel(774 * SX, 999 * SY, 372 * SX * Charge, 14 * SY, FLinearColor(1.0f, 0.62f, 0.12f, 1.0f));
}

void ADriftsteadHUD::DrawInventory(float SX, float SY)
{
    const ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(GetOwningPawn());
    if (!Character) return;
    const UInventoryComponent* Inventory = Character->GetInventory();
    const float Cell = InventoryCellSize * FMath::Min(SX, SY);
    const float X0 = InventoryOriginX * SX;
    const float Y0 = InventoryOriginY * SY;
    DrawPanel(X0 - 42, Y0 - 80, Inventory->GetColumns() * Cell + 84, Inventory->GetRows() * Cell + 150, FLinearColor(0.015f, 0.03f, 0.05f, 0.94f));
    DrawText(TEXT("空间背包　[左键拖拽] [R 旋转] [Shift+R 拆分] [B 取回]"), FLinearColor(1.0f, 0.72f, 0.20f), X0 - 20, Y0 - 58, InterfaceFont, 0.92f * FMath::Min(SX, SY));
    for (int32 Y = 0; Y < Inventory->GetRows(); ++Y)
        for (int32 X = 0; X < Inventory->GetColumns(); ++X)
            DrawPanel(X0 + X * Cell, Y0 + Y * Cell, Cell - 3, Cell - 3, FLinearColor(0.08f, 0.12f, 0.15f, 0.95f));

    for (const FInventoryEntry& Entry : Inventory->GetEntries())
    {
        const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(Entry.ItemId);
        if (!Definition) continue;
        const FIntPoint Size = Entry.bRotated ? FIntPoint(Definition->Footprint.Y, Definition->Footprint.X) : Definition->Footprint;
        DrawPanel(X0 + Entry.GridPosition.X * Cell + 4, Y0 + Entry.GridPosition.Y * Cell + 4, Size.X * Cell - 11, Size.Y * Cell - 11, Definition->PrimaryColor.CopyWithNewOpacity(0.92f));
        DrawText(FString::Printf(TEXT("%s ×%d"), *Definition->DisplayName.ToString(), Entry.Quantity), FLinearColor::White, X0 + Entry.GridPosition.X * Cell + 9, Y0 + Entry.GridPosition.Y * Cell + 12, InterfaceFont, 0.68f * FMath::Min(SX, SY));
    }
    DrawText(FString::Printf(TEXT("临时回收篮：%d 组物资"), Inventory->GetRecoveryBasket().Num()), FLinearColor(0.95f, 0.76f, 0.30f), X0, Y0 + Inventory->GetRows() * Cell + 24, InterfaceFont, 0.85f * FMath::Min(SX, SY));
}

bool ADriftsteadHUD::BeginInventoryDrag()
{
    if (!bInventoryOpen || !PlayerOwner) return false;
    ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(GetOwningPawn());
    if (!Character) return false;
    float MouseX = 0.0f, MouseY = 0.0f;
    if (!PlayerOwner->GetMousePosition(MouseX, MouseY) || !Canvas) return false;
    const float SX = Canvas->SizeX / 1920.0f;
    const float SY = Canvas->SizeY / 1080.0f;
    const float Cell = InventoryCellSize * FMath::Min(SX, SY);
    const FIntPoint CellPosition(FMath::FloorToInt((MouseX - InventoryOriginX * SX) / Cell), FMath::FloorToInt((MouseY - InventoryOriginY * SY) / Cell));
    for (const FInventoryEntry& Entry : Character->GetInventory()->GetEntries())
    {
        const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(Entry.ItemId);
        if (!Definition) continue;
        const FIntPoint Size = Entry.bRotated ? FIntPoint(Definition->Footprint.Y, Definition->Footprint.X) : Definition->Footprint;
        if (CellPosition.X >= Entry.GridPosition.X && CellPosition.Y >= Entry.GridPosition.Y && CellPosition.X < Entry.GridPosition.X + Size.X && CellPosition.Y < Entry.GridPosition.Y + Size.Y)
        {
            DraggedInstanceId = Entry.InstanceId;
            DragCellOffset = CellPosition - Entry.GridPosition;
            return true;
        }
    }
    return false;
}

bool ADriftsteadHUD::EndInventoryDrag()
{
    if (!DraggedInstanceId.IsValid() || !PlayerOwner || !Canvas) return false;
    ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(GetOwningPawn());
    float MouseX = 0.0f, MouseY = 0.0f;
    if (!Character || !PlayerOwner->GetMousePosition(MouseX, MouseY))
    {
        DraggedInstanceId.Invalidate();
        return false;
    }
    const float SX = Canvas->SizeX / 1920.0f;
    const float SY = Canvas->SizeY / 1080.0f;
    const float Cell = InventoryCellSize * FMath::Min(SX, SY);
    const FIntPoint DropCell(FMath::FloorToInt((MouseX - InventoryOriginX * SX) / Cell), FMath::FloorToInt((MouseY - InventoryOriginY * SY) / Cell));
    const bool bMoved = Character->GetInventory()->MoveItem(DraggedInstanceId, DropCell - DragCellOffset);
    Character->ShowFeedback(bMoved ? NSLOCTEXT("Driftstead", "DragMoved", "物品已移动。") : NSLOCTEXT("Driftstead", "DragBlocked", "目标位置被占用。"), bMoved ? FLinearColor::Green : FLinearColor::Red);
    DraggedInstanceId.Invalidate();
    return bMoved;
}

void ADriftsteadHUD::DrawHelp(float SX, float SY)
{
    if (!bDeveloperPanelOpen) return;
    DrawPanel(1480 * SX, 24 * SY, 410 * SX, 300 * SY, FLinearColor(0.02f, 0.05f, 0.07f, 0.78f));
    const TArray<FString> Lines = {
        TEXT("WASD 移动　鼠标瞄准"), TEXT("按住/松开左键：蓄力抛钩"), TEXT("右键 / Space：提前收钩"),
        TEXT("E 交互　Tab 打开背包"), TEXT("R 旋转　Shift+R 拆分　Esc 暂停"), TEXT("F1 资源　F2 木筏等级（Shift 反向）"),
        TEXT("F3 生成物资　F4 楼层（Shift 反向）"), TEXT("F5 保存　F6 帮助　F9 重置")
    };
    float Y = 48 * SY;
    for (const FString& Line : Lines)
    {
        DrawText(Line, FLinearColor::White, 1510 * SX, Y, InterfaceFont, 0.82f * FMath::Min(SX, SY));
        Y += 32 * SY;
    }
}

void ADriftsteadHUD::ShowFeedback(const FText& Message, FLinearColor Color)
{
    FeedbackMessage = Message;
    FeedbackColor = Color;
    FeedbackExpiry = FPlatformTime::Seconds() + 3.0;
}
