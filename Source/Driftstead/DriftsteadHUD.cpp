#include "DriftsteadHUD.h"
#include "DriftsteadCharacter.h"
#include "DriftsteadPlayerController.h"
#include "DriftsteadGameMode.h"
#include "DriftsteadQuestSubsystem.h"
#include "HookComponent.h"
#include "InventoryComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "InteractableInterface.h"
#include "FacilityActor.h"
#include "Kismet/GameplayStatics.h"

namespace
{
struct FInventoryLayout
{
    float Cell, X, Y=210, Width;
    explicit FInventoryLayout(const UInventoryComponent* Inventory)
    {
        Cell=FMath::Min(60.0f,380.0f/Inventory->GetRows());
        Width=Inventory->GetColumns()*Cell+280;
        X=(1280-Width)*.5f+24;
    }
};
}

void ADriftsteadHUD::DrawPanel(float X, float Y, float W, float H, FLinearColor Color)
{
    DrawRect(Color, X, Y, W, H);
}

void ADriftsteadHUD::BeginPlay()
{
    Super::BeginPlay();
    if (GEngine) InterfaceFont = GEngine->GetMediumFont();
    // Upload small UI textures before the inventory's first visible frame.
    for (const auto& Definition : FDriftsteadItemCatalog::GetDefinitions())
    {
        const FString Id = Definition.ItemId.ToString();
        const FString Path = FString::Printf(TEXT("/Game/Driftstead/UI/Icons/T_Icon_%s.T_Icon_%s"),*Id,*Id);
        ItemIcons.Add(Definition.ItemId,LoadObject<UTexture2D>(nullptr,*Path));
    }
}

void ADriftsteadHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;
    if (!InterfaceFont && GEngine) InterfaceFont = GEngine->GetMediumFont();
    const float SX = Canvas->SizeX / 1920.0f;
    const float SY = Canvas->SizeY / 1080.0f;
    if (bMainMenuOpen)
    {
        Card(42,74,470,566,FLinearColor(.015f,.043f,.053f,.96f));
        Card(42,74,6,566,FLinearColor(.95f,.69f,.28f));
        DrawLabel(TEXT("DRIFTSTEAD  /  第一段航程"),76,112,16,FLinearColor(.4f,.78f,.76f));
        DrawLabel(TEXT("漂海牧场"),72,150,48,FLinearColor(1,.83f,.5f));
        DrawLabel(TEXT("把漂来的残骸，变成能生活的家。"),76,222,20);
        DrawLabel(TEXT("风暴过后，你只剩一张小木筏。\n打捞补给，建立菜园，登高点亮信标。\n远方是否还有人，也在等待回应？"),76,270,18,FLinearColor(.74f,.84f,.82f));
        const auto* Player = Cast<ADriftsteadCharacter>(GetOwningPawn());
        const bool Suspended = Player && Player->HasSuspendedSession();
        const TArray<FString> Buttons = {TEXT("开始新的航程    [N]"),Suspended?TEXT("继续当前航程    [C / Enter]"):TEXT("继续上次航程    [C / Enter]"),TEXT("参观十级牧场    [H]")};
        for (int32 Index=0; Index<3; ++Index)
        {
            Card(76,390+Index*58,396,46,Index==0?FLinearColor(.18f,.48f,.45f):FLinearColor(.07f,.15f,.18f));
            DrawLabel(Buttons[Index],96,401+Index*58,20);
        }
        DrawLabel(Suspended?TEXT("航程已保存  ·  继续可回到刚才的位置\n鼠标点击按钮或按快捷键开始     Q 退出"):TEXT("建议体验 20–30 分钟  ·  可随时 F5 保存\n鼠标点击按钮或按快捷键开始     Q 退出"),76,572,15,FLinearColor(.57f,.71f,.7f));
        return;
    }
    DrawStatus(SX,SY);
    DrawHelp(SX,SY);
    if (bInventoryOpen) DrawInventory(SX,SY);
    if (bPausePanelOpen)
    {
        Card(0,0,1280,720,FLinearColor(0,0,0,.48f));
        Card(400,164,480,390,FLinearColor(.025f,.065f,.085f,.98f));
        DrawLabel(TEXT("暂泊片刻"),436,194,32,FLinearColor(1,.8f,.46f));
        DrawLabel(TEXT("航程已暂停，准备好后继续出发。"),436,247,17,FLinearColor(.7f,.83f,.83f));
        const TArray<FString> Buttons = {TEXT("继续航程    [Esc]"),TEXT("保存进度    [F5]"),TEXT("保存并返回主菜单    [M]")};
        for (int32 Index=0; Index<3; ++Index)
        {
            Card(436,291+Index*58,408,46,Index==0?FLinearColor(.18f,.48f,.45f):FLinearColor(.07f,.15f,.18f));
            DrawLabel(Buttons[Index],456,302+Index*58,20);
        }
        DrawLabel(TEXT("返回前会自动收钩，物资入包后再保存。\n远处小岛与沉船为环境布景，本版不可登陆。"),436,481,14,FLinearColor(.61f,.75f,.74f));
    }
    if (FPlatformTime::Seconds() < FeedbackExpiry)
    {
        Card(220,590,840,42,FLinearColor(.025f,.07f,.09f,.96f));
        DrawLabel(FeedbackMessage.ToString(),238,600,18,FeedbackColor);
    }
}

