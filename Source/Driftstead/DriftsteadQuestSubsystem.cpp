#include "DriftsteadQuestSubsystem.h"

void UDriftsteadQuestSubsystem::NotifyEvent(EDriftsteadQuestStep Event)
{
    if (bComplete || Event != CurrentStep) return;
    if (CurrentStep == EDriftsteadQuestStep::Complete)
    {
        bComplete = true;
        return;
    }
    CurrentStep = static_cast<EDriftsteadQuestStep>(static_cast<uint8>(CurrentStep) + 1);
    if (CurrentStep == EDriftsteadQuestStep::Complete) bComplete = true;
    OnQuestStepChanged.Broadcast(CurrentStep);
}

void UDriftsteadQuestSubsystem::ResetQuest()
{
    CurrentStep = EDriftsteadQuestStep::Move;
    bComplete = false;
    OnQuestStepChanged.Broadcast(CurrentStep);
}

void UDriftsteadQuestSubsystem::RestoreQuest(EDriftsteadQuestStep SavedStep, bool bSavedComplete)
{
    CurrentStep = SavedStep;
    bComplete = bSavedComplete || SavedStep == EDriftsteadQuestStep::Complete;
    OnQuestStepChanged.Broadcast(CurrentStep);
}

FText UDriftsteadQuestSubsystem::GetCurrentInstruction() const
{
    static const TMap<EDriftsteadQuestStep, FText> Instructions = {
        {EDriftsteadQuestStep::Move, NSLOCTEXT("Driftstead", "QuestMove", "使用 WASD 在木筏上移动。")},
        {EDriftsteadQuestStep::ChargeHook, NSLOCTEXT("Driftstead", "QuestHook", "按住并松开鼠标左键，蓄力抛出钩子。")},
        {EDriftsteadQuestStep::SalvageItem, NSLOCTEXT("Driftstead", "QuestSalvage", "打捞 1 件漂流物资。")},
        {EDriftsteadQuestStep::OpenInventory, NSLOCTEXT("Driftstead", "QuestInventory", "按 Tab 打开空间背包。")},
        {EDriftsteadQuestStep::RotateItem, NSLOCTEXT("Driftstead", "QuestRotate", "按 R 旋转 1 件背包物品。")},
        {EDriftsteadQuestStep::OpenBarrel, NSLOCTEXT("Driftstead", "QuestBarrel", "在工作台打开 1 个密封木桶。")},
        {EDriftsteadQuestStep::UpgradeLevel2, NSLOCTEXT("Driftstead", "QuestLevel2", "使用工作台将木筏升级到 2 级。")},
        {EDriftsteadQuestStep::HarvestCrop, NSLOCTEXT("Driftstead", "QuestHarvest", "种植并收获 1 次作物。")},
        {EDriftsteadQuestStep::UpgradeLevel4, NSLOCTEXT("Driftstead", "QuestLevel4", "将木筏升级到 4 级。")},
        {EDriftsteadQuestStep::ReachSecondFloor, NSLOCTEXT("Driftstead", "QuestFloor2", "使用楼梯到达第 2 层。")},
        {EDriftsteadQuestStep::Complete, NSLOCTEXT("Driftstead", "QuestComplete", "核心 Demo 流程完成，可继续建造或进入展示模式查看十级海上牧场。")}
    };
    return Instructions.FindRef(CurrentStep);
}
