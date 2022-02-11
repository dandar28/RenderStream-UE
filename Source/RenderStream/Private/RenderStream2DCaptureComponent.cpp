// Fill out your copyright notice in the Description page of Project Settings.

#include "RenderStream2DCaptureComponent.h"
#include "Engine/Public/HardwareInfo.h"
#include "ImageUtils.h"

// Sets default values for this component's properties
URenderStream2DCaptureComponent::URenderStream2DCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void URenderStream2DCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

    AActor* parent = GetOwner();
    
    if (parent)
    {
        UActorComponent* foundActor = parent->GetComponentByClass(USceneCaptureComponent2D::StaticClass());
        if (foundActor)
        {
            m_captureComponent = CastChecked<USceneCaptureComponent2D>(foundActor);
            UpdateRenderTarget(CaptureWidth, CaptureHeight);
        }
    }

    if (ChannelName.IsEmpty())
    {
        ChannelName = GetName();
    }
}

bool URenderStream2DCaptureComponent::UpdateRenderTarget(int rs_desiredWidth, int rs_desiredHeight)
{
    if (m_captureComponent && (rs_desiredWidth != CaptureWidth || rs_desiredHeight != CaptureHeight || m_captureComponent->TextureTarget == nullptr))
    {
        m_renderTargetUpdating = true;
        //resize the capture texture
        if (m_captureComponent->TextureTarget == nullptr)
        {
            m_captureComponent->TextureTarget = NewObject<UTextureRenderTarget2D>(this, UTextureRenderTarget2D::StaticClass(), FName(ChannelName));

            check(m_captureComponent->TextureTarget);

            m_captureComponent->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
            m_captureComponent->TextureTarget->Filter = TextureFilter::TF_Bilinear;
            m_captureComponent->TextureTarget->SizeX = rs_desiredWidth;
            m_captureComponent->TextureTarget->SizeY = rs_desiredHeight;
            m_captureComponent->TextureTarget->ClearColor = FLinearColor::Green;
            m_captureComponent->TextureTarget->bNeedsTwoCopies = false;
            m_captureComponent->TextureTarget->AddressX = TextureAddress::TA_Clamp;
            m_captureComponent->TextureTarget->AddressY = TextureAddress::TA_Clamp;
            m_captureComponent->TextureTarget->bAutoGenerateMips = false;   //TODO: play with this... maybe not needed
            
            m_captureComponent->TextureTarget->UpdateResource();
        }
        else
        {
            m_captureComponent->TextureTarget->ResizeTarget(rs_desiredWidth, rs_desiredHeight);
        }

        //always set the unreal things to be as the incoming RS ones
        CaptureWidth = rs_desiredWidth;
        CaptureHeight = rs_desiredHeight;

        m_renderTargetUpdating = false;
        m_renderTargetUpdated = true;

        return true;
    }

    return false;
}

void URenderStream2DCaptureComponent::ReadPixels(TSharedPtr<FRenderRequest> RenderRequest, FTextureRenderTargetResource* Target, FReadSurfaceDataFlags Flags, FIntRect Rect)
{
    uint32 targetWidth = Target->GetSizeX();
    uint32 targetHeight = Target->GetSizeY();
    
    if (Rect == FIntRect(0, 0, 0, 0))
    {
        Rect = FIntRect(0, 0, targetWidth, targetHeight);
    }

    RenderRequest->ImageData.Reset();
    RenderRequest->RenderGroup = 0;
    RenderRequest->FaceNumber = 0; //todo? not relevant for simple surfaces
    RenderRequest->ImageWidth = targetWidth;
    RenderRequest->ImageHeight = targetHeight;

    FString versionToggle = FHardwareInfo::GetHardwareInfo(NAME_RHI);
    int bpp = 4;
    ENQUEUE_RENDER_COMMAND(ReadSurfaceCommand)(
        [Target, Rect, RenderRequest, Flags, versionToggle, bpp](FRHICommandListImmediate& RHICmdList)
        {
            RHICmdList.ReadSurfaceData(Target->TextureRHI, Rect, RenderRequest->ImageData, Flags);
        }
    );

    RenderRequest->RenderFence.BeginFence();
}

// Called every frame
void URenderStream2DCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	////somehow get these back from renderstream
 //   uint32 rs_desiredWidth = 256;
 //   uint32 rs_desiredHeight = 256;

 //   //update the render target if it needs it
 //   bool updated = UpdateRenderTarget(rs_desiredWidth, rs_desiredHeight);

    //then read the surface and send it via renderstream
    //m_captureComponent->TextureTarget->
    FDateTime ms = FDateTime::Now();
    FTimespan diff = ms - m_startTime;
    if ((!m_renderTargetUpdating && m_renderTargetUpdated) || (CaptureTimerMillis != 0 && diff.GetTotalMilliseconds() >= CaptureTimerMillis))
    {
        m_renderTargetUpdated = false;
     
        TSharedPtr<FRenderRequest> renderRequest(new FRenderRequest);
        ReadPixels(renderRequest, m_captureComponent->TextureTarget->GameThread_GetRenderTargetResource(), FReadSurfaceDataFlags(RCM_UNorm));
        m_envmapRenderRequests.Enqueue(renderRequest);

        m_startTime = FDateTime::Now();
    }

    if (!m_envmapRenderRequests.IsEmpty())
    {
        TSharedPtr<FRenderRequest> nextRequest = nullptr;
        bool peeked = m_envmapRenderRequests.Peek(nextRequest);
        if (peeked && nextRequest && nextRequest->RenderFence.IsFenceComplete())
        {
            void* vData = (void*)nextRequest->ImageData.GetData();
            RenderStreamLink::CameraResponseData frameInfo = m_frameResponseData;
            frameInfo.envmapCaptureType = (RenderStreamLink::EnvmapCaptureType)EnhancedFrameType.GetValue();
            frameInfo.camera.environmentFace = frameInfo.envmapCaptureType - 1;
            frameInfo.camera.environmentFaceResolution = CaptureWidth;

            bool saveImg = false;

            if (saveImg)
            {
                FString filename = FString::Printf(TEXT("C:/temp/envmap_%d.bmp"), frameInfo.enhancedCaptureType);
                TArray<uint8> compressedImage;
                FImageUtils::CompressImageArray(CaptureWidth, CaptureHeight, nextRequest->ImageData, compressedImage);
                FFileHelper::SaveArrayToFile(compressedImage, *filename);
            }
            
            if (m_stream)
            {
//                m_stream->SendFrameNDI_RenderingThreead(frameInfo, vData, CaptureWidth, CaptureHeight);
            }

            m_envmapRenderRequests.Pop();
        }
    }
}

