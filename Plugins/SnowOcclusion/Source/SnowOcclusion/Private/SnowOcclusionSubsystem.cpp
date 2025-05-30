// Copyright Fast Travel Games AB 2023


#include "SnowOcclusionSubsystem.h"
#include "SnowOcclusionComponent.h"

#include "IXRTrackingSystem.h"
#include "IHeadMountedDisplay.h"

#include "Engine/Canvas.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

static int32 GSOVisualizeBounds = 0;
static FAutoConsoleVariableRef CVarSOVisualizeBuffer(
	TEXT("ftg.so.VisualizeBounds"),
	GSOVisualizeBounds,
	TEXT("Visualize occlusion bounds"),
	ECVF_Cheat
);

static int32 GSOVisualizeResultsBounds = 0;
static FAutoConsoleVariableRef CVarSOVisualizeResults(
	TEXT("ftg.so.VisualizeResultsBounds"),
	GSOVisualizeResultsBounds,
	TEXT("Visualize occlusion results bounds"),
	ECVF_Cheat
);

static int32 GSOEnable = 1;
static FAutoConsoleVariableRef CVarSOEnable(
	TEXT("ftg.so.Enable"),
	GSOEnable,
	TEXT("Enable/Disable OC in runtime"),
	ECVF_Cheat
);

static int32 GSOOptimizationsEnable = 1;
static FAutoConsoleVariableRef CVarSOOptimizationsEnable(
	TEXT("ftg.so.OptimizationsEnable"),
	GSOOptimizationsEnable,
	TEXT("Enable/Disable OC optimizations in runtime"),
	ECVF_Cheat
);

static float GSOCullingDot = -0.4f;
static FAutoConsoleVariableRef CVarSOCullingDot(
	TEXT("ftg.so.CullingDot"),
	GSOCullingDot,
	TEXT("Defines the threshold for the similarity of the view vector and the direction to the occluder for it to be considered. Lower values are discarded."),
	ECVF_Default
);

//#pragma optimize("", off)
void USnowOcclusionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USnowOcclusionSubsystem::Deinitialize()
{
	Super::Deinitialize();

	OcclusionSystem.FlushResults();
}

bool USnowOcclusionSubsystem::IsAllowedToTick() const
{
#if WITH_EDITOR
	if (GEditor && GEditor->IsSimulatingInEditor())
	{
		return false;
	}
#endif
	return bEnabled && GSOEnable;
}

TStatId USnowOcclusionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USnowOcclusionSubsystem, STATGROUP_Tickables);
}

