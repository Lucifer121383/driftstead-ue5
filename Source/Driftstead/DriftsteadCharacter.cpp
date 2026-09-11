#include "DriftsteadCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "InventoryComponent.h"
#include "HookComponent.h"
#include "InteractableInterface.h"
#include "DriftsteadHUD.h"
#include "DriftsteadGameMode.h"
#include "DriftsteadGameInstance.h"
#include "DriftsteadQuestSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "DemoArt.h"
#include "FacilityActor.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ADriftsteadCharacter::ADriftsteadCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(38.0f, 88.0f);
    GetCharacterMovement()->MaxWalkSpeed = 460.0f;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    bUseControllerRotationYaw = false;
    GetMesh()->SetHiddenInGame(true);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
    Hook = CreateDefaultSubobject<UHookComponent>(TEXT("Hook"));

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 1650.0f;
    CameraBoom->SetUsingAbsoluteRotation(true);
    CameraBoom->SetRelativeRotation(FRotator(-58.0f, -45.0f, 0.0f));
    CameraBoom->bDoCollisionTest = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
    Camera->OrthoWidth = 2200.0f;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    BodyMesh->SetupAttachment(RootComponent);
    BodyMesh->SetStaticMesh(Cylinder.Object);
    BodyMesh->SetRelativeLocation(FVector(0, 0, -10));
    BodyMesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 0.82f));
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
    HeadMesh->SetupAttachment(RootComponent);
    HeadMesh->SetStaticMesh(Sphere.Object);
    HeadMesh->SetRelativeLocation(FVector(0, 0, 62));
    HeadMesh->SetRelativeScale3D(FVector(0.48f));
    HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    HatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hat"));
    HatMesh->SetupAttachment(RootComponent);
    HatMesh->SetStaticMesh(Cylinder.Object);
    HatMesh->SetRelativeLocation(FVector(0, 0, 92));
    HatMesh->SetRelativeScale3D(FVector(0.62f, 0.62f, 0.12f));
    HatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BackpackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backpack"));
    BackpackMesh->SetupAttachment(RootComponent);
    BackpackMesh->SetStaticMesh(Cube.Object);
    BackpackMesh->SetRelativeLocation(FVector(-34, 0, 10));
    BackpackMesh->SetRelativeScale3D(FVector(0.25f, 0.42f, 0.52f));
    BackpackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADriftsteadCharacter::BeginPlay()
{
    Super::BeginPlay();
    Inventory->InitializeGrid(6, 4);
    Inventory->AddResource(TEXT("Wood"), 2);
    Inventory->AddResource(TEXT("Rope"), 1);
    GetWorldTimerManager().SetTimer(InteractionTimer, this, &ADriftsteadCharacter::UpdateInteractionTarget, .15f, true);

    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Driftstead/Materials/M_Character.M_Character"));
    const TArray<TPair<UStaticMeshComponent*, FLinearColor>> Colors = {
        {BodyMesh, FLinearColor(0.10f, 0.55f, 0.68f)}, {HeadMesh, FLinearColor(0.95f, 0.72f, 0.48f)},
        {HatMesh, FLinearColor(0.92f, 0.63f, 0.16f)}, {BackpackMesh, FLinearColor(0.34f, 0.18f, 0.08f)}
    };
    if (BaseMaterial)
    {
        for (const TPair<UStaticMeshComponent*, FLinearColor>& Pair : Colors)
        {
            UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            Material->SetVectorParameterValue(TEXT("BaseColor"), Pair.Value);
            Pair.Key->SetMaterial(0, Material);
        }
    }
    if (DriftsteadArt::Fit(BodyMesh, TEXT("Sailor"), 174, FVector(0,0,-88)))
    {
        bImportedSailor = true;
        SailorRestLocation = BodyMesh->GetRelativeLocation();
        BodyMesh->SetRelativeRotation(FRotator(0,-90,0));
        HeadMesh->SetVisibility(false); HatMesh->SetVisibility(false); BackpackMesh->SetVisibility(false);
    }
}

void ADriftsteadCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bReturnToMenuPending && (Hook->GetHookState() == EHookState::Idle || Hook->GetHookState() == EHookState::Cooldown))
    {
        FinishReturnToMainMenu();
        return;
    }
    const float ForwardValue = (bForward ? 1.0f : 0.0f) - (bBackward ? 1.0f : 0.0f);
    const float RightValue = (bRight ? 1.0f : 0.0f) - (bLeft ? 1.0f : 0.0f);
    FVector ScreenForward = Camera->GetForwardVector(); ScreenForward.Z = 0.0f; ScreenForward.Normalize();
    FVector ScreenRight = Camera->GetRightVector(); ScreenRight.Z = 0.0f; ScreenRight.Normalize();
    const bool bInputBlocked = IsGameplayInputBlocked(true);
    if (!bInputBlocked && !FMath::IsNearlyZero(ForwardValue)) AddMovementInput(ScreenForward, ForwardValue);
    if (!bInputBlocked && !FMath::IsNearlyZero(RightValue)) AddMovementInput(ScreenRight, RightValue);
    if (!bInputBlocked && !bMoveQuestNotified && (!FMath::IsNearlyZero(ForwardValue) || !FMath::IsNearlyZero(RightValue)))
    {
        bMoveQuestNotified = true;
        if (UDriftsteadQuestSubsystem* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::Move);
    }
    UpdateAim();
    if (bImportedSailor)
    {
        const float SpeedAlpha = FMath::Clamp(GetVelocity().Size2D()/460.0f, 0.0f, 1.0f);
        BodyMesh->SetRelativeLocation(SailorRestLocation + FVector(0,0,FMath::Abs(FMath::Sin(GetWorld()->GetTimeSeconds()*11))*5*SpeedAlpha));
        BodyMesh->SetRelativeRotation(FRotator(0,-90,FMath::Sin(GetWorld()->GetTimeSeconds()*11)*4*SpeedAlpha));
    }
    if (GetActorLocation().Z < -180.0f)
    {
        GetCharacterMovement()->StopMovementImmediately();
        SetCurrentFloor(0, false);
        SetActorLocation(FVector(0, 0, 125), false, nullptr, ETeleportType::TeleportPhysics);
        ShowFeedback(NSLOCTEXT("Driftstead", "OceanRescue", "你落入海中，已安全返回木筏。"), FLinearColor::Yellow);
    }
    if (bResetArmed && FPlatformTime::Seconds() - ResetArmTime > 3.0) bResetArmed = false;
}

UInputAction* ADriftsteadCharacter::CreateBooleanAction(FKey Key)
{
    UInputAction* Action = NewObject<UInputAction>(this);
    Action->ValueType = EInputActionValueType::Boolean;
    RuntimeActions.Add(Action);
    RuntimeMappingContext->MapKey(Action, Key);
    return Action;
}

void ADriftsteadCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    UEnhancedInputComponent* Enhanced = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
    RuntimeMappingContext = NewObject<UInputMappingContext>(this);

    auto BindDigital = [this, Enhanced](FKey Key, void (ADriftsteadCharacter::*Pressed)(), void (ADriftsteadCharacter::*Released)(), bool bWhilePaused = false)
    {
        UInputAction* Action = CreateBooleanAction(Key);
        Action->bTriggerWhenPaused = bWhilePaused;
        Enhanced->BindAction(Action, ETriggerEvent::Started, this, Pressed);
        if (Released) Enhanced->BindAction(Action, ETriggerEvent::Completed, this, Released);
    };

    BindDigital(EKeys::W, &ADriftsteadCharacter::MoveForwardOn, &ADriftsteadCharacter::MoveForwardOff);
    BindDigital(EKeys::S, &ADriftsteadCharacter::MoveBackwardOn, &ADriftsteadCharacter::MoveBackwardOff);
    BindDigital(EKeys::A, &ADriftsteadCharacter::MoveLeftOn, &ADriftsteadCharacter::MoveLeftOff);
    BindDigital(EKeys::D, &ADriftsteadCharacter::MoveRightOn, &ADriftsteadCharacter::MoveRightOff);
    BindDigital(EKeys::LeftMouseButton, &ADriftsteadCharacter::StartHook, &ADriftsteadCharacter::ReleaseHook, true);
    BindDigital(EKeys::RightMouseButton, &ADriftsteadCharacter::RecallHook, nullptr);
    BindDigital(EKeys::SpaceBar, &ADriftsteadCharacter::RecallHook, nullptr);
    BindDigital(EKeys::E, &ADriftsteadCharacter::Interact, nullptr);
    BindDigital(EKeys::U, &ADriftsteadCharacter::UpgradeAtWorkbench, nullptr);
    BindDigital(EKeys::Tab, &ADriftsteadCharacter::ToggleInventory, nullptr);
    BindDigital(EKeys::R, &ADriftsteadCharacter::RotateSelection, nullptr);
    BindDigital(EKeys::B, &ADriftsteadCharacter::RecoverFirstBasketItem, nullptr);
    UInputAction* PauseAction = CreateBooleanAction(EKeys::Escape);
    PauseAction->bTriggerWhenPaused = true;
    Enhanced->BindAction(PauseAction, ETriggerEvent::Started, this, &ADriftsteadCharacter::TogglePause);
    BindDigital(EKeys::F1, &ADriftsteadCharacter::AddDebugResources, nullptr);
    BindDigital(EKeys::F2, &ADriftsteadCharacter::ChangeRaftLevel, nullptr);
    BindDigital(EKeys::F3, &ADriftsteadCharacter::SpawnDebugItems, nullptr);
    BindDigital(EKeys::F4, &ADriftsteadCharacter::ChangeFloor, nullptr);
    UInputAction* SaveAction = CreateBooleanAction(EKeys::F5);
    SaveAction->bTriggerWhenPaused = true;
    Enhanced->BindAction(SaveAction, ETriggerEvent::Started, this, &ADriftsteadCharacter::QuickSave);
    BindDigital(EKeys::F6, &ADriftsteadCharacter::ToggleDeveloperPanel, nullptr);
    BindDigital(EKeys::F9, &ADriftsteadCharacter::ConfirmResetSave, nullptr);
    BindDigital(EKeys::Enter, &ADriftsteadCharacter::StartGameFromMenu, nullptr, true);
    BindDigital(EKeys::N, &ADriftsteadCharacter::StartNewNormalGame, nullptr, true);
    BindDigital(EKeys::C, &ADriftsteadCharacter::ContinueNormalGame, nullptr, true);
    BindDigital(EKeys::H, &ADriftsteadCharacter::StartShowcaseGame, nullptr, true);
    BindDigital(EKeys::Q, &ADriftsteadCharacter::QuitFromMenu, nullptr, true);
    BindDigital(EKeys::M, &ADriftsteadCharacter::RequestReturnToMainMenu, nullptr, true);

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Subsystem->ClearAllMappings();
            Subsystem->AddMappingContext(RuntimeMappingContext, 0);
        }
    }
}

void ADriftsteadCharacter::UpdateAim()
{
    if (FParse::Param(FCommandLine::Get(), TEXT("DriftsteadMenuTest"))) return;
    if (IsGameplayInputBlocked(true) || FParse::Param(FCommandLine::Get(), TEXT("DriftsteadCapture")) || FParse::Param(FCommandLine::Get(), TEXT("DriftsteadSmokeTest")) || FParse::Param(FCommandLine::Get(), TEXT("DriftsteadOpeningTest"))) return;
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;
    FVector Origin, Direction;
    if (!PC->DeprojectMousePositionToWorld(Origin, Direction)) return;
    const FPlane Plane(FVector(0, 0, 35.0f), FVector::UpVector);
    const FVector Target = FMath::LinePlaneIntersection(Origin, Origin + Direction * 100000.0f, Plane);
    FVector Aim = Target - GetActorLocation(); Aim.Z = 0.0f;
    if (!Aim.IsNearlyZero())
    {
        SetActorRotation(Aim.Rotation());
        Hook->SetAimDirection(Aim);
    }
}

bool ADriftsteadCharacter::IsShiftDown() const
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    return PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift));
}

bool ADriftsteadCharacter::IsGameplayInputBlocked(bool bIncludeInventory) const
{
    if (bReturnToMenuPending) return true;
    const APlayerController* PC = Cast<APlayerController>(GetController());
    const ADriftsteadHUD* HUD = PC ? Cast<ADriftsteadHUD>(PC->GetHUD()) : nullptr;
    return HUD && (HUD->IsMainMenuOpen() || HUD->IsPausePanelOpen() || (bIncludeInventory && HUD->IsInventoryOpen()));
}

