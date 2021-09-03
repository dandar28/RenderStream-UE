// Fill out your copyright notice in the Description page of Project Settings.


#include "RenderStreamLightSubsystem.h"
#include "Engine/SpotLight.h"
#include "Components/LightComponent.h"
#include "Math/UnitConversion.h"
#include "RenderStream.h"


void URenderStreamLightSubsystem::Initialize(FSubsystemCollectionBase& CollectionBase)
{
    GetWorld()->OnWorldBeginPlay.AddUObject(this, &URenderStreamLightSubsystem::OnBeginPlay);
}

void URenderStreamLightSubsystem::OnBeginPlay()
{
    for (int i = 0; i < NumLights; i++)
    {
        ASpotLight* Light = GetWorld()->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), FVector(0, 0, 0), FRotator(0, 0, 0));
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetBrightness(5000);
        Light->GetLightComponent()->SetVisibility(false);
        Lights.Add(Light);
    }
}

void URenderStreamLightSubsystem::Deinitialize()
{
    

}

void URenderStreamLightSubsystem::UpdateLights(const std::vector<RenderStreamLink::RSLightData>& LightData)
{
    for (size_t i = 0; i < FMath::Min((int)LightData.size(), Lights.Num()); i++)
    {
        auto ld = LightData[i];
        auto Light = Lights[i];
        auto LightRootComponent = Light->GetRootComponent();

        float _pitch = ld.rx;
        float _yaw = ld.ry;
        float _roll = ld.rz;
        FQuat rotationQuat = FQuat::MakeFromEuler(FVector(_roll, _pitch, _yaw));
        Light->SetActorRotation(rotationQuat);

        FVector pos;
        pos.X = FUnitConversion::Convert(float(ld.z), EUnit::Meters, FRenderStreamModule::distanceUnit());
        pos.Y = FUnitConversion::Convert(float(ld.x), EUnit::Meters, FRenderStreamModule::distanceUnit());
        pos.Z = FUnitConversion::Convert(float(ld.y), EUnit::Meters, FRenderStreamModule::distanceUnit());
        Light->SetActorLocation(pos);


        Lights[i]->GetLightComponent()->SetVisibility(true);
        Lights[i]->SetBrightness(LightData[i].strength);
    }
    for (size_t i = LightData.size(); i < Lights.Num(); i++)
    {
        Lights[i]->GetLightComponent()->SetVisibility(false);
    }

}

bool URenderStreamLightSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