void ADriftsteadHUD::SetInventoryOpen(bool bOpen)
{
    bInventoryOpen = bOpen;
    if (!bOpen) { DraggedInstanceId.Invalidate(); DragCellOffset = FIntPoint::ZeroValue; }
}

void ADriftsteadHUD::ResetTransientUI()
{
    SetInventoryOpen(false);
    SelectedInstanceId.Invalidate();
    bPausePanelOpen = bDeveloperPanelOpen = false;
    FeedbackExpiry = 0;
    CompletionShownAt = -1;
}

void ADriftsteadHUD::OpenMainMenu()
{
    ResetTransientUI();
    bMainMenuOpen = true;
}

void ADriftsteadHUD::DrawItemIcon(FName ItemId, float X, float Y, float W, float H)
{
    if (!ItemIcons.Contains(ItemId))
    {
        const FString Path = FString::Printf(TEXT("/Game/Driftstead/UI/Icons/T_Icon_%s.T_Icon_%s"),*ItemId.ToString(),*ItemId.ToString());
        ItemIcons.Add(ItemId, LoadObject<UTexture2D>(nullptr,*Path));
    }
    if (UTexture2D* Texture = ItemIcons.FindRef(ItemId))
    {
        const float Size = FMath::Min(W,H);
        const float SX=Canvas->SizeX/1280.0f, SY=Canvas->SizeY/720.0f;
        DrawTexture(Texture,(X+(W-Size)*.5f)*SX,(Y+(H-Size)*.5f)*SY,Size*SX,Size*SY,0,0,1,1,FLinearColor::White,BLEND_Opaque);
    }
}

void ADriftsteadHUD::Card(float X,float Y,float W,float H,FLinearColor Color)
{
    DrawPanel(X*Canvas->SizeX/1280.0f,Y*Canvas->SizeY/720.0f,W*Canvas->SizeX/1280.0f,H*Canvas->SizeY/720.0f,Color);
}

void ADriftsteadHUD::DrawLabel(const FString& Text,float X,float Y,float Size,FLinearColor Color)
{
    if (!Canvas || !InterfaceFont) return;
    const float SX = Canvas->SizeX/1280.0f, SY = Canvas->SizeY/720.0f;
    const float DPI = FMath::Max(Canvas->GetDPIScale(), .01f);
    const float Pixels = FMath::Max(1.0f,FMath::RoundToFloat(Size*FMath::Min(SX,SY)));
    FSlateFontInfo FontInfo = InterfaceFont->GetLegacySlateFontInfo();
    // Slate sizes are points at 96 DPI. Request glyphs at their final pixel size;
    // scaling the legacy 10-point atlas instead makes Chinese strokes blurry.
    FontInfo.Size = Pixels * 72.0f / 96.0f / DPI;
    TArray<FString> Lines; Text.ParseIntoArrayLines(Lines);
    for (int32 Index=0; Index<Lines.Num(); ++Index)
    {
        const FVector2D Position(FMath::RoundToFloat(X*SX)/DPI,FMath::RoundToFloat((Y+Index*(Size+8))*SY)/DPI);
        FCanvasTextItem Item(Position,FText::FromString(Lines[Index]),FontInfo,Color);
        Item.Scale = FVector2D(1,1);
        Canvas->DrawItem(Item);
    }
}

