#include "HookComponent.h"
#include "HookActor.h"
#include "CatchableInterface.h"
#include "DriftItemActor.h"
#include "DriftsteadCharacter.h"
#include "DriftsteadHUD.h"
#include "InventoryComponent.h"
#include "DriftsteadQuestSubsystem.h"
#include "TimerManager.h"
#include "DemoArt.h"

UHookComponent::UHookComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    HookActorClass = AHookActor::StaticClass();
}

void UHookComponent::BeginPlay()
{
    Super::BeginPlay();
    AimDirection = GetOwner()->GetActorForwardVector();
}

void UHookComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (AActor* Actor : AttachedActors) if (ADriftItemActor* Item = Cast<ADriftItemActor>(Actor)) if (IsValid(Item)) Item->ReleaseFromHook();
    if (GetWorld()) GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    if (IsValid(ActiveHook)) ActiveHook->Destroy();
    ActiveHook = nullptr;
    AttachedActors.Reset();
    RejectedActors.Reset();
    Super::EndPlay(EndPlayReason);
}

void UHookComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if ((State == EHookState::Flying || State == EHookState::Attached || State == EHookState::Returning) && !IsValid(ActiveHook))
    {
        const EHookState Previous = State;
        for (AActor* Actor : AttachedActors) if (ADriftItemActor* Item = Cast<ADriftItemActor>(Actor)) if (IsValid(Item)) Item->ReleaseFromHook();
        State = EHookState::Idle;
        ActiveHook = nullptr;
        AttachedActors.Reset();
        RejectedActors.Reset();
        OnHookStateChanged.Broadcast(Previous, State);
        return;
    }
    if (State == EHookState::Charging)
    {
        ChargeSeconds = FMath::Min(ChargeSeconds + DeltaTime, FullChargeSeconds);
    }
    else if (State == EHookState::Flying && ActiveHook)
    {
        const FVector PreviousLocation = ActiveHook->GetActorLocation();
        FVector NextLocation = PreviousLocation + FlightDirection * FlightSpeed * DeltaTime;
        NextLocation.Z = FMath::FInterpConstantTo(PreviousLocation.Z, CatchPlaneHeight, DeltaTime, CatchPlaneApproachSpeed);
        const float NextDistance = FVector::Dist2D(LaunchOrigin, NextLocation);
        if (NextDistance >= TargetRange)
        {
            NextLocation.X = LaunchOrigin.X + FlightDirection.X * TargetRange;
            NextLocation.Y = LaunchOrigin.Y + FlightDirection.Y * TargetRange;
        }
        ActiveHook->SetActorLocation(NextLocation, true);
        if (NextDistance >= TargetRange) BeginReturn();
    }
    else if ((State == EHookState::Returning || State == EHookState::Attached) && ActiveHook)
    {
        const FVector Origin = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, RecoveryPointVerticalOffset);
        const FVector PreviousLocation = ActiveHook->GetActorLocation();
        const FVector NextLocation = FMath::VInterpConstantTo(PreviousLocation, Origin, DeltaTime, ReturnSpeed);
        ActiveHook->SetActorLocation(NextLocation, true);
        TryCatchAlongReturnPath(PreviousLocation, ActiveHook->GetActorLocation());
        const float DistanceToPlayer = FVector::Dist2D(ActiveHook->GetActorLocation(), Origin);
        if (DistanceToPlayer < 130.0f)
            for (AActor* Actor : AttachedActors) if (IsValid(Actor)) Actor->SetActorLocation(FMath::VInterpConstantTo(Actor->GetActorLocation(), Origin, DeltaTime, ReturnSpeed * 1.8f));
        if (FVector::DistSquared(ActiveHook->GetActorLocation(), Origin) < FMath::Square(12.0f)) FinishReturn();
    }
}

void UHookComponent::StartCharging()
{
    if (State != EHookState::Idle) return;
    ChargeSeconds = 0.0f;
    SetState(EHookState::Charging);
}

void UHookComponent::ReleaseHook()
{
    if (State != EHookState::Charging) return;
    if (ChargeSeconds < MinimumChargeSeconds)
    {
        NotifyPlayer(NSLOCTEXT("Driftstead", "HookTooShort", "蓄力时间太短，请按住左键更久。"), FLinearColor::Yellow);
        SetState(EHookState::Cooldown);
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UHookComponent::EnterIdle);
        return;
    }

    TargetRange = FMath::Lerp(MinimumRange, MaximumRange, GetChargeAlpha());
    DriftsteadArt::Play(this,TEXT("Cast"));
    if (auto* Quest = GetWorld()->GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::ChargeHook);
    LaunchOrigin = GetRopeOrigin();
    FlightDirection = AimDirection;
    AttachedActors.Reset();
    RejectedActors.Reset();
    FActorSpawnParameters Parameters;
    Parameters.Owner = GetOwner();
    ActiveHook = GetWorld()->SpawnActor<AHookActor>(HookActorClass, LaunchOrigin, FlightDirection.Rotation(), Parameters);
    if (!ActiveHook)
    {
        SetState(EHookState::Idle);
        return;
    }
    ActiveHook->InitializeHook(this);
    SetState(EHookState::Flying);
}

