// Copyright Fast Travel Games AB 2023


#include "SnowOcclusionComponent.h"
#include "SnowOcclusionSubsystem.h"
#include "SceneSoftwareOcclusion.h"
#include "Kismet/KismetSystemLibrary.h"

/** Factor by which to grow occlusion tests **/
#define OCCLUSION_SLOP (1.0f)

USnowOcclusionComponent::USnowOcclusionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USnowOcclusionComponent::HandleOcclusionVisibility(bool bVisible)
{
	GetOwner()->SetActorHiddenInGame(!bVisible);
}

void USnowOcclusionComponent::ReplacePrimitiveComponent(UPrimitiveComponent* InPrimitiveComponent)
{
	if (InPrimitiveComponent == nullptr || InPrimitiveComponent == PrimitiveComponent)
	{
		return;
	}

	PrimitiveComponent = InPrimitiveComponent;
	
	if (Info != nullptr)
	{
		Info->MaxDrawDistance = PrimitiveComponent->CachedMaxDrawDistance > 0 ? PrimitiveComponent->CachedMaxDrawDistance : PrimitiveComponent->LDMaxDrawDistance;
	}

	UpdateInfo();
}

void USnowOcclusionComponent::BeginPlay()
{
	Super::BeginPlay();
    
    if (UKismetSystemLibrary::IsDedicatedServer(this))
    {
        return;
    }

	if (!bUseAsOccluder && !bCanBeOccludee)
	{
		return;
	}

	USnowOcclusionSubsystem* Occlusion = GEngine->GetEngineSubsystem<USnowOcclusionSubsystem>();
	checkf(Occlusion, TEXT("USnowOcclusionComponent used without a USnowOcclusionSubsystem present! Make sure the WorldFoundation plugin is enabled"));

	if (PrimitiveComponent == nullptr)
	{
		PrimitiveComponent = GetOwner()->FindComponentByClass<UPrimitiveComponent>();
	}
    
    if (PrimitiveComponent == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("%s has no primitive component. SnowOcclusion requires on! Will not be used as occluder or occludee"), *GetNameSafe(GetOwner()));
        return;
    }

	Info = NewObject<USnowPrimitiveInfo>();
	Info->PrimitiveComponentId = PrimitiveComponent->GetPrimitiveSceneId();
	Info->bOccluder = bUseAsOccluder && OccluderMesh != nullptr;
	Info->bOcludee = bCanBeOccludee;
	Info->MaxDrawDistance = PrimitiveComponent->CachedMaxDrawDistance > 0 ? PrimitiveComponent->CachedMaxDrawDistance : PrimitiveComponent->LDMaxDrawDistance;
	if (Info->bOccluder)
	{
		Info->OccluderData = FSnowMeshOccluderData::Build(OccluderMesh);
		if (Info->OccluderData.IsValid() == false)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed Building Occlusion Data for Actor: %s"), *GetNameSafe(GetOwner()));
			return;
		}
	}

	UpdateInfo();

	Occlusion->RegisterOccluder(this, Info);
}

void USnowOcclusionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	USnowOcclusionSubsystem* Occlusion = GEngine->GetEngineSubsystem<USnowOcclusionSubsystem>();
	Occlusion->UnregisterOccluder(this);
}

void USnowOcclusionComponent::UpdateInfo()
{
	FMatrix LocalToWorld = PrimitiveComponent->GetComponentTransform().ToMatrixWithScale();

	// Store occlusion bounds.
	FBoxSphereBounds OcclusionBounds = PrimitiveComponent->Bounds;
	if (bUseCustomBounds)
	{
		FVector HalfExtent = CustomBounds * 0.5f;
		FVector BoxPoint = HalfExtent - -HalfExtent;
		OcclusionBounds = FBoxSphereBounds(CustomBoundsOffset, BoxPoint, BoxPoint.Size()).TransformBy(LocalToWorld);
	}

	OcclusionBounds.BoxExtent.X = OcclusionBounds.BoxExtent.X + OCCLUSION_SLOP;
	OcclusionBounds.BoxExtent.Y = OcclusionBounds.BoxExtent.Y + OCCLUSION_SLOP;
	OcclusionBounds.BoxExtent.Z = OcclusionBounds.BoxExtent.Z + OCCLUSION_SLOP;
	OcclusionBounds.SphereRadius = OcclusionBounds.SphereRadius + OCCLUSION_SLOP;

	Info->LocalToWorld = LocalToWorld;
	Info->Bounds = OcclusionBounds;

	if (!Info->bOccluder)
	{
		return;
	}
	
	if (bBoundingBoxAsOcclusion)
	{
		Info->LocalToWorld.SetOrigin(Info->Bounds.Origin);
		
		// Calculate how much to scale the unit cube using un-rotated extents
		FTransform PrimitiveCompTransform = PrimitiveComponent->GetComponentTransform();
		PrimitiveCompTransform.SetRotation(FQuat::Identity);
		
		const FVector UnitCubeExtents = (FVector::OneVector * 100.0f) * 0.5f;

		// The bounds of the primitive when not rotated
		const FBoxSphereBounds PrimitiveIdentityBounds = PrimitiveComponent->CalcBounds(PrimitiveCompTransform);
		
		// What factor to scale the unit cube by to reach the primitive extents
		const FVector Scale = PrimitiveIdentityBounds.BoxExtent / (UnitCubeExtents * PrimitiveComponent->GetOwner()->GetActorScale3D());

		Info->LocalToWorld = FScaleMatrix::Make(UnitCubeScale * Scale) * Info->LocalToWorld;
	}
	else if (bOccluderIsScaledUnitCube)
	{
		Info->LocalToWorld.SetOrigin(Info->Bounds.Origin);
		Info->LocalToWorld = FScaleMatrix::Make(UnitCubeScale) * Info->LocalToWorld;
	}
}
