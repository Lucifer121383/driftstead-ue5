#include "HookComponent.h"
#include "HookActor.h"
#include "CatchableInterface.h"
#include "DriftItemActor.h"
#include "DriftsteadCharacter.h"
#include "DriftsteadHUD.h"
#include "InventoryComponent.h"
#include "DriftsteadQuestSubsystem.h"
#include "TimerManager.h"

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
    if (GetWorld()) GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    if (IsValid(ActiveHook)) ActiveHook->Destroy();
    ActiveHook = nullptr;
    AttachedActor = nullptr;
    Super::EndPlay(EndPlayReason);
}

void UHookComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if ((State == EHookState::Flying || State == EHookState::Attached || State == EHookState::Returning) && !IsValid(ActiveHook))
    {
        const EHookState Previous = State;
        State = EHookState::Idle;
        ActiveHook = nullptr;
        AttachedActor = nullptr;
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
        const FVector NextLocation = PreviousLocation + FlightDirection * FlightSpeed * DeltaTime;
        ActiveHook->SetActorLocation(NextLocation, true);
        TryCatchAlongFlightPath(PreviousLocation, ActiveHook->GetActorLocation());
        if (State == EHookState::Flying && FVector::DistSquared2D(LaunchOrigin, ActiveHook->GetActorLocation()) >= FMath::Square(TargetRange)) BeginReturn(false);
    }
    else if (State == EHookState::Returning && ActiveHook)
    {
        const FVector Origin = GetRopeOrigin();
        ActiveHook->SetActorLocation(FMath::VInterpConstantTo(ActiveHook->GetActorLocation(), Origin, DeltaTime, ReturnSpeed), true);
        if (FVector::DistSquared(ActiveHook->GetActorLocation(), Origin) < FMath::Square(45.0f)) FinishReturn();
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
    LaunchOrigin = GetRopeOrigin();
    FlightDirection = AimDirection;
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
    if (State == EHookState::Flying || State == EHookState::Attached) BeginReturn(AttachedActor != nullptr);
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
    return GetRopeOrigin() + AimDirection * Range;
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
    case EHookState::Attached: return To == EHookState::Returning;
    case EHookState::Returning: return To == EHookState::Cooldown;
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

void UHookComponent::NotifyHookOverlap(AActor* OtherActor)
{
    if (State != EHookState::Flying || !OtherActor || !OtherActor->GetClass()->ImplementsInterface(UCatchableInterface::StaticClass())) return;
    if (!ICatchableInterface::Execute_CanBeCaught(OtherActor, HookCapacity))
    {
        NotifyPlayer(NSLOCTEXT("Driftstead", "TooHeavy", "物资太重，当前钩子无法拖动。"), FLinearColor::Red);
        BeginReturn(false);
        return;
    }
    AttachedActor = OtherActor;
    ICatchableInterface::Execute_OnCaught(OtherActor, ActiveHook);
    SetState(EHookState::Attached);
    BeginReturn(true);
}

void UHookComponent::TryCatchAlongFlightPath(const FVector& Start, const FVector& End)
{
    if (State != EHookState::Flying || !GetWorld()) return;

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

    for (const FHitResult& Hit : Hits)
    {
        AActor* Candidate = Hit.GetActor();
        if (!Candidate || !Candidate->GetClass()->ImplementsInterface(UCatchableInterface::StaticClass())) continue;
        NotifyHookOverlap(Candidate);
        if (State != EHookState::Flying) return;
    }
}

void UHookComponent::BeginReturn(bool bHitSomething)
{
    if (State != EHookState::Flying && State != EHookState::Attached) return;
    SetState(EHookState::Returning);
    if (!bHitSomething) NotifyPlayer(NSLOCTEXT("Driftstead", "HookMiss", "钩子落空了。"), FLinearColor(0.72f, 0.82f, 1.0f));
}

void UHookComponent::FinishReturn()
{
    ADriftsteadCharacter* Character = Cast<ADriftsteadCharacter>(GetOwner());
    ADriftItemActor* DriftItem = Cast<ADriftItemActor>(AttachedActor);
    if (Character && DriftItem)
    {
        DriftItem->PrepareForRecovery();
        const FName ItemId = DriftItem->GetItemId();
        const EInventoryAddResult Result = Character->GetInventory()->TryAddItem(ItemId, 1);
        static const TMap<FName, TPair<FName, int32>> ResourceConversion = {
            {TEXT("Driftwood"), {TEXT("Wood"), 2}}, {TEXT("Rope"), {TEXT("Rope"), 1}},
            {TEXT("ScrapMetal"), {TEXT("Metal"), 1}}, {TEXT("Cloth"), {TEXT("Cloth"), 1}},
            {TEXT("SeedCrate"), {TEXT("Seeds"), 2}}, {TEXT("FoodCrate"), {TEXT("Food"), 3}},
            {TEXT("MachineryCrate"), {TEXT("Parts"), 2}}, {TEXT("Electronics"), {TEXT("Parts"), 2}}
        };
        if (const TPair<FName, int32>* Resource = ResourceConversion.Find(ItemId)) Character->GetInventory()->AddResource(Resource->Key, Resource->Value);
        if (UDriftsteadQuestSubsystem* Quest = Character->GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::SalvageItem);
        NotifyPlayer(Result == EInventoryAddResult::RecoveryBasket ? NSLOCTEXT("Driftstead", "Basket", "背包已满——物资已转入临时回收篮。") : NSLOCTEXT("Driftstead", "Caught", "打捞成功！"), Result == EInventoryAddResult::RecoveryBasket ? FLinearColor::Yellow : FLinearColor::Green);
        DriftItem->Destroy();
    }
    AttachedActor = nullptr;
    if (IsValid(ActiveHook)) ActiveHook->Destroy();
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
