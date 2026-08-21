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
    Inventory->AddResource(TEXT("Wood"), 8);
    Inventory->AddResource(TEXT("Rope"), 3);

    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Driftstead/Materials/M_WoodLight.M_WoodLight"));
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
}

void ADriftsteadCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float ForwardValue = (bForward ? 1.0f : 0.0f) - (bBackward ? 1.0f : 0.0f);
    const float RightValue = (bRight ? 1.0f : 0.0f) - (bLeft ? 1.0f : 0.0f);
    FVector ScreenForward = Camera->GetForwardVector(); ScreenForward.Z = 0.0f; ScreenForward.Normalize();
    FVector ScreenRight = Camera->GetRightVector(); ScreenRight.Z = 0.0f; ScreenRight.Normalize();
    if (!FMath::IsNearlyZero(ForwardValue)) AddMovementInput(ScreenForward, ForwardValue);
    if (!FMath::IsNearlyZero(RightValue)) AddMovementInput(ScreenRight, RightValue);
    if (!bMoveQuestNotified && (!FMath::IsNearlyZero(ForwardValue) || !FMath::IsNearlyZero(RightValue)))
    {
        bMoveQuestNotified = true;
        if (UDriftsteadQuestSubsystem* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::Move);
    }
    UpdateAim();
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

    auto BindDigital = [this, Enhanced](FKey Key, void (ADriftsteadCharacter::*Pressed)(), void (ADriftsteadCharacter::*Released)())
    {
        UInputAction* Action = CreateBooleanAction(Key);
        Enhanced->BindAction(Action, ETriggerEvent::Started, this, Pressed);
        if (Released) Enhanced->BindAction(Action, ETriggerEvent::Completed, this, Released);
    };

    BindDigital(EKeys::W, &ADriftsteadCharacter::MoveForwardOn, &ADriftsteadCharacter::MoveForwardOff);
    BindDigital(EKeys::S, &ADriftsteadCharacter::MoveBackwardOn, &ADriftsteadCharacter::MoveBackwardOff);
    BindDigital(EKeys::A, &ADriftsteadCharacter::MoveLeftOn, &ADriftsteadCharacter::MoveLeftOff);
    BindDigital(EKeys::D, &ADriftsteadCharacter::MoveRightOn, &ADriftsteadCharacter::MoveRightOff);
    BindDigital(EKeys::LeftMouseButton, &ADriftsteadCharacter::StartHook, &ADriftsteadCharacter::ReleaseHook);
    BindDigital(EKeys::RightMouseButton, &ADriftsteadCharacter::RecallHook, nullptr);
    BindDigital(EKeys::SpaceBar, &ADriftsteadCharacter::RecallHook, nullptr);
    BindDigital(EKeys::E, &ADriftsteadCharacter::Interact, nullptr);
    BindDigital(EKeys::Tab, &ADriftsteadCharacter::ToggleInventory, nullptr);
    BindDigital(EKeys::R, &ADriftsteadCharacter::RotateSelection, nullptr);
    BindDigital(EKeys::B, &ADriftsteadCharacter::RecoverFirstBasketItem, nullptr);
    BindDigital(EKeys::Escape, &ADriftsteadCharacter::TogglePause, nullptr);
    BindDigital(EKeys::F1, &ADriftsteadCharacter::AddDebugResources, nullptr);
    BindDigital(EKeys::F2, &ADriftsteadCharacter::ChangeRaftLevel, nullptr);
    BindDigital(EKeys::F3, &ADriftsteadCharacter::SpawnDebugItems, nullptr);
    BindDigital(EKeys::F4, &ADriftsteadCharacter::ChangeFloor, nullptr);
    BindDigital(EKeys::F5, &ADriftsteadCharacter::QuickSave, nullptr);
    BindDigital(EKeys::F6, &ADriftsteadCharacter::ToggleDeveloperPanel, nullptr);
    BindDigital(EKeys::F9, &ADriftsteadCharacter::ConfirmResetSave, nullptr);
    BindDigital(EKeys::Enter, &ADriftsteadCharacter::StartGameFromMenu, nullptr);
    BindDigital(EKeys::N, &ADriftsteadCharacter::StartNewNormalGame, nullptr);
    BindDigital(EKeys::C, &ADriftsteadCharacter::ContinueNormalGame, nullptr);
    BindDigital(EKeys::H, &ADriftsteadCharacter::StartShowcaseGame, nullptr);
    BindDigital(EKeys::Q, &ADriftsteadCharacter::QuitFromMenu, nullptr);

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
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;
    FVector Origin, Direction;
    if (!PC->DeprojectMousePositionToWorld(Origin, Direction)) return;
    const FPlane Plane(FVector(0, 0, GetActorLocation().Z), FVector::UpVector);
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
        if (HUD->IsInventoryOpen())
        {
            HUD->BeginInventoryDrag();
            return;
        }
    }
    Hook->StartCharging();
    if (UDriftsteadQuestSubsystem* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::ChargeHook);
}

void ADriftsteadCharacter::ReleaseHook()
{
    if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr)
    {
        if (HUD->IsDraggingInventoryItem())
        {
            HUD->EndInventoryDrag();
            return;
        }
        if (HUD->IsInventoryOpen()) return;
    }
    Hook->ReleaseHook();
}
void ADriftsteadCharacter::RecallHook() { Hook->RecallHook(); }

