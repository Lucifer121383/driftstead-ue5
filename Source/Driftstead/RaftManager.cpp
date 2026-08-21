#include "RaftManager.h"
#include "FacilityActor.h"
#include "StairActor.h"
#include "InventoryComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ARaftManager::ARaftManager()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
    FloorOne = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorOne")); FloorOne->SetupAttachment(SceneRoot);
    FloorTwo = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorTwo")); FloorTwo->SetupAttachment(SceneRoot);
    FloorThree = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorThree")); FloorThree->SetupAttachment(SceneRoot);
    RailsOne = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RailsOne")); RailsOne->SetupAttachment(SceneRoot);
    RailsTwo = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RailsTwo")); RailsTwo->SetupAttachment(SceneRoot);
    RailsThree = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RailsThree")); RailsThree->SetupAttachment(SceneRoot);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodLight(TEXT("/Game/Driftstead/Materials/M_WoodLight.M_WoodLight"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodDark(TEXT("/Game/Driftstead/Materials/M_WoodDark.M_WoodDark"));
    for (UInstancedStaticMeshComponent* Component : {FloorOne, FloorTwo, FloorThree, RailsOne, RailsTwo, RailsThree})
    {
        if (Cube.Succeeded()) Component->SetStaticMesh(Cube.Object);
        Component->SetMobility(EComponentMobility::Movable);
    }
    for (UInstancedStaticMeshComponent* Floor : {FloorOne, FloorTwo, FloorThree})
        if (WoodLight.Succeeded()) Floor->SetMaterial(0, WoodLight.Object);
    for (UInstancedStaticMeshComponent* Rails : {RailsOne, RailsTwo, RailsThree})
        if (WoodDark.Succeeded()) Rails->SetMaterial(0, WoodDark.Object);
    for (UInstancedStaticMeshComponent* Rails : {RailsOne, RailsTwo, RailsThree}) Rails->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void ARaftManager::BeginPlay()
{
    Super::BeginPlay();
    GenerateRaft();
}

bool ARaftManager::TryUpgrade(UInventoryComponent* Inventory)
{
    if (!Inventory || RaftLevel >= 10) return false;
    const FRaftLevelDefinition* Next = FRaftProgressionCatalog::Find(RaftLevel + 1);
    if (!Next || !Inventory->ConsumeResourcesTransactional(Next->UpgradeCost)) return false;
    ForceLevel(RaftLevel + 1);
    return true;
}

void ARaftManager::ForceLevel(int32 NewLevel)
{
    const int32 Clamped = FMath::Clamp(NewLevel, 1, 10);
    if (RaftLevel == Clamped && FloorOne->GetInstanceCount() > 0) return;
    RaftLevel = Clamped;
    ViewedFloor = FMath::Min(ViewedFloor, GetMaximumFloor());
    GenerateRaft();
    OnRaftLevelChanged.Broadcast(RaftLevel);
}

int32 ARaftManager::GetMaximumFloor() const
{
    const FRaftLevelDefinition* Definition = FRaftProgressionCatalog::Find(RaftLevel);
    if (!Definition) return 0;
    if (Definition->ThirdFloor.X > 0) return 2;
    if (Definition->SecondFloor.X > 0) return 1;
    return 0;
}

TArray<FFacilitySaveState> ARaftManager::CaptureFacilityStates() const
{
    TArray<FFacilitySaveState> States;
    for (const AActor* Actor : GeneratedActors)
    {
        if (const AFacilityActor* Facility = Cast<AFacilityActor>(Actor)) States.Add(Facility->CaptureSaveState());
    }
    return States;
}

void ARaftManager::RestoreFacilityStates(const TArray<FFacilitySaveState>& States)
{
    TSet<int32> RestoredActorIndices;
    for (const FFacilitySaveState& State : States)
    {
        for (int32 Index = 0; Index < GeneratedActors.Num(); ++Index)
        {
            if (RestoredActorIndices.Contains(Index)) continue;
            AFacilityActor* Facility = Cast<AFacilityActor>(GeneratedActors[Index]);
            if (!Facility || Facility->GetFacilityType() != State.FacilityType || IInteractableInterface::Execute_GetInteractionFloor(Facility) != State.FloorIndex) continue;
            Facility->RestoreSaveState(State);
            RestoredActorIndices.Add(Index);
            break;
        }
    }
}

void ARaftManager::GenerateRaft()
{
    const FRaftLevelDefinition* Definition = FRaftProgressionCatalog::Find(RaftLevel);
    if (!Definition) return;
    DestroyGeneratedActors();
    FloorOne->ClearInstances(); FloorTwo->ClearInstances(); FloorThree->ClearInstances();
    RailsOne->ClearInstances(); RailsTwo->ClearInstances(); RailsThree->ClearInstances();
    GenerateFloor(FloorOne, RailsOne, Definition->FirstFloor, 0);
    GenerateFloor(FloorTwo, RailsTwo, Definition->SecondFloor, 1);
    GenerateFloor(FloorThree, RailsThree, Definition->ThirdFloor, 2);
    SpawnFacilities(*Definition);
    SetViewedFloor(ViewedFloor);
}

