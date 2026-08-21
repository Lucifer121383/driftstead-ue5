#include "StairActor.h"
#include "DriftsteadCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AStairActor::AStairActor()
{
    PrimaryActorTick.bCanEverTick = false;
    InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
    InteractionBounds->InitBoxExtent(FVector(85, 120, 120));
    InteractionBounds->SetCollisionObjectType(ECC_WorldDynamic);
    InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBounds->SetCollisionResponseToAllChannels(ECR_Overlap);
    SetRootComponent(InteractionBounds);
    StairMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StairMesh"));
    StairMesh->SetupAttachment(RootComponent);
    StairMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) StairMesh->SetStaticMesh(Cube.Object);
    StairMesh->SetRelativeScale3D(FVector(1.2f, 2.2f, 0.25f));
    StairMesh->SetRelativeRotation(FRotator(0, 0, 20));
}

void AStairActor::Configure(int32 InFromFloor, int32 InToFloor)
{
    FromFloor = InFromFloor;
    ToFloor = InToFloor;
#if WITH_EDITOR
    SetActorLabel(FString::Printf(TEXT("Stair_Floor%d_to_%d"), FromFloor + 1, ToFloor + 1));
#endif
}

bool AStairActor::Interact_Implementation(ADriftsteadCharacter* Character)
{
    if (!Character || Character->GetCurrentFloor() != FromFloor) return false;
    Character->SetCurrentFloor(ToFloor);
    return true;
}

FText AStairActor::GetInteractionPrompt_Implementation() const
{
    return FText::Format(NSLOCTEXT("Driftstead", "StairPrompt", "E — 前往第 {0} 层"), FText::AsNumber(ToFloor + 1));
}
