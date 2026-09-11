#include "DriftItemActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "DriftsteadTypes.h"
#include "DemoArt.h"

ADriftItemActor::ADriftItemActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(55.0f);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetCollisionResponseToAllChannels(ECR_Overlap);
    SetRootComponent(Collision);

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetupAttachment(Collision);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) VisualMesh->SetStaticMesh(Cube.Object);

    RareLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RareLight"));
    RareLight->SetupAttachment(Collision);
    RareLight->SetIntensity(0.0f);
    RareLight->SetAttenuationRadius(240.0f);
}

void ADriftItemActor::BeginPlay()
{
    Super::BeginPlay();
    BaseHeight = GetActorLocation().Z;
    BobPhase = FMath::FRandRange(0.0f, PI * 2.0f);
    ApplyDefinitionVisuals();
}

void ADriftItemActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bCaught) return;

    FVector Location = GetActorLocation() + DriftVelocity * DeltaSeconds;
    Location.Z = BaseHeight + FMath::Sin(GetWorld()->GetTimeSeconds() * 1.7f + BobPhase) * 10.0f;
    SetActorLocation(Location);
    AddActorLocalRotation(FRotator(0.0f, 12.0f * DeltaSeconds, 3.0f * DeltaSeconds));
}

void ADriftItemActor::ConfigureItem(FName NewItemId, FVector NewDriftVelocity)
{
    ItemId = NewItemId;
    DriftVelocity = NewDriftVelocity;
    if (HasActorBegunPlay())
    {
        BaseHeight = GetActorLocation().Z;
        ApplyDefinitionVisuals();
    }
}

void ADriftItemActor::ApplyDefinitionVisuals()
{
    const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(ItemId);
    if (!Definition) return;

    // Keep the visible mesh size data-driven, but make the gameplay target more
    // forgiving than its low-poly silhouette.
    Collision->SetSphereRadius(FMath::Max(Definition->WorldCollisionSize.GetMax() * 0.85f, 75.0f));
    const TCHAR* Art = TEXT("crate");
    if (ItemId == TEXT("Driftwood")) Art = TEXT("platform_planks");
    else if (ItemId == TEXT("Rope")) Art = TEXT("RopeCoil");
    else if (ItemId == TEXT("ScrapMetal")) Art = TEXT("tool_shovel");
    else if (ItemId == TEXT("Cloth")) Art = TEXT("flag");
    else if (ItemId == TEXT("SealedBarrel")) Art = TEXT("barrel");
    else if (ItemId == TEXT("Electronics")) Art = TEXT("chest");
    else if (ItemId == TEXT("FoodCrate")) Art = TEXT("crate_bottles");
    if (DriftsteadArt::Fit(VisualMesh, Art, FMath::Max(65.0f,Definition->WorldCollisionSize.GetMax()), FVector(0,0,-18)))
    {
        RareLight->SetLightColor(Definition->SecondaryColor);
        RareLight->SetIntensity(Definition->Rarity >= EDriftItemRarity::Rare ? 220.0f : 0.0f);
        return;
    }
    VisualMesh->SetWorldScale3D(Definition->WorldCollisionSize / 100.0f);
    const TCHAR* MaterialPath = TEXT("/Game/Driftstead/Materials/M_WoodLight.M_WoodLight");
    if (ItemId == TEXT("Rope")) MaterialPath = TEXT("/Game/Driftstead/Materials/M_Rope.M_Rope");
    else if (ItemId == TEXT("Cloth")) MaterialPath = TEXT("/Game/Driftstead/Materials/M_Cloth.M_Cloth");
    else if (ItemId == TEXT("ScrapMetal") || ItemId == TEXT("MachineryCrate") || ItemId == TEXT("Electronics")) MaterialPath = TEXT("/Game/Driftstead/Materials/M_Metal.M_Metal");
    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, MaterialPath);
    if (BaseMaterial)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        Material->SetVectorParameterValue(TEXT("BaseColor"), Definition->PrimaryColor);
        VisualMesh->SetMaterial(0, Material);
    }
    const bool bRare = Definition->Rarity == EDriftItemRarity::Rare || Definition->Rarity == EDriftItemRarity::Epic;
    RareLight->SetLightColor(Definition->SecondaryColor);
    RareLight->SetIntensity(bRare ? 1800.0f : 0.0f);
}

bool ADriftItemActor::CanBeCaught_Implementation(float HookCapacity) const
{
    const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(ItemId);
    return !bCaught && Definition && Definition->Weight <= HookCapacity;
}

float ADriftItemActor::GetCatchWeight_Implementation() const
{
    const FDriftItemDefinition* Definition = FDriftsteadItemCatalog::Find(ItemId);
    return Definition ? static_cast<float>(Definition->Weight) : TNumericLimits<float>::Max();
}

void ADriftItemActor::OnCaught_Implementation(AActor* HookActor)
{
    if (!HookActor || bCaught) return;
    bCaught = true;
    DriftVelocity = FVector::ZeroVector;
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AttachToActor(HookActor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    SetActorRelativeLocation(FVector(0.0f, 0.0f, -22.0f));
    SetActorRelativeRotation(FRotator::ZeroRotator);
}

void ADriftItemActor::PrepareForRecovery()
{
    bCaught = true;
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetActorTickEnabled(false);
}

void ADriftItemActor::ReleaseFromHook()
{
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    bCaught = false;
    BaseHeight = 35.0f;
    DriftVelocity = FVector(8,-22,0);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SetActorTickEnabled(true);
}