void ARaftManager::GenerateFloor(UInstancedStaticMeshComponent* Floor, UInstancedStaticMeshComponent* Rails, FIntPoint Dimensions, int32 FloorIndex)
{
    if (Dimensions.X <= 0 || Dimensions.Y <= 0) return;
    const float Z = FloorIndex * FloorHeight;
    for (int32 Y = 0; Y < Dimensions.Y; ++Y)
    {
        for (int32 X = 0; X < Dimensions.X; ++X)
        {
            const FVector Position((X - (Dimensions.X - 1) * 0.5f) * TileSize, (Y - (Dimensions.Y - 1) * 0.5f) * TileSize, Z);
            Floor->AddInstance(FTransform(FRotator::ZeroRotator, Position, FVector(TileSize / 100.0f * 0.98f, TileSize / 100.0f * 0.98f, 0.20f)));
        }
    }
    for (int32 X = 0; X < Dimensions.X; ++X)
    {
        for (int32 EdgeY : {0, Dimensions.Y - 1})
        {
            const FVector P((X - (Dimensions.X - 1) * 0.5f) * TileSize, (EdgeY - (Dimensions.Y - 1) * 0.5f) * TileSize, Z + 65.0f);
            Rails->AddInstance(FTransform(FRotator::ZeroRotator, P, FVector(TileSize / 100.0f, 0.10f, 0.62f)));
        }
    }
    for (int32 Y = 1; Y < Dimensions.Y - 1; ++Y)
    {
        for (int32 EdgeX : {0, Dimensions.X - 1})
        {
            const FVector P((EdgeX - (Dimensions.X - 1) * 0.5f) * TileSize, (Y - (Dimensions.Y - 1) * 0.5f) * TileSize, Z + 65.0f);
            Rails->AddInstance(FTransform(FRotator::ZeroRotator, P, FVector(0.10f, TileSize / 100.0f, 0.62f)));
        }
    }
}

void ARaftManager::SpawnFacilities(const FRaftLevelDefinition& Definition)
{
    const int32 MaxFloor = GetMaximumFloor();
    for (int32 Index = 0; Index < Definition.Facilities.Num(); ++Index)
    {
        const int32 FloorIndex = MaxFloor > 0 ? Index % (MaxFloor + 1) : 0;
        const float Angle = Index * 2.399963f;
        const float Radius = 180.0f + Index * 35.0f;
        const FVector Location(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, FloorIndex * FloorHeight + 35.0f);
        AFacilityActor* Facility = GetWorld()->SpawnActor<AFacilityActor>(AFacilityActor::StaticClass(), Location, FRotator(0, FMath::RadiansToDegrees(Angle), 0));
        if (Facility)
        {
            Facility->Configure(Definition.Facilities[Index], FloorIndex);
            GeneratedActors.Add(Facility);
        }
    }
    if (MaxFloor >= 1)
    {
        AStairActor* Up = GetWorld()->SpawnActor<AStairActor>(AStairActor::StaticClass(), FVector(-220, 0, 45), FRotator::ZeroRotator); if (Up) { Up->Configure(0,1); GeneratedActors.Add(Up); }
        AStairActor* Down = GetWorld()->SpawnActor<AStairActor>(AStairActor::StaticClass(), FVector(-220, 0, FloorHeight + 45), FRotator(0,180,0)); if (Down) { Down->Configure(1,0); GeneratedActors.Add(Down); }
    }
    if (MaxFloor >= 2)
    {
        AStairActor* Up = GetWorld()->SpawnActor<AStairActor>(AStairActor::StaticClass(), FVector(220, 0, FloorHeight + 45), FRotator::ZeroRotator); if (Up) { Up->Configure(1,2); GeneratedActors.Add(Up); }
        AStairActor* Down = GetWorld()->SpawnActor<AStairActor>(AStairActor::StaticClass(), FVector(220, 0, FloorHeight * 2 + 45), FRotator(0,180,0)); if (Down) { Down->Configure(2,1); GeneratedActors.Add(Down); }
    }
}

void ARaftManager::DestroyGeneratedActors()
{
    for (AActor* Actor : GeneratedActors) if (IsValid(Actor)) Actor->Destroy();
    GeneratedActors.Reset();
}

void ARaftManager::SetViewedFloor(int32 FloorIndex)
{
    ViewedFloor = FMath::Clamp(FloorIndex, 0, GetMaximumFloor());
    const TArray<UInstancedStaticMeshComponent*> Floors{FloorOne, FloorTwo, FloorThree};
    const TArray<UInstancedStaticMeshComponent*> Rails{RailsOne, RailsTwo, RailsThree};
    for (int32 Index = 0; Index < 3; ++Index)
    {
        const bool bExists = Index <= GetMaximumFloor();
        const bool bHideAbove = Index > ViewedFloor;
        Floors[Index]->SetVisibility(bExists && !bHideAbove, true);
        Rails[Index]->SetVisibility(bExists && !bHideAbove, true);
        Floors[Index]->SetCollisionEnabled(Index == ViewedFloor ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        Rails[Index]->SetCollisionEnabled(Index == ViewedFloor ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    }
    for (AActor* Actor : GeneratedActors)
    {
        if (!Actor) continue;
        int32 ActorFloor = 0;
        if (AFacilityActor* Facility = Cast<AFacilityActor>(Actor)) ActorFloor = IInteractableInterface::Execute_GetInteractionFloor(Facility);
        else if (AStairActor* Stair = Cast<AStairActor>(Actor)) ActorFloor = Stair->GetFromFloor();
        Actor->SetActorHiddenInGame(ActorFloor > ViewedFloor);
        Actor->SetActorEnableCollision(ActorFloor == ViewedFloor);
    }
}