void DrawCameraFrustum(const APlayerCameraManager* CameraManager, const FColor& Color)
{
	if (!CameraManager)
		return;

	const FMinimalViewInfo MinimalView = CameraManager->GetCameraCacheView();

	float FrustumAngle = MinimalView.FOV;
	float FrustumStartDist = MinimalView.GetFinalPerspectiveNearClipPlane();
	float FrustumEndDist = 1000.0f;


	FVector Direction(1, 0, 0);
	FVector LeftVector(0, 1, 0);
	FVector UpVector(0, 0, 1);

	FVector Verts[8];

	// FOVAngle controls the horizontal angle.
	const float HozHalfAngleInRadians = FMath::DegreesToRadians(FrustumAngle * 0.5f);

	float HozLength = 0.0f;
	float VertLength = 0.0f;

	if (FrustumAngle > 0.0f)
	{
		HozLength = FrustumStartDist * FMath::Tan(HozHalfAngleInRadians);
		VertLength = HozLength / MinimalView.AspectRatio;
	}
	else
	{
		const float OrthoWidth = (FrustumAngle == 0.0f) ? 1000.0f : -FrustumAngle;
		HozLength = OrthoWidth * 0.5f;
		VertLength = HozLength / MinimalView.AspectRatio;
	}

	// near plane verts
	Verts[0] = (Direction * FrustumStartDist) + (UpVector * VertLength) + (LeftVector * HozLength);
	Verts[1] = (Direction * FrustumStartDist) + (UpVector * VertLength) - (LeftVector * HozLength);
	Verts[2] = (Direction * FrustumStartDist) - (UpVector * VertLength) - (LeftVector * HozLength);
	Verts[3] = (Direction * FrustumStartDist) - (UpVector * VertLength) + (LeftVector * HozLength);

	if (FrustumAngle > 0.0f)
	{
		HozLength = FrustumEndDist * FMath::Tan(HozHalfAngleInRadians);
		VertLength = HozLength / MinimalView.AspectRatio;
	}

	// far plane verts
	Verts[4] = (Direction * FrustumEndDist) + (UpVector * VertLength) + (LeftVector * HozLength);
	Verts[5] = (Direction * FrustumEndDist) + (UpVector * VertLength) - (LeftVector * HozLength);
	Verts[6] = (Direction * FrustumEndDist) - (UpVector * VertLength) - (LeftVector * HozLength);
	Verts[7] = (Direction * FrustumEndDist) - (UpVector * VertLength) + (LeftVector * HozLength);

	for (int32 X = 0; X < 8; ++X)
	{
		Verts[X] = CameraManager->ActorToWorld().TransformPosition(Verts[X]);
	}

	DrawDebugLine(CameraManager->GetWorld(), Verts[0], Verts[1], Color);
	DrawDebugLine(CameraManager->GetWorld(), Verts[1], Verts[2], Color);
	DrawDebugLine(CameraManager->GetWorld(), Verts[2], Verts[3], Color);
	DrawDebugLine(CameraManager->GetWorld(), Verts[3], Verts[0], Color);

	DrawDebugLine(CameraManager->GetWorld(), Verts[4], Verts[5], Color);
	DrawDebugLine(CameraManager->GetWorld(), Verts[5], Verts[6], Color);
	DrawDebugLine(CameraManager->GetWorld(), Verts[6], Verts[7], Color);
	DrawDebugLine(CameraManager->GetWorld(), Verts[7], Verts[4], Color);

	DrawDebugLine(CameraManager->GetWorld(), Verts[0], Verts[4], Color);
	DrawDebugLine(CameraManager->GetWorld(), Verts[1], Verts[5], Color);
	DrawDebugLine(CameraManager->GetWorld(), Verts[2], Verts[6], Color);
	DrawDebugLine(CameraManager->GetWorld(), Verts[3], Verts[7], Color);
}