bool ADriftsteadHUD::GetPointerInDesignSpace(float& X, float& Y, bool bUsePressPosition) const
{
    // AHUD::Canvas is cleared after PostRender; input callbacks run outside it.
    // GetMousePosition and GetViewportSize share the viewport's pixel space.
    if (!PlayerOwner) return false;
    bool HasPosition = false;
    if (bUsePressPosition)
        if (const auto* Controller = Cast<ADriftsteadPlayerController>(PlayerOwner)) HasPosition = Controller->GetLastPointerPress(X,Y);
    if (!HasPosition && !PlayerOwner->GetMousePosition(X, Y)) return false;
    int32 Width=0, Height=0;
    PlayerOwner->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0) return false;
    X *= 1280.0f / Width; Y *= 720.0f / Height;
    return true;
}

void ADriftsteadHUD::HandleMenuClick()
{
    if (!bMainMenuOpen) return;
    float X=0,Y=0; if (!GetPointerInDesignSpace(X,Y,true)) return;
    if (X < 76 || X > 472) return;
    for (int32 Index=0; Index<3; ++Index)
        if (Y >= 390+Index*58 && Y <= 436+Index*58)
            if (auto* Character=Cast<ADriftsteadCharacter>(GetOwningPawn())) { Character->SelectMenuOption(Index); return; }
}

void ADriftsteadHUD::HandlePauseClick()
{
    if (!bPausePanelOpen || bMainMenuOpen) return;
    float X=0,Y=0; if (!GetPointerInDesignSpace(X,Y,true)) return;
    if (X < 436 || X > 844) return;
    for (int32 Index=0; Index<3; ++Index)
        if (Y >= 291+Index*58 && Y <= 337+Index*58)
            if (auto* Character=Cast<ADriftsteadCharacter>(GetOwningPawn())) { Character->SelectPauseOption(Index); return; }
}

