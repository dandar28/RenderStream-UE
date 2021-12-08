// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TextureRenderTargetCube.h"

#include "FrameStream.h"
#include "RenderStream.h"
#include "RenderStreamLink.h"
#include "RenderStreamEnvMapTask.h"
#include "RenderRequest.h"

#include <functional>

#include "RenderStreamCubeCaptureComponent.generated.h"

UCLASS( ClassGroup=(RenderStream), meta=(BlueprintSpawnableComponent) )
class RENDERSTREAM_API URenderStreamCubeCaptureComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URenderStreamCubeCaptureComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderStream")
    int CaptureTimerMillis;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderStream")
    bool CaptureOnCameraMove;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderStream")
    bool SnapCubemapToCamera;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderStream")
    UTextureRenderTargetCube* RenderTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderStream")
    TSoftObjectPtr<AActor> CubemapActor;

    void Init(std::function<void(RenderStreamLink::EnvmapCaptureType envmapCaptureType, void* vData, int width, int height)> sendCallback);

    bool ThreadRunning() const { return m_envmapThreadRunning; }
    void SignalCameraMoved(const FVector& newLocation, const FRotator& newRotation);

    bool HasEnvmapRenderRequests() const { return !m_envmapRenderRequests.IsEmpty(); }
    bool PeekEnvmapRenderRequest(TSharedPtr<FRenderRequest>& outRef) const { return m_envmapRenderRequests.Peek(outRef); }
    void PopEnvmapRenderRequest() { m_envmapRenderRequests.Pop(); }

    void StartEnvmapCaptureThread();
    void StopEnvmapCaptureThread();

private:
    
    std::function<void(RenderStreamLink::EnvmapCaptureType envmapCaptureType, void* vData, int width, int height)> SendSurfaceData;

    unsigned int m_lastRenderGroup;
    unsigned int m_currentRenderGroup;
    TArray<FColor> m_faces;
    TArray<FRHITexture*> m_faceTexs;
    unsigned short m_lastFace;
    int m_width;
    int m_height;
    bool m_async;

    bool m_envmapThreadRunning = false;
    URenderStreamEnvMapTask m_envmapThread;

    TQueue<TSharedPtr<FRenderRequest>> m_envmapRenderRequests;
    unsigned int m_renderGroupCounter;

    void ReadPixels(TSharedPtr<FRenderRequest> RenderRequest, FTextureRenderTargetCubeResource* target, FReadSurfaceDataFlags InFlags = FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX), FIntRect InRect = FIntRect(0, 0, 0, 0));

    void EnvmapCaptureFunc();


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