void USnowOcclusionSubsystem::Tick(float DeltaTime)
{
	if (PlayerCameraManager == nullptr && Occluders.Num() > 0)
	{
		USnowOcclusionComponent* Comp = Occluders.begin()->Key;
		PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(Comp->GetWorld(), 0);
	}

	if (!PlayerCameraManager.IsValid())
	{
		return;
	}

	if (!GEngine)
	{
		return;
	}

	FSnowViewInfo View;

	const FMinimalViewInfo MinimalView = PlayerCameraManager->GetCameraCacheView();
	FMatrix VP;
	FMatrix P;
	UGameplayStatics::GetViewProjectionMatrix(MinimalView, View.ViewMatrix, P, VP);
	View.ViewOrigin = MinimalView.Location;

	bool bStereoRendering = GEngine->StereoRenderingDevice.IsValid() && 
		GEngine->StereoRenderingDevice->IsStereoEnabled() && 
		GEngine->XRSystem.IsValid() && 
		GEngine->XRSystem->GetHMDDevice();

	// When using stereo we use a "center eye"
	View.ProjectionMatrix = bStereoRendering ? GEngine->StereoRenderingDevice->GetStereoProjectionMatrix(EStereoscopicEye::eSSE_MONOSCOPIC) : P;

#if WITH_EDITOR
	if (GEditor && !GEditor->IsVRPreviewActive())
	{
		View.ProjectionMatrix = P;
	}
#endif

	if (GSOVisualizeBounds && !GEngine->StereoRenderingDevice.IsValid())
	{
		DrawCameraFrustum(PlayerCameraManager.Get(), FColor::Cyan);
	}

	TArray<USnowPrimitiveInfo*> Scene;
	for (auto Pair : Occluders)
	{
		if (Pair.Key->bUpdateBounds)
		{
			Pair.Key->UpdateInfo();
		}

		if (GSOOptimizationsEnable)
		{
			// Simple "frustrum" culling
			FVector CameraForward = PlayerCameraManager->GetCameraRotation().Vector();
			FVector DirToOccluder = (Pair.Value->Bounds.Origin - PlayerCameraManager->GetCameraLocation()).GetSafeNormal();

			// Skip objects where the bounds center is beyond the draw distance
			if (Pair.Value->MaxDrawDistance > 0.0f &&
				FVector::Distance(PlayerCameraManager->GetCameraLocation(), Pair.Value->Bounds.Origin) > Pair.Value->MaxDrawDistance)
			{
				Pair.Key->HandleOcclusionVisibility(true);
				continue;
			}

			// Skip objects behind the player
			if (CameraForward.Dot(DirToOccluder) < GSOCullingDot)
			{
				const bool bInsideOccluder = UKismetMathLibrary::IsPointInBox(PlayerCameraManager->GetCameraLocation(), Pair.Value->Bounds.Origin, Pair.Value->Bounds.BoxExtent);
				if (!bInsideOccluder)
				{
					Pair.Key->HandleOcclusionVisibility(true);
					continue;
				}
			}
		}

		Scene.Add(Pair.Value);

		if (GSOVisualizeBounds)
		{
			FColor BoundsColor = FColor::Red; //Ocluder & Ocludee
			if (Pair.Value->bOccluder && !Pair.Value->bOcludee) // Only Occluder
			{
				BoundsColor = FColor::Green;
			}
			if (!Pair.Value->bOccluder && Pair.Value->bOcludee) // Only Occludee
			{
				BoundsColor = FColor::Yellow;
			}

			auto Bounds = Pair.Value->Bounds;
			bool bNeedForeground = false;
			if (GSOVisualizeBounds == 2 && IsValid(Pair.Key->OccluderMesh) && Pair.Value->bOccluder)
			{
				Bounds = Pair.Key->OccluderMesh.Get()->GetBounds().TransformBy(Pair.Value->LocalToWorld);
				bNeedForeground = Bounds.SphereRadius < Pair.Value->Bounds.SphereRadius;
			}

			DrawDebugBox(PlayerCameraManager->GetWorld(), Bounds.Origin, Bounds.BoxExtent, FQuat::Identity, BoundsColor, false, 0, bNeedForeground ? SDPG_Foreground : SDPG_World);
		}
	}

	OcclusionSystem.Process(Scene, View);

	//ParallelFor(Scene.Num(), [this, &Scene](int32 Index)
	//{
	//	USnowPrimitiveInfo* Info = Scene[Index];

	//	if (!InfoToComp.Contains(Info->PrimitiveComponentId.PrimIDValue))
	//	{
	//		return;
	//	}

	//	USnowOcclusionComponent* Comp = InfoToComp[Info->PrimitiveComponentId.PrimIDValue];
	//	Comp->HandleOcclusionVisibility(Info->bVisible);
	//});

	for (USnowPrimitiveInfo* Info : Scene)
	{
		if (!InfoToComp.Contains(Info->PrimitiveComponentId.PrimIDValue))
		{
			continue;
		}

		USnowOcclusionComponent* Comp = InfoToComp[Info->PrimitiveComponentId.PrimIDValue];
		Comp->HandleOcclusionVisibility(Info->bVisible);

		if (GSOVisualizeResultsBounds)
		{
			FColor Color = Info->bVisible ? FColor::Magenta : FColor::Cyan;

			auto Bounds = Info->Bounds;
			if (IsValid(Comp->OccluderMesh) && Info->bOccluder)
			{
				Bounds = Comp->OccluderMesh.Get()->GetBounds().TransformBy(Info->LocalToWorld);
			}
			DrawDebugBox(PlayerCameraManager->GetWorld(), Bounds.Origin, Bounds.BoxExtent * 0.99, FQuat::Identity, Color, false, 0, SDPG_Foreground);
		}
	}
}

void USnowOcclusionSubsystem::RegisterOccluder(USnowOcclusionComponent* Occluder, USnowPrimitiveInfo* Info)
{
	Occluders.Add(Occluder, Info);
	InfoToComp.Add(Info->PrimitiveComponentId.PrimIDValue, Occluder);
}

void USnowOcclusionSubsystem::UnregisterOccluder(USnowOcclusionComponent* Occluder)
{
	if (Occluders.Contains(Occluder))
	{
		USnowPrimitiveInfo* Info = Occluders[Occluder];
		Occluders.Remove(Occluder);
		InfoToComp.Remove(Info->PrimitiveComponentId.PrimIDValue);
	}
}

void USnowOcclusionSubsystem::DrawToCanvas(UCanvas* Canvas, int32 Width, int32 Height)
{
	OcclusionSystem.DebugDrawToCanvas(Canvas->Canvas, 20, 20);
}

//#pragma optimize("", on)