void ADriftsteadCharacter::Interact()
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
    if (Best) IInteractableInterface::Execute_Interact(Best, this);
    else ShowFeedback(NSLOCTEXT("Driftstead", "NoInteraction", "附近没有可交互的对象。"), FLinearColor::Yellow);
}

void ADriftsteadCharacter::ToggleInventory()
{
    if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr) HUD->ToggleInventory();
    if (UDriftsteadQuestSubsystem* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::OpenInventory);
}

void ADriftsteadCharacter::RotateSelection()
{
    if (Inventory->GetEntries().Num() == 0)
    {
        ShowFeedback(NSLOCTEXT("Driftstead", "RotateEmpty", "背包是空的。"), FLinearColor::Yellow);
        return;
    }
    if (IsShiftDown())
    {
        const bool bSplit = Inventory->SplitStack(Inventory->GetEntries()[0].InstanceId);
        ShowFeedback(bSplit ? NSLOCTEXT("Driftstead", "SplitSuccess", "物品堆已拆分到空闲格位。") : NSLOCTEXT("Driftstead", "SplitFailed", "数量不足，或背包没有可用空间。"), bSplit ? FLinearColor::Green : FLinearColor::Red);
        return;
    }
    if (Inventory->RotateItem(Inventory->GetEntries()[0].InstanceId))
    {
        ShowFeedback(NSLOCTEXT("Driftstead", "Rotated", "物品已旋转。"), FLinearColor::Green);
        if (UDriftsteadQuestSubsystem* Quest = GetGameInstance()->GetSubsystem<UDriftsteadQuestSubsystem>()) Quest->NotifyEvent(EDriftsteadQuestStep::RotateItem);
    }
    else ShowFeedback(NSLOCTEXT("Driftstead", "RotateFailed", "物品无法在当前位置旋转。"), FLinearColor::Red);
}

void ADriftsteadCharacter::RecoverFirstBasketItem()
{
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
    if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr) HUD->TogglePausePanel();
}

void ADriftsteadCharacter::AddDebugResources() { Inventory->AddTestResources(50); ShowFeedback(NSLOCTEXT("Driftstead", "DebugResources", "已添加测试资源。"), FLinearColor::Yellow); }
void ADriftsteadCharacter::ChangeRaftLevel() { if (ADriftsteadGameMode* GM = GetWorld()->GetAuthGameMode<ADriftsteadGameMode>()) GM->DebugChangeRaftLevel(IsShiftDown() ? -1 : 1); }
void ADriftsteadCharacter::SpawnDebugItems() { if (ADriftsteadGameMode* GM = GetWorld()->GetAuthGameMode<ADriftsteadGameMode>()) GM->SpawnDebugItems(); }
void ADriftsteadCharacter::ChangeFloor() { SetCurrentFloor(CurrentFloor + (IsShiftDown() ? -1 : 1)); }
void ADriftsteadCharacter::QuickSave() { if (UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(GetGameInstance())) GI->SaveCurrentGame(); }
void ADriftsteadCharacter::ToggleDeveloperPanel() { if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr) HUD->ToggleDeveloperPanel(); }

void ADriftsteadCharacter::ConfirmResetSave()
{
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
    UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(GetGameInstance());
    if (GI) GI->StartNewGame(bShowcase);

    TArray<FInventoryEntry> EmptyEntries;
    TMap<FName, int32> StartingResources;
    StartingResources.Add(TEXT("Wood"), bShowcase ? 250 : 8);
    StartingResources.Add(TEXT("Rope"), bShowcase ? 250 : 3);
    if (bShowcase)
    {
        for (const FName Resource : {FName(TEXT("Metal")), FName(TEXT("Cloth")), FName(TEXT("Seeds")), FName(TEXT("Food")), FName(TEXT("Water")), FName(TEXT("Parts")), FName(TEXT("Power"))}) StartingResources.Add(Resource, 250);
    }
    Inventory->RestoreState(bShowcase ? 12 : 6, bShowcase ? 8 : 4, EmptyEntries, EmptyEntries, StartingResources);
    if (ADriftsteadGameMode* GM = GetWorld()->GetAuthGameMode<ADriftsteadGameMode>()) GM->SetRaftLevelFromSave(bShowcase ? 10 : 1);
    bMoveQuestNotified = false;
    SetCurrentFloor(0);
    SetActorLocation(FVector(0, 0, 125), false, nullptr, ETeleportType::TeleportPhysics);
    if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr) HUD->CloseMainMenu();
    ShowFeedback(bShowcase ? NSLOCTEXT("Driftstead", "ShowcaseStarted", "展示模式已加载十级木筏与测试资源。") : NSLOCTEXT("Driftstead", "NewGameStarted", "新的漂海牧场已经启航。"), FLinearColor::Green);
}

void ADriftsteadCharacter::StartNewNormalGame()
{
    ResetRuntimeForMode(false);
}

void ADriftsteadCharacter::ContinueNormalGame()
{
    UDriftsteadGameInstance* GI = Cast<UDriftsteadGameInstance>(GetGameInstance());
    if (GI) GI->StartNewGame(false);
    if (!GI || !GI->LoadCurrentGame()) ResetRuntimeForMode(false);
    else if (ADriftsteadHUD* HUD = GetController<APlayerController>() ? Cast<ADriftsteadHUD>(GetController<APlayerController>()->GetHUD()) : nullptr) HUD->CloseMainMenu();
}

void ADriftsteadCharacter::StartShowcaseGame()
{
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
