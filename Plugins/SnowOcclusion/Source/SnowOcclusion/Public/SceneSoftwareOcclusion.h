// Copyright Epic Games, Inc. All Rights Reserved.
// Modified by Fast Travel Games

#pragma once

#include "CoreMinimal.h"
#include "Async/TaskGraphInterfaces.h"
#include "SceneSoftwareOcclusion.generated.h"

class USnowOcclusionComponent;

class FRHICommandListImmediate;
class FScene;
class FViewInfo;
struct FOcclusionFrameResults;

typedef TArray<FVector> FOccluderVertexArray;
typedef TArray<uint16> FOccluderIndexArray;
typedef TSharedPtr<FOccluderVertexArray, ESPMode::ThreadSafe> FOccluderVertexArraySP;
typedef TSharedPtr<FOccluderIndexArray, ESPMode::ThreadSafe> FOccluderIndexArraySP;

class FSnowMeshOccluderData
{
public:
	FSnowMeshOccluderData();

	FOccluderVertexArraySP VerticesSP;
	FOccluderIndexArraySP IndicesSP;

	static TUniquePtr<FSnowMeshOccluderData> Build(UStaticMesh* Owner);
};

UCLASS()
class USnowPrimitiveInfo : public UObject
{
	GENERATED_BODY()
public:
	USnowPrimitiveInfo();

	FPrimitiveComponentId PrimitiveComponentId;
	TUniquePtr<FSnowMeshOccluderData> OccluderData;
	FBoxSphereBounds Bounds;
	FMatrix LocalToWorld;
	bool bOccluder;
	bool bOcludee;
	bool bVisible;
	float MaxDrawDistance;
};

class FSnowViewInfo
{
public:
	FMatrix ViewMatrix;
	FMatrix ProjectionMatrix;
	FVector ViewOrigin;
};

class FSceneSoftwareOcclusion
{
public:
	FSceneSoftwareOcclusion();
	~FSceneSoftwareOcclusion();

	int32 Process(const TArray<USnowPrimitiveInfo*> Scene, FSnowViewInfo& View);
	void FlushResults();

	void DebugDrawToCanvas(FCanvas* Canvas, int32 InX, int32 InY);

private:
	FGraphEventRef TaskRef;
	TUniquePtr<FOcclusionFrameResults> Available;
	TUniquePtr<FOcclusionFrameResults> Processing;
};
