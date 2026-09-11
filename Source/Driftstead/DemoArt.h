#pragma once
#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

// Fit the authored mesh to a desired longest dimension, with its bottom at Z.
namespace DriftsteadArt
{
inline void Play(const UObject* World, const TCHAR* Name, float Volume = .65f)
{
    if (auto* Sound=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Driftstead/Art/A_%s.A_%s"),Name,Name)))
        UGameplayStatics::PlaySound2D(World,Sound,Volume);
}
inline UStaticMesh* Load(const TCHAR* Name)
{
    return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Driftstead/Art/S_%s.S_%s"), Name, Name));
}
inline bool Fit(UStaticMeshComponent* Component, const TCHAR* Name, float Size, FVector Bottom = FVector::ZeroVector)
{
    UStaticMesh* Mesh = Load(Name);
    if (!Component || !Mesh) return false;
    Component->SetStaticMesh(Mesh);
    Component->EmptyOverrideMaterials();
    const FBoxSphereBounds Bounds = Mesh->GetBounds();
    const float Scale = Size / FMath::Max(1.0f, Bounds.BoxExtent.GetMax() * 2.0f);
    Component->SetRelativeScale3D(FVector(Scale));
    Component->SetRelativeLocation(Bottom - FVector(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z - Bounds.BoxExtent.Z) * Scale);
    return true;
}
}
