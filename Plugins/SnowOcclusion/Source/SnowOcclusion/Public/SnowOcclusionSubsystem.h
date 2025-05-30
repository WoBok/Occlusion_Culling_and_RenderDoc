// Copyright Fast Travel Games AB 2023

#pragma once

#include "CoreMinimal.h"
#include "SceneSoftwareOcclusion.h"
#include "Subsystems/WorldSubsystem.h"

#include "SnowOcclusionSubsystem.generated.h"

class APlayerCameraManager;
class USnowOcclusionComponent;
class USnowPrimitiveInfo;

/**
 *
 */
UCLASS(Config = Game)
class SNOWOCCLUSION_API USnowOcclusionSubsystem : public UEngineSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual bool IsAllowedToTick() const override;

	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;

	void RegisterOccluder(USnowOcclusionComponent* Occluder, USnowPrimitiveInfo* Info);
	void UnregisterOccluder(USnowOcclusionComponent* Occluder);
	UFUNCTION(BlueprintCallable, Category = "Snow Occlusion")
	void DrawToCanvas(UCanvas* Canvas, int32 Width, int32 Height);

protected:
	UPROPERTY(Config)
	bool bEnabled = true;

	FSceneSoftwareOcclusion OcclusionSystem;
	TMap<USnowOcclusionComponent*, USnowPrimitiveInfo*> Occluders;
	TMap<uint32, USnowOcclusionComponent*> InfoToComp;
	TWeakObjectPtr<APlayerCameraManager> PlayerCameraManager;
};