void UHookComponent::RecallHook()
{
    if (State == EHookState::Charging)
    {
        ChargeSeconds = 0.0f;
        SetState(EHookState::Idle);
        return;
    }
    if (State == EHookState::Flying) BeginReturn();
}

void UHookComponent::SetAimDirection(FVector NewDirection)
{
    NewDirection.Z = 0.0f;
    if (!NewDirection.IsNearlyZero()) AimDirection = NewDirection.GetSafeNormal();
}

float UHookComponent::GetChargeAlpha() const
{
    return FMath::Clamp(ChargeSeconds / FullChargeSeconds, 0.0f, 1.0f);
}

FVector UHookComponent::GetEstimatedLandingPoint() const
{
    const float Range = State == EHookState::Charging ? FMath::Lerp(MinimumRange, MaximumRange, GetChargeAlpha()) : MaximumRange;
    FVector LandingPoint = GetRopeOrigin() + AimDirection * Range;
    LandingPoint.Z = CatchPlaneHeight;
    return LandingPoint;
}

FVector UHookComponent::GetRopeOrigin() const
{
    return GetOwner() ? GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 65.0f) : FVector::ZeroVector;
}

bool UHookComponent::IsTransitionAllowed(EHookState From, EHookState To)
{
    switch (From)
    {
    case EHookState::Idle: return To == EHookState::Charging;
    case EHookState::Charging: return To == EHookState::Flying || To == EHookState::Cooldown || To == EHookState::Idle;
    case EHookState::Flying: return To == EHookState::Attached || To == EHookState::Returning;
    case EHookState::Attached: return To == EHookState::Returning || To == EHookState::Cooldown;
    case EHookState::Returning: return To == EHookState::Attached || To == EHookState::Cooldown;
    case EHookState::Cooldown: return To == EHookState::Idle;
    default: return false;
    }
}

void UHookComponent::SetState(EHookState NewState)
{
    if (State == NewState) return;
    if (!IsTransitionAllowed(State, NewState))
    {
        UE_LOG(LogTemp, Warning, TEXT("Rejected illegal hook transition %d -> %d"), static_cast<int32>(State), static_cast<int32>(NewState));
        return;
    }
    const EHookState Previous = State;
    State = NewState;
    OnHookStateChanged.Broadcast(Previous, State);
}

void UHookComponent::TryAttachCatchable(AActor* OtherActor)
{
    if ((State != EHookState::Returning && State != EHookState::Attached) || !IsValid(ActiveHook) || !OtherActor ||
        AttachedActors.Contains(OtherActor) || RejectedActors.Contains(TWeakObjectPtr<AActor>(OtherActor)) ||
        !OtherActor->GetClass()->ImplementsInterface(UCatchableInterface::StaticClass())) return;
    if (!ICatchableInterface::Execute_CanBeCaught(OtherActor, HookCapacity))
    {
        RejectedActors.Add(TWeakObjectPtr<AActor>(OtherActor));
        NotifyPlayer(NSLOCTEXT("Driftstead", "TooHeavy", "物资太重，当前钩子无法拖动。"), FLinearColor::Red);
        return;
    }

    ICatchableInterface::Execute_OnCaught(OtherActor, ActiveHook);
    const int32 AttachedIndex = AttachedActors.Add(OtherActor);
    const int32 Row = AttachedIndex / 3;
    const int32 Slot = AttachedIndex % 3;
    const float LateralOffset = Slot == 0 ? 0.0f : (Slot == 1 ? -48.0f : 48.0f);
    OtherActor->SetActorRelativeLocation(FVector(42.0f + Row * 58.0f, LateralOffset, -28.0f - Row * 10.0f));
    OtherActor->SetActorRelativeRotation(FRotator::ZeroRotator);
    if (State == EHookState::Returning) SetState(EHookState::Attached);
}