void ADriftsteadCharacter::MoveForwardOn() { bForward = true; }
void ADriftsteadCharacter::MoveForwardOff() { bForward = false; }
void ADriftsteadCharacter::MoveBackwardOn() { bBackward = true; }
void ADriftsteadCharacter::MoveBackwardOff() { bBackward = false; }
void ADriftsteadCharacter::MoveLeftOn() { bLeft = true; }
void ADriftsteadCharacter::MoveLeftOff() { bLeft = false; }
void ADriftsteadCharacter::MoveRightOn() { bRight = true; }
void ADriftsteadCharacter::MoveRightOff() { bRight = false; }
void ADriftsteadCharacter::StartHook()
{
    if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr)
    {
        if (HUD->IsMainMenuOpen()) { HUD->HandleMenuClick(); return; }
        if (HUD->IsPausePanelOpen()) { HUD->HandlePauseClick(); return; }
        if (HUD->IsInventoryOpen())
        {
            HUD->BeginInventoryDrag();
            return;
        }
    }
    if (IsGameplayInputBlocked(true)) return;
    bHookPressOwned = true;
    Hook->StartCharging();
}

void ADriftsteadCharacter::ReleaseHook()
{
    if (IsGameplayInputBlocked()) { bHookPressOwned = false; return; }
    if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr)
    {
        if (HUD->IsDraggingInventoryItem())
        {
            HUD->EndInventoryDrag();
            return;
        }
        if (HUD->IsInventoryOpen()) return;
    }
    if (bHookPressOwned) Hook->ReleaseHook();
    bHookPressOwned = false;
}
void ADriftsteadCharacter::RecallHook() { if (!IsGameplayInputBlocked(true)) Hook->RecallHook(); }

void ADriftsteadCharacter::UpdateInteractionTarget()
{
    TArray<FOverlapResult> Results;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(DriftsteadInteraction), false, this);
    GetWorld()->OverlapMultiByChannel(Results, GetActorLocation(), FQuat::Identity, ECC_WorldDynamic, FCollisionShape::MakeSphere(180.0f), Query);
    AActor* Best = nullptr;
    float BestDistance = TNumericLimits<float>::Max();
    for (const FOverlapResult& Result : Results)
    {
        AActor* Candidate = Result.GetActor();
        if (!Candidate || !Candidate->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass())) continue;
        if (IInteractableInterface::Execute_GetInteractionFloor(Candidate) != CurrentFloor) continue;
        const float Distance = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
        if (Distance < BestDistance) { Best = Candidate; BestDistance = Distance; }
    }
    InteractionTarget = Best;
}

void ADriftsteadCharacter::Interact()
{
    if (IsGameplayInputBlocked(true)) return;
    UpdateInteractionTarget();
    if (InteractionTarget.IsValid()) IInteractableInterface::Execute_Interact(InteractionTarget.Get(), this);
    else ShowFeedback(NSLOCTEXT("Driftstead", "NoInteraction", "附近没有可交互的对象。"), FLinearColor::Yellow);
}

void ADriftsteadCharacter::UpgradeAtWorkbench()
{
    if (IsGameplayInputBlocked(true)) return;
    UpdateInteractionTarget();
    if (auto* Facility = Cast<AFacilityActor>(InteractionTarget.Get()))
        if (Facility->GetFacilityType() == EFacilityType::Workbench) { Facility->TryUpgrade(this); return; }
    ShowFeedback(FText::FromString(TEXT("靠近工作台后按 U 扩建。")), FLinearColor::Yellow);
}

void ADriftsteadCharacter::SelectMenuOption(int32 Option)
{
    if (Option == 0) StartNewNormalGame();
    else if (Option == 1) ContinueNormalGame();
    else if (Option == 2) StartShowcaseGame();
}

void ADriftsteadCharacter::ToggleInventory()
{
    if (IsGameplayInputBlocked()) return;
    ResetTransientInput();
    if (Hook->GetHookState() == EHookState::Charging) Hook->RecallHook();
    if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr) HUD->ToggleInventory();
    if (UDriftsteadQuestSubsystem* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::OpenInventory);
}

