// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "RenderStreamLink.h"
#include "RenderStreamLightSubsystem.generated.h"

/**
 * 
 */

class ALight;

UCLASS()
class RENDERSTREAM_API URenderStreamLightSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    void Initialize(FSubsystemCollectionBase& CollectionBase) override;

    void Deinitialize() override;

    void OnBeginPlay();

    void UpdateLights(const std::vector<RenderStreamLink::RSLightData>& LightData);

    bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
    const size_t NumLights = 16;
    TArray<ALight*> Lights;
};