void UHookComponent::TryCatchAlongReturnPath(const FVector& Start, const FVector& End)
{
    if ((State != EHookState::Returning && State != EHookState::Attached) || !GetWorld()) return;

    TArray<FHitResult> Hits;
    FCollisionObjectQueryParams ObjectQuery;
    ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(DriftsteadHookPlanarCatch), false, GetOwner());
    if (IsValid(ActiveHook)) Query.AddIgnoredActor(ActiveHook);

    // A tall vertical capsule makes catching a 2D gameplay decision: the hook
    // must cross the item's X/Y position, while presentation-only Z differences
    // between the ocean, hook and upper raft floors do not create false misses.
    const FCollisionShape CatchShape = FCollisionShape::MakeCapsule(PlanarCatchRadius, PlanarCatchHalfHeight);
    if (!GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjectQuery, CatchShape, Query)) return;

    // Outbound flight deliberately ignores catches. During return, collect
    // every valid target crossed by this swept segment, independent of Z.
    for (const FHitResult& Hit : Hits)
    {
        AActor* Candidate = Hit.GetActor();
        if (!Candidate || !Candidate->GetClass()->ImplementsInterface(UCatchableInterface::StaticClass())) continue;
        TryAttachCatchable(Candidate);
    }
}

void UHookComponent::BeginReturn()
{
    if (State != EHookState::Flying) return;
    SetState(EHookState::Returning);
}

void UHookComponent::FinishReturn()
{
    ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(GetOwner());
    int32 RecoveredCount = 0;
    int32 BasketCount = 0;
    static const TMap<FName, TPair<FName, int32>> ResourceConversion = {
        {TEXT("Driftwood"), {TEXT("Wood"), 2}}, {TEXT("Rope"), {TEXT("Rope"), 1}},
        {TEXT("ScrapMetal"), {TEXT("Metal"), 1}}, {TEXT("Cloth"), {TEXT("Cloth"), 1}},
        {TEXT("SeedCrate"), {TEXT("Seeds"), 2}}, {TEXT("FoodCrate"), {TEXT("Food"), 3}},
        {TEXT("MachineryCrate"), {TEXT("Parts"), 2}}, {TEXT("Electronics"), {TEXT("Parts"), 2}}
    };
    for (AActor* AttachedActor : AttachedActors)
    {
        ADriftItemActor* DriftItem = Cast<ADriftItemActor>(AttachedActor);
        if (!Character || !IsValid(DriftItem)) continue;
        DriftItem->PrepareForRecovery();
        const FName ItemId = DriftItem->GetItemId();
        const EInventoryAddResult Result = Character->GetInventory()->TryAddItem(ItemId, 1);
        if (const TPair<FName, int32>* Resource = ResourceConversion.Find(ItemId)) Character->GetInventory()->AddResource(Resource->Key, Resource->Value);
        if (UDriftsteadQuestSubsystem* Quest = Character->GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::SalvageItem);
        ++RecoveredCount;
        if (Result == EInventoryAddResult::RecoveryBasket) ++BasketCount;
        DriftItem->Destroy();
    }
    if (RecoveredCount == 0)
    {
        NotifyPlayer(NSLOCTEXT("Driftstead", "HookMiss", "回程没有碰到物资。"), FLinearColor(0.72f, 0.82f, 1.0f));
    }
    else if (BasketCount > 0)
    {
        NotifyPlayer(FText::Format(NSLOCTEXT("Driftstead", "MultiCatchBasket", "成功打捞 {0} 件物资，其中 {1} 件进入临时回收篮。"), FText::AsNumber(RecoveredCount), FText::AsNumber(BasketCount)), FLinearColor::Yellow);
    }
    else
    {
        NotifyPlayer(FText::Format(NSLOCTEXT("Driftstead", "MultiCatch", "成功打捞 {0} 件物资！"), FText::AsNumber(RecoveredCount)), FLinearColor::Green);
    }
    AttachedActors.Reset();
    RejectedActors.Reset();
    if (IsValid(ActiveHook)) ActiveHook->Destroy();
    if (RecoveredCount > 0) DriftsteadArt::Play(this,TEXT("Recover"));
    ActiveHook = nullptr;
    SetState(EHookState::Cooldown);
    FTimerHandle CooldownHandle;
    GetWorld()->GetTimerManager().SetTimer(CooldownHandle, this, &UHookComponent::EnterIdle, 0.25f, false);
}

void UHookComponent::EnterIdle()
{
    if (State == EHookState::Cooldown) SetState(EHookState::Idle);
}

void UHookComponent::NotifyPlayer(const FText& Message, FLinearColor Color) const
{
    if (const ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(GetOwner()))
    {
        Character->ShowFeedback(Message, Color);
    }
}
