#include "DriftsteadQuestSubsystem.h"

void UDriftsteadQuestSubsystem::NotifyEvent(EDriftsteadQuestStep Event)
{
    if (Event == EDriftsteadQuestStep::Complete) return;
    EventCounts.FindOrAdd(Event) = FMath::Min(9999, GetEventCount(Event) + 1);
    RefreshProgress();
}

const TArray<EDriftsteadQuestStep>& UDriftsteadQuestSubsystem::GetSequence()
{
    static const TArray<EDriftsteadQuestStep> Steps = {
        EDriftsteadQuestStep::Move, EDriftsteadQuestStep::ChargeHook, EDriftsteadQuestStep::SalvageItem,
        EDriftsteadQuestStep::OpenInventory, EDriftsteadQuestStep::OpenBarrel, EDriftsteadQuestStep::UpgradeLevel2,
        EDriftsteadQuestStep::CollectWater, EDriftsteadQuestStep::UpgradeLevel3, EDriftsteadQuestStep::HarvestCrop,
        EDriftsteadQuestStep::UpgradeLevel4, EDriftsteadQuestStep::ReachSecondFloor, EDriftsteadQuestStep::RepairSignal};
    return Steps;
}

void UDriftsteadQuestSubsystem::RefreshProgress()
{
    EDriftsteadQuestStep Next = EDriftsteadQuestStep::Complete;
    for (const auto Step : GetSequence())
        if (GetEventCount(Step) < (Step == EDriftsteadQuestStep::SalvageItem ? 6 : 1)) { Next = Step; break; }
    const bool bChanged = CurrentStep != Next;
    CurrentStep = Next;
    bComplete = Next == EDriftsteadQuestStep::Complete;
    if (bChanged) OnQuestStepChanged.Broadcast(CurrentStep);
}

int32 UDriftsteadQuestSubsystem::GetCompletedObjectives() const
{
    int32 Count = 0;
    for (const auto Step : GetSequence())
    {
        if (GetEventCount(Step) < (Step == EDriftsteadQuestStep::SalvageItem ? 6 : 1)) break;
        ++Count;
    }
    return Count;
}

void UDriftsteadQuestSubsystem::RestoreEvents(const TMap<EDriftsteadQuestStep, int32>& SavedEvents)
{
    EventCounts.Reset();
    for (const auto& Pair : SavedEvents)
        if (GetSequence().Contains(Pair.Key)) EventCounts.Add(Pair.Key, FMath::Clamp(Pair.Value, 0, 9999));
    RefreshProgress();
}

void UDriftsteadQuestSubsystem::ResetQuest()
{
    CurrentStep = EDriftsteadQuestStep::Move;
    EventCounts.Reset();
    bComplete = false;
    OnQuestStepChanged.Broadcast(CurrentStep);
}

void UDriftsteadQuestSubsystem::RestoreQuest(EDriftsteadQuestStep SavedStep, bool bSavedComplete)
{
    EventCounts.Reset();
    for (const auto Step : GetSequence())
        if (static_cast<uint8>(Step) < static_cast<uint8>(SavedStep) && static_cast<uint8>(Step) < 10)
            EventCounts.Add(Step, Step == EDriftsteadQuestStep::SalvageItem ? 6 : 1);
    RefreshProgress();
}

FText UDriftsteadQuestSubsystem::GetCurrentInstruction() const
{
    static const TMap<EDriftsteadQuestStep, FText> Instructions = {
        {EDriftsteadQuestStep::Move, NSLOCTEXT("Driftstead", "QuestMove", "第一章 · 漂流者\n用 WASD 走到甲板边缘，观察水上的物资。")},
        {EDriftsteadQuestStep::ChargeHook, NSLOCTEXT("Driftstead", "QuestHook", "第一章 · 学会抛钩\n鼠标瞄准物资，按住左键蓄力，松开抛出。")},
        {EDriftsteadQuestStep::SalvageItem, NSLOCTEXT("Driftstead", "QuestSalvage", "打捞 1 件漂流物资。")},
        {EDriftsteadQuestStep::OpenInventory, NSLOCTEXT("Driftstead", "QuestInventory", "第一章 · 整理收获\n按 Tab 查看背包；建造材料已计入资源。")},
        {EDriftsteadQuestStep::RotateItem, NSLOCTEXT("Driftstead", "QuestRotate", "按 R 旋转 1 件背包物品。")},
        {EDriftsteadQuestStep::OpenBarrel, NSLOCTEXT("Driftstead", "QuestBarrel", "第二章 · 海上补给\n打捞圆形密封桶，靠近工作台按 E 打开。")},
        {EDriftsteadQuestStep::UpgradeLevel2, NSLOCTEXT("Driftstead", "QuestLevel2", "第二章 · 安稳落脚\n收集木材与绳索，靠近工作台按 U 扩至 2 级。")},
        {EDriftsteadQuestStep::CollectWater, NSLOCTEXT("Driftstead", "QuestWater", "第三章 · 第一杯淡水\n靠近甲板上的雨水桶，按 E 收取淡水。")},
        {EDriftsteadQuestStep::UpgradeLevel3, NSLOCTEXT("Driftstead", "QuestLevel3", "第三章 · 海上菜园\n打捞废铁，工作台 U 升至 3 级，开放种植田。")},
        {EDriftsteadQuestStep::HarvestCrop, NSLOCTEXT("Driftstead", "QuestHarvest", "第三章 · 等待发芽\n种植田 E 播种；消耗种子与淡水，成熟后 E 收获。")},
        {EDriftsteadQuestStep::UpgradeLevel4, NSLOCTEXT("Driftstead", "QuestLevel4", "第四章 · 更高的地方\n备齐木材、绳索、废铁和布料，工作台 U 升至 4 级。")},
        {EDriftsteadQuestStep::ReachSecondFloor, NSLOCTEXT("Driftstead", "QuestFloor2", "第四章 · 登高望远\n靠近木梯按 E，前往二层寻找求救信标。")},
        {EDriftsteadQuestStep::RepairSignal, NSLOCTEXT("Driftstead", "QuestSignal", "终章 · 远方的回应\n备齐补给与零件，二层信标按 E 发出求救信号。")},
        {EDriftsteadQuestStep::Complete, NSLOCTEXT("Driftstead", "QuestComplete", "航程完成 · 灯光得到了回应\n你已拥有海上家园。可继续自由建造。")}
    };
    if (CurrentStep == EDriftsteadQuestStep::SalvageItem)
        return FText::FromString(FString::Printf(TEXT("第一章 · 一网多得  %d / 6\n回程碰到几件带几件，到脚下才入包。"), FMath::Min(6, GetEventCount(CurrentStep))));
    return Instructions.FindRef(CurrentStep);
}