void ADriftsteadHUD::DrawStatus(float SX, float SY)
{
    const ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(GetOwningPawn());
    if (!Character) return;
    const UInventoryComponent* Inventory = Character->GetInventory();
    const ADriftsteadGameMode* GM = GetWorld()->GetAuthGameMode<ADriftsteadGameMode>();

    const FLinearColor Navy(.018f,.058f,.072f,.92f), Gold(1,.8f,.43f), Muted(.6f,.79f,.78f);
    Card(18,18,1244,64,Navy);
    DrawLabel(FString::Printf(TEXT("漂海牧场   %d 级 / %d 层"), GM?GM->GetRaftLevel():1,Character->GetCurrentFloor()+1),34,38,21,Gold);
    DrawLabel(FString::Printf(TEXT("木材 %d    绳索 %d    金属 %d    布料 %d    零件 %d"),Inventory->GetResource(TEXT("Wood")),Inventory->GetResource(TEXT("Rope")),Inventory->GetResource(TEXT("Metal")),Inventory->GetResource(TEXT("Cloth")),Inventory->GetResource(TEXT("Parts"))),360,29,18);
    DrawLabel(FString::Printf(TEXT("食物 %d   淡水 %d   种子 %d       Tab 背包  ·  F5 保存  ·  Esc 暂停"),Inventory->GetResource(TEXT("Food")),Inventory->GetResource(TEXT("Water")),Inventory->GetResource(TEXT("Seeds"))),360,54,15,Muted);
    const auto* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>();
    Card(18,96,440,130,Navy);
    if (Quest)
    {
        DrawLabel(Quest->GetCurrentInstruction().ToString(),34,112,18,Gold);
        DrawLabel(FString::Printf(TEXT("航程进度  %d / 12"),Quest->GetCompletedObjectives()),34,180,14,Muted);
        Card(34,207,404,4,FLinearColor(.12f,.22f,.25f));
        Card(34,207,404*Quest->GetCompletedObjectives()/12.0f,4,FLinearColor(.38f,.78f,.64f));
    }
    Card(968,96,294,196,Navy);
    DrawLabel(GM && GM->GetRaftLevel()==4 && Quest && !Quest->IsComplete()?TEXT("本次航程终点"):TEXT("下一次扩建"),984,112,20,Gold);
    if (GM) DrawLabel(GM->GetUpgradeSummary(Character->GetInventory()),984,147,16);
    DrawLabel(GM && GM->GetRaftLevel()==4 && Quest && !Quest->IsComplete()?TEXT("二层信标按 E 修复"):TEXT("靠近工作台按 U\nE 开桶 / 使用设施"),984,257,14,Muted);
    Card(300,654,680,50,Navy);
    const auto State = Character->GetHook()->GetHookState();
    FString HookText = TEXT("WASD 移动  ·  鼠标瞄准  ·  按住左键蓄力抛钩");
    if (State == EHookState::Flying) HookText = TEXT("钩子飞向目标距离  ·  右键可提前收钩");
    if (State == EHookState::Attached || State == EHookState::Returning) HookText = FString::Printf(TEXT("回程打捞中  ·  已携带 %d 件  ·  到脚下后入包"),Character->GetHook()->GetAttachedCount());
    if (State == EHookState::Charging) HookText = FString::Printf(TEXT("蓄力 %d%%  ·  松开左键抛出  ·  回程可以连带多件物资"),FMath::RoundToInt(Character->GetHook()->GetChargeAlpha()*100));
    DrawLabel(HookText,322,665,17);
    if (State == EHookState::Charging)
    {
        Card(310,696,660*Character->GetHook()->GetChargeAlpha(),4,Gold);
        const FVector Point = Project(Character->GetHook()->GetEstimatedLandingPoint());
        if (Point.Z > 0) { DrawLine(Point.X-10,Point.Y,Point.X+10,Point.Y,Gold,2); DrawLine(Point.X,Point.Y-10,Point.X,Point.Y+10,Gold,2); }
    }
    if (!bInventoryOpen && Character->GetInteractionTarget())
    {
        const AActor* Target = Character->GetInteractionTarget();
        const FVector P = Project(Target->GetActorLocation() + FVector(0,0,155));
        const FString Prompt = IInteractableInterface::Execute_GetInteractionPrompt(const_cast<AActor*>(Target)).ToString();
        if (P.Z>0) DrawLabel(Prompt,P.X*1280.0f/Canvas->SizeX-85,P.Y*720.0f/Canvas->SizeY-14,17,Gold);
    }
    if (Quest && Quest->IsComplete() && CompletionShownAt < 0) CompletionShownAt=GetWorld()->GetTimeSeconds();
    if (Quest && !Quest->IsComplete()) CompletionShownAt=-1;
    if (Quest && Quest->IsComplete() && !bInventoryOpen && GetWorld()->GetTimeSeconds()-CompletionShownAt<12)
    {
        Card(300,480,680,96,Navy);
        DrawLabel(TEXT("远方传来了回应。你的海上家园，终于被看见。"),324,498,22,Gold);
        DrawLabel(TEXT("首段航程完成  ·  F5 保存  ·  可继续扩建与自由打捞"),324,540,16,Muted);
    }
}