void ADriftsteadCharacter::RotateSelection()
{
    if (IsGameplayInputBlocked()) return;
    if (Inventory->GetEntries().Num() == 0)
    {
        ShowFeedback(NSLOCTEXT("Driftstead", "RotateEmpty", "背包是空的。"), FLinearColor::Yellow);
        return;
    }
    FGuid TargetId = Inventory->GetEntries()[0].InstanceId;
    if (const APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (const ADriftsteadHUD* HUD = Cast<ADriftsteadHUD>(PC->GetHUD()))
        {
            const FGuid SelectedId = HUD->GetSelectedInventoryItemId();
            if (SelectedId.IsValid() && Inventory->GetEntries().ContainsByPredicate([SelectedId](const FInventoryEntry& Entry) { return Entry.InstanceId == SelectedId; }))
            {
                TargetId = SelectedId;
            }
        }
    }
    if (IsShiftDown())
    {
        const bool bSplit = Inventory->SplitStack(TargetId);
        ShowFeedback(bSplit ? NSLOCTEXT("Driftstead", "SplitSuccess", "物品堆已拆分到空闲格位。") : NSLOCTEXT("Driftstead", "SplitFailed", "数量不足，或背包没有可用空间。"), bSplit ? FLinearColor::Green : FLinearColor::Red);
        return;
    }
    if (Inventory->RotateItem(TargetId))
    {
        ShowFeedback(NSLOCTEXT("Driftstead", "Rotated", "物品已旋转。"), FLinearColor::Green);
        if (UDriftsteadQuestSubsystem* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::RotateItem);
    }
    else ShowFeedback(NSLOCTEXT("Driftstead", "RotateFailed", "物品无法在当前位置旋转。"), FLinearColor::Red);
}

void ADriftsteadCharacter::RecoverFirstBasketItem()
{
    if (IsGameplayInputBlocked()) return;
    if (Inventory->GetRecoveryBasket().Num() == 0)
    {
        ShowFeedback(NSLOCTEXT("Driftstead", "BasketEmpty", "临时回收篮是空的。"), FLinearColor::Yellow);
        return;
    }
    const bool bRecovered = Inventory->RecoverFromBasket(Inventory->GetRecoveryBasket()[0].InstanceId);
    ShowFeedback(bRecovered ? NSLOCTEXT("Driftstead", "BasketRecovered", "物品已放回背包。") : NSLOCTEXT("Driftstead", "BasketStillFull", "请先在背包中腾出空间。"), bRecovered ? FLinearColor::Green : FLinearColor::Yellow);
}

void ADriftsteadCharacter::TogglePause()
{
    if (bReturnToMenuPending) return;
    if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr)
    {
        if (HUD->IsMainMenuOpen()) return;
        if (HUD->IsInventoryOpen()) { HUD->SetInventoryOpen(false); return; }
        ResetTransientInput();
        if (Hook->GetHookState() == EHookState::Charging) Hook->RecallHook();
        HUD->TogglePausePanel();
        UGameplayStatics::SetGamePaused(this, HUD->IsPausePanelOpen());
    }
}

void ADriftsteadCharacter::ResetTransientInput()
{
    bForward = bBackward = bLeft = bRight = false;
    bHookPressOwned = bResetArmed = false;
    InteractionTarget.Reset();
    ConsumeMovementInputVector();
    GetCharacterMovement()->StopMovementImmediately();
}

void ADriftsteadCharacter::SelectPauseOption(int32 Option)
{
    auto* PC = GetController<APlayerController>();
    auto* HUD = PC ? Cast<ADriftsteadHUD>(PC->GetHUD()) : nullptr;
    if (!HUD || !HUD->IsPausePanelOpen() || HUD->IsMainMenuOpen()) return;
    if (Option == 0) TogglePause();
    else if (Option == 1) QuickSave();
    else if (Option == 2) RequestReturnToMainMenu();
}

void ADriftsteadCharacter::RequestReturnToMainMenu()
{
    auto* PC = GetController<APlayerController>();
    auto* HUD = PC ? Cast<ADriftsteadHUD>(PC->GetHUD()) : nullptr;
    if (!HUD || !HUD->IsPausePanelOpen() || HUD->IsMainMenuOpen() || bReturnToMenuPending) return;
    ResetTransientInput();
    HUD->ResetTransientUI();
    bReturnToMenuPending = true;
    // Keep the current world alive until all attached cargo reaches the backpack.
    UGameplayStatics::SetGamePaused(this, false);
    Hook->RecallHook();
    if (Hook->GetHookState() == EHookState::Idle || Hook->GetHookState() == EHookState::Cooldown) FinishReturnToMainMenu();
    else ShowFeedback(FText::FromString(TEXT("正在收回钩子和物资，随后保存并返回主菜单……")), FLinearColor(1,.8f,.43f));
}

