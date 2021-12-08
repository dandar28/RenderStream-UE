// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "FrameStream.h"
#include "RenderRequest.h"
#include "RenderStream2DCaptureComponent.generated.h"

UENUM()
enum EFaceIndexFrameType
{
    E_NONE = RenderStreamLink::EnvmapCaptureType::NONE,
    E_UP = RenderStreamLink::EnvmapCaptureType::ENV_UP,
    E_DOWN,
    E_LEFT,
    E_RIGHT,
    E_FRONT,
    E_BACK
};

UCLASS( ClassGroup=(RenderStream), meta=(BlueprintSpawnableComponent) )
class RENDERSTREAM_API URenderStream2DCaptureComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URenderStream2DCaptureComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

    USceneCaptureComponent2D* m_captureComponent;
    FDateTime m_startTime;
    TQueue<TSharedPtr<FRenderRequest>> m_envmapRenderRequests;
    TSharedPtr<FFrameStream> m_stream;
    RenderStreamLink::CameraResponseData m_frameResponseData;

    bool m_renderTargetUpdating = false;
    bool m_renderTargetUpdated = false;

public:

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void SetStream(TSharedPtr<FFrameStream> stream) { m_stream = stream; }
    void SetFrameData(RenderStreamLink::CameraResponseData responseData) { m_frameResponseData = responseData; }

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
        FString ChannelName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
        int CaptureWidth;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
        int CaptureHeight;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
        int CaptureTimerMillis;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
        TEnumAsByte<EFaceIndexFrameType> EnhancedFrameType;

    bool UpdateRenderTarget(int DesiredWidth, int DesiredHeight);

    void ReadPixels(TSharedPtr<FRenderRequest> RenderRequest, FTextureRenderTargetResource* Target, FReadSurfaceDataFlags Flags, FIntRect Rect = FIntRect(0,0,0,0));
};