void ADriftsteadHUD::DrawInventory(float SX, float SY)
{
    const ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(GetOwningPawn());
    if (!Character) return;
    const UInventoryComponent* Inventory = Character->GetInventory();
    const FInventoryLayout Layout(Inventory);
    const float Cell=Layout.Cell, X0=Layout.X, Y0=Layout.Y;
    const float Height=Inventory->GetRows()*Cell;
    const float SideX=X0+Inventory->GetColumns()*Cell+24;
    Card(0,0,1280,720,FLinearColor(0,.025f,.035f,.6f));
    Card(X0-24,124,Layout.Width,Height+156,FLinearColor(.016f,.047f,.064f,.98f));
    DrawLabel(TEXT("航行背包"),X0,144,26,FLinearColor(1,.8f,.43f));
    DrawLabel(TEXT("左键拖放 / 交换    R 旋转    Shift+R 拆分    Tab / Esc 关闭"),X0,184,14);
    for (int32 Y = 0; Y < Inventory->GetRows(); ++Y)
        for (int32 X = 0; X < Inventory->GetColumns(); ++X)
            Card(X0 + X * Cell, Y0 + Y * Cell, Cell - 3, Cell - 3, FLinearColor(.06f,.12f,.15f));

    static const TMap<FName,FString> ShortNames={{TEXT("Driftwood"),TEXT("木材")},{TEXT("Rope"),TEXT("绳索")},{TEXT("ScrapMetal"),TEXT("废铁")},{TEXT("Cloth"),TEXT("布料")},{TEXT("SeedCrate"),TEXT("种子")},{TEXT("FoodCrate"),TEXT("食物")},{TEXT("SealedBarrel"),TEXT("木桶")},{TEXT("Electronics"),TEXT("电路")},{TEXT("MachineryCrate"),TEXT("机械")},{TEXT("AnimalCrate"),TEXT("动物")}};
    const FDriftItemDefinition* Selected=nullptr;
    for (const FInventoryEntry& Entry : Inventory->GetEntries())
    {
        const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(Entry.ItemId);
        if (!Definition) continue;
        const FIntPoint Size = Entry.bRotated ? FIntPoint(Definition->Footprint.Y, Definition->Footprint.X) : Definition->Footprint;
        if (Entry.InstanceId == SelectedInstanceId)
        {
            Selected=Definition;
            Card(X0 + Entry.GridPosition.X * Cell, Y0 + Entry.GridPosition.Y * Cell, Size.X * Cell - 3, Size.Y * Cell - 3, FLinearColor(1,.78f,.3f));
        }
        const float X=X0+Entry.GridPosition.X*Cell, Y=Y0+Entry.GridPosition.Y*Cell;
        Card(X+3,Y+3,Size.X*Cell-9,Size.Y*Cell-9,FLinearColor(.018f,.037f,.052f));
        Card(X+4,Y+4,3,Size.Y*Cell-11,Definition->SecondaryColor);
        DrawItemIcon(Entry.ItemId,X+8,Y+5,Size.X*Cell-18,Size.Y*Cell-13);
        Card(X+8,Y+Size.Y*Cell-20,Size.X*Cell-17,15,FLinearColor(.018f,.037f,.052f,.94f));
        DrawLabel(ShortNames.FindRef(Entry.ItemId),X+9,Y+Size.Y*Cell-19,13);
        if (Entry.Quantity > 1)
        {
            Card(X+Size.X*Cell-31,Y+5,24,17,FLinearColor(.015f,.035f,.045f,.92f));
            DrawLabel(FString::Printf(TEXT("%d"),Entry.Quantity),X+Size.X*Cell-27,Y+6,12,FLinearColor(1,.85f,.58f));
        }
    }
    DrawLabel(Selected?Selected->DisplayName.ToString():TEXT("整理海上收获"),SideX,Y0+4,20,FLinearColor(1,.8f,.43f));
    if (Selected)
    {
        const auto* Entry = Inventory->GetEntries().FindByPredicate([this](const FInventoryEntry& E){return E.InstanceId==SelectedInstanceId;});
        if (Entry) DrawItemIcon(Entry->ItemId,SideX,Y0+36,186,96);
        DrawLabel(FString::Printf(TEXT("占格 %d×%d  ·  重量 %d\n堆叠上限 %d"),Selected->Footprint.X,Selected->Footprint.Y,Selected->Weight,Selected->StackLimit),SideX,Y0+142,14);
        DrawLabel(TEXT("建造材料已计入顶部资源。\n密封木桶：靠近工作台按 E。"),SideX,Y0+196,13,FLinearColor(.7f,.83f,.83f));
    }
    else DrawLabel(TEXT("点击物品查看实物图标。\n\n建造材料已计入顶部资源。\n密封木桶：在工作台按 E。\n\n拖放整理不改变资源数量。"),SideX,Y0+42,14,FLinearColor(.7f,.83f,.83f));
    if (Inventory->GetEntries().IsEmpty()) DrawLabel(TEXT("背包还是空的\n去水面打捞第一份补给吧"),X0+20,Y0+60,18,FLinearColor(.58f,.74f,.75f));
    DrawLabel(FString::Printf(TEXT("临时回收篮 %d 组   ·   B 尝试放回背包   ·   满包不会丢失物资"),Inventory->GetRecoveryBasket().Num()),X0,Y0+Height+20,14,FLinearColor(.95f,.78f,.46f));
    if (DraggedInstanceId.IsValid() && PlayerOwner)
    {
        float MX=0,MY=0;
        if (GetPointerInDesignSpace(MX,MY))
        {
            const FIntPoint Drop(FMath::FloorToInt((MX-X0)/Cell)-DragCellOffset.X,FMath::FloorToInt((MY-Y0)/Cell)-DragCellOffset.Y);
            const auto* Dragged=Inventory->GetEntries().FindByPredicate([this](const FInventoryEntry& E){return E.InstanceId==DraggedInstanceId;});
            if (Dragged)
            {
                const bool Valid=Inventory->CanPlace(*Dragged,Drop,Dragged->bRotated,DraggedInstanceId);
                Card(MX+10,MY+12,110,26,Valid?FLinearColor(.12f,.4f,.29f):FLinearColor(.38f,.17f,.12f));
                DrawLabel(Valid?TEXT("松开放置"):TEXT("交换 / 不可放置"),MX+16,MY+16,12);
            }
        }
    }
}