void ADriftsteadCharacter::FinishReturnToMainMenu()
{
    auto* PC = GetController<APlayerController>();
    auto* HUD = PC ? Cast<ADriftsteadHUD>(PC->GetHUD()) : nullptr;
    auto* GI = Cast<UDriftsteadGameInstance>(GetGameInstance());
    bReturnToMenuPending = false;
    ResetTransientInput();
    if (HUD && GI && GI->SaveCurrentGame())
    {
        bSuspendedSession = true;
        HUD->OpenMainMenu();
    }
    else if (HUD)
    {
        if (!HUD->IsPausePanelOpen()) HUD->TogglePausePanel();
        ShowFeedback(FText::FromString(TEXT("保存失败，已保留当前航程。请重试或继续游戏。")), FLinearColor::Red);
    }
    UGameplayStatics::SetGamePaused(this, true);
}

void ADriftsteadCharacter::AddDebugResources()
{
    if (IsGameplayInputBlocked()) return;
    Inventory->AddTestResources(100);
    ShowFeedback(NSLOCTEXT("Driftstead", "ResourceSupply100", "补给已到账：全部 9 种资源各 +100。"), FLinearColor::Yellow);
    if (FParse::Param(FCommandLine::Get(), TEXT("DriftsteadManualQA")))
    {
        for (const auto& Resource : Inventory->GetResources())
            UE_LOG(LogTemp, Display, TEXT("[ManualQA] F1 supply %s=%d"), *Resource.Key.ToString(), Resource.Value);
    }
}
void ADriftsteadCharacter::ChangeRaftLevel() { if (IsGameplayInputBlocked()) return; if (ADriftsteadGameMode* GM = GetWorld()->GetAuthGameMode<ADriftsteadGameMode>()) GM->DebugChangeRaftLevel(IsShiftDown() ? -1 : 1); }
void ADriftsteadCharacter::SpawnDebugItems() { if (IsGameplayInputBlocked()) return; if (ADriftsteadGameMode* GM = GetWorld()->GetAuthGameMode<ADriftsteadGameMode>()) GM->SpawnDebugItems(); }
void ADriftsteadCharacter::ChangeFloor() { if (IsGameplayInputBlocked()) return; SetCurrentFloor(CurrentFloor + (IsShiftDown() ? -1 : 1)); }
void ADriftsteadCharacter::QuickSave()
{
    if (bReturnToMenuPending) return;
    if (const auto* PC=GetController<APlayerController>()) if (const auto* HUD=Cast<ADriftsteadHUD>(PC->GetHUD())) if (HUD->IsMainMenuOpen()) return;
    if (UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(GetGameInstance())) GI->SaveCurrentGame();
}
void ADriftsteadCharacter::ToggleDeveloperPanel() { if (IsGameplayInputBlocked()) return; if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr) HUD->ToggleDeveloperPanel(); }

void ADriftsteadCharacter::ConfirmResetSave()
{
    if (IsGameplayInputBlocked()) return;
    if (!bResetArmed)
    {
        bResetArmed = true;
        ResetArmTime = FPlatformTime::Seconds();
        ShowFeedback(NSLOCTEXT("Driftstead", "ResetConfirm", "请在 3 秒内再次按 F9，确认重置存档。"), FLinearColor::Red);
    }
    else if (UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(GetGameInstance()))
    {
        GI->ResetAllSaves();
        bResetArmed = false;
    }
}

void ADriftsteadCharacter::StartGameFromMenu()
{
    ContinueNormalGame();
}

void ADriftsteadCharacter::QuitFromMenu()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    ADriftsteadHUD* HUD = PC ? Cast<ADriftsteadHUD>(PC->GetHUD()) : nullptr;
    if (HUD && HUD->IsMainMenuOpen()) UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
}

