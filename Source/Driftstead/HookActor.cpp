#include "HookActor.h"
#include "HookComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AHookActor::AHookActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("HookCollision"));
    Collision->InitSphereRadius(56.0f);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    // Catch selection is centralized in UHookComponent's planar sweep so a
    // frame with several overlaps still resolves exactly one closest target.
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    SetRootComponent(Collision);

    HookMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HookMesh"));
    HookMesh->SetupAttachment(Collision);
    HookMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded()) HookMesh->SetStaticMesh(Sphere.Object);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MetalMaterial(TEXT("/Game/Driftstead/Materials/M_Metal.M_Metal"));
    if (MetalMaterial.Succeeded()) HookMesh->SetMaterial(0, MetalMaterial.Object);
    HookMesh->SetRelativeScale3D(FVector(0.35f));

    RopeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RopeMesh"));
    RopeMesh->SetupAttachment(GetRootComponent());
    RopeMesh->SetUsingAbsoluteLocation(true);
    RopeMesh->SetUsingAbsoluteRotation(true);
    RopeMesh->SetUsingAbsoluteScale(true);
    RopeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded()) RopeMesh->SetStaticMesh(Cylinder.Object);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> RopeMaterial(TEXT("/Game/Driftstead/Materials/M_Rope.M_Rope"));
    if (RopeMaterial.Succeeded()) RopeMesh->SetMaterial(0, RopeMaterial.Object);
}

void AHookActor::InitializeHook(UHookComponent* InOwnerComponent)
{
    OwnerComponent = InOwnerComponent;
}

void AHookActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!OwnerComponent.IsValid() || !OwnerComponent->GetOwner()) return;

    const FVector Start = OwnerComponent->GetRopeOrigin();
    const FVector End = GetActorLocation();
    const FVector Delta = End - Start;
    const float Length = Delta.Size();
    RopeMesh->SetWorldLocation((Start + End) * 0.5f);
    RopeMesh->SetWorldRotation(FQuat::FindBetweenNormals(FVector::UpVector, Delta.GetSafeNormal()).Rotator());
    RopeMesh->SetWorldScale3D(FVector(0.035f, 0.035f, FMath::Max(Length / 100.0f, 0.01f)));
}