bool ADriftsteadHUD::BeginInventoryDrag()
{
    if (!bInventoryOpen || !PlayerOwner) return false;
    ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(GetOwningPawn());
    if (!Character) return false;
    float MouseX = 0.0f, MouseY = 0.0f;
    if (!GetPointerInDesignSpace(MouseX, MouseY,true)) return false;
    const FInventoryLayout Layout(Character->GetInventory());
    const FIntPoint CellPosition(FMath::FloorToInt((MouseX-Layout.X)/Layout.Cell),FMath::FloorToInt((MouseY-Layout.Y)/Layout.Cell));
    for (const FInventoryEntry& Entry : Character->GetInventory()->GetEntries())
    {
        const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(Entry.ItemId);
        if (!Definition) continue;
        const FIntPoint Size = Entry.bRotated ? FIntPoint(Definition->Footprint.Y, Definition->Footprint.X) : Definition->Footprint;
        if (CellPosition.X >= Entry.GridPosition.X && CellPosition.Y >= Entry.GridPosition.Y && CellPosition.X < Entry.GridPosition.X + Size.X && CellPosition.Y < Entry.GridPosition.Y + Size.Y)
        {
            DraggedInstanceId = Entry.InstanceId;
            SelectedInstanceId = Entry.InstanceId;
            DragCellOffset = CellPosition - Entry.GridPosition;
            return true;
        }
    }
    SelectedInstanceId.Invalidate();
    return false;
}

bool ADriftsteadHUD::EndInventoryDrag()
{
    if (!DraggedInstanceId.IsValid()) return false;
    ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(GetOwningPawn());
    float MouseX = 0.0f, MouseY = 0.0f;
    if (!Character || !GetPointerInDesignSpace(MouseX, MouseY))
    {
        DraggedInstanceId.Invalidate();
        return false;
    }
    const FInventoryLayout Layout(Character->GetInventory());
    const FIntPoint DropCell(FMath::FloorToInt((MouseX-Layout.X)/Layout.Cell),FMath::FloorToInt((MouseY-Layout.Y)/Layout.Cell));
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
        TEXT("E 交互　Tab 打开背包"), TEXT("R 旋转　Shift+R 拆分　Esc 暂停"), TEXT("F1 全资源 +100　F2 木筏等级"),
        TEXT("F3 生成物资　F4 楼层（Shift 反向）"), TEXT("F5 保存　F6 帮助　F9 重置")
    };
    float Y = 48 * SY;
    for (const FString& Line : Lines)
    {
        DrawLabel(Line,1510.0f*1280/1920,Y*720.0f/Canvas->SizeY,14);
        Y += 32 * SY;
    }
}

void ADriftsteadHUD::ShowFeedback(const FText& Message, FLinearColor Color)
{
    FeedbackMessage = Message;
    FeedbackColor = Color;
    FeedbackExpiry = FPlatformTime::Seconds() + 3.0;
}