void ADriftsteadCharacter::ResetRuntimeForMode(bool bShowcase)
{
    ResetTransientInput();
    bSuspendedSession = bReturnToMenuPending = false;
    UGameplayStatics::SetGamePaused(this, false);
    if (auto* PC = GetController<APlayerController>()) if (auto* HUD = Cast<ADriftsteadHUD>(PC->GetHUD())) HUD->ResetTransientUI();
    UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(GetGameInstance());
    if (GI) GI->StartNewGame(bShowcase);

    TArray<FInventoryEntry> EmptyEntries;
    TMap<FName, int32> StartingResources;
    StartingResources.Add(TEXT("Wood"), bShowcase ? 250 : 2);
    StartingResources.Add(TEXT("Rope"), bShowcase ? 250 : 1);
    if (bShowcase)
    {
        for (const FName Resource : {FName(TEXT("Metal")), FName(TEXT("Cloth")), FName(TEXT("Seeds")), FName(TEXT("Food")), FName(TEXT("Water")), FName(TEXT("Parts")), FName(TEXT("Power"))}) StartingResources.Add(Resource, 250);
    }
    Inventory->RestoreState(bShowcase ? 12 : 6, bShowcase ? 8 : 4, EmptyEntries, EmptyEntries, StartingResources);
    if (ADriftsteadGameMode* GM = GetWorld()->GetAuthGameMode<ADriftsteadGameMode>()) { GM->SetRaftLevelFromSave(bShowcase ? 10 : 1); GM->RestoreFacilityStates({}); GM->ResetOpeningSupplies(); }
    bMoveQuestNotified = false;
    SetCurrentFloor(0);
    SetActorLocation(FVector(0, 0, 125), false, nullptr, ETeleportType::TeleportPhysics);
    if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr) HUD->CloseMainMenu();
    ShowFeedback(bShowcase ? NSLOCTEXT("Driftstead", "ShowcaseStarted", "展示模式已加载十级木筏与测试资源。") : NSLOCTEXT("Driftstead", "NewGameStarted", "新的漂海牧场已经启航。"), FLinearColor::Green);
}

void ADriftsteadCharacter::StartNewNormalGame()
{
    if (const auto* PC = GetController<APlayerController>()) if (const auto* HUD = Cast<ADriftsteadHUD>(PC->GetHUD())) if (!HUD->IsMainMenuOpen()) return;
    ResetRuntimeForMode(false);
}

void ADriftsteadCharacter::ContinueNormalGame()
{
    if (const auto* PC = GetController<APlayerController>()) if (const auto* HUD = Cast<ADriftsteadHUD>(PC->GetHUD())) if (!HUD->IsMainMenuOpen()) return;
    ResetTransientInput();
    UGameplayStatics::SetGamePaused(this, false);
    if (bSuspendedSession)
    {
        bSuspendedSession = false;
        if (auto* PC = GetController<APlayerController>()) if (auto* HUD = Cast<ADriftsteadHUD>(PC->GetHUD())) { HUD->ResetTransientUI(); HUD->CloseMainMenu(); }
        ShowFeedback(FText::FromString(TEXT("已继续当前航程。")), FLinearColor::Green);
        return;
    }
    UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(GetGameInstance());
    if (GI) GI->StartNewGame(false);
    if (!GI || !GI->LoadCurrentGame()) ResetRuntimeForMode(false);
    else if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr) HUD->CloseMainMenu();
}

void ADriftsteadCharacter::StartShowcaseGame()
{
    if (const auto* PC = GetController<APlayerController>()) if (const auto* HUD = Cast<ADriftsteadHUD>(PC->GetHUD())) if (!HUD->IsMainMenuOpen()) return;
    ResetRuntimeForMode(true);
}

void ADriftsteadCharacter::SetCurrentFloor(int32 NewFloor, bool bTeleport)
{
    int32 MaxFloor = 0;
    if (ADriftsteadGameMode* GM = GetWorld()->GetAuthGameMode<ADriftsteadGameMode>()) MaxFloor = GM->GetMaximumFloor();
    CurrentFloor = FMath::Clamp(NewFloor, 0, MaxFloor);
    if (bTeleport)
    {
        FVector Location = GetActorLocation();
        Location.Z = 110.0f + CurrentFloor * 340.0f;
        SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
    }
    if (ADriftsteadGameMode* GM = GetWorld()->GetAuthGameMode<ADriftsteadGameMode>()) GM->SetViewedFloor(CurrentFloor);
    if (CurrentFloor >= 1)
        if (UDriftsteadQuestSubsystem* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::ReachSecondFloor);
    ShowFeedback(FText::Format(NSLOCTEXT("Driftstead", "FloorChanged", "已到达第 {0} 层"), FText::AsNumber(CurrentFloor + 1)), FLinearColor::White);
}

void ADriftsteadCharacter::ShowFeedback(const FText& Message, FLinearColor Color) const
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
        if (ADriftsteadHUD* HUD = Cast<ADriftsteadHUD>(PC->GetHUD())) HUD->ShowFeedback(Message, Color);
    }
}
