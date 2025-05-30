// Copyright Fast Travel Games AB 2023

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SnowOcclusionComponent.generated.h"

class USnowPrimitiveInfo;

UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SNOWOCCLUSION_API USnowOcclusionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USnowOcclusionComponent();

	void HandleOcclusionVisibility(bool bVisible);
	void ReplacePrimitiveComponent(UPrimitiveComponent* InPrimitiveComponent);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	friend class USnowOcclusionSubsystem;
	void UpdateInfo();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snow Occlusion|Occluder")
	bool bUseAsOccluder = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Occlusion|Occluder", Meta = (EditCondition = "bUseAsOccluder"))
	TObjectPtr<UStaticMesh> OccluderMesh;

	// Whether to recalculate bounds every tick
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Occlusion|Occluder", Meta = (EditCondition = "bUseAsOccluder"))
	bool bUpdateBounds = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snow Occlusion|Occluder", Meta = (EditCondition = "bUseAsOccluder && !bBoundingBoxAsOcclusion"))
	bool bOccluderIsScaledUnitCube = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snow Occlusion|Occluder", Meta = (EditCondition = "bUseAsOccluder && !bOccluderIsScaledUnitCube"))
	bool bBoundingBoxAsOcclusion = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snow Occlusion|Occluder", Meta = (EditCondition = "bUseAsOccluder"))
	FVector UnitCubeScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Occlusion|Occludee")
	bool bCanBeOccludee = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Occlusion|Occludee", Meta = (EditCondition = "bCanBeOccludee"))
	bool bUseCustomBounds = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Occlusion|Occludee", meta = (EditCondition = "bCanBeOccludee && bUseCustomBounds"))
	FVector CustomBounds = FVector::OneVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Occlusion|Occludee", meta = (EditCondition = "bCanBeOccludee && bUseCustomBounds"))
	FVector CustomBoundsOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Snow Occlusion")
	TObjectPtr<UPrimitiveComponent> PrimitiveComponent;


	UPROPERTY(Transient)
	USnowPrimitiveInfo* Info;
};
