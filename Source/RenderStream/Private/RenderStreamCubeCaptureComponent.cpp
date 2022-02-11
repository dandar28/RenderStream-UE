// Fill out your copyright notice in the Description page of Project Settings.

#include "RenderStreamCubeCaptureComponent.h"
#include "Engine/Public/HardwareInfo.h"

URenderStreamCubeCaptureComponent::URenderStreamCubeCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

    //Toggle this to run it in it's own thread - not recommended
    m_async = false;
}

void URenderStreamCubeCaptureComponent::Init(std::function<void(RenderStreamLink::EnvmapCaptureType envmapCaptureType, void* vData, int width, int height)> sendCallback)
{
    SendSurfaceData = sendCallback;
    StartEnvmapCaptureThread();
}

void URenderStreamCubeCaptureComponent::BeginPlay()
{
	Super::BeginPlay();
}

void URenderStreamCubeCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    bool doNothing = false;
    if (doNothing == false)
    {
        if (!m_async)
            m_envmapThread.RunTick();

        if (HasEnvmapRenderRequests())
        {
            TSharedPtr<FRenderRequest> nextRequest = nullptr;
            bool peeked = PeekEnvmapRenderRequest(nextRequest);

            if (peeked && nextRequest && nextRequest->RenderFence.IsFenceComplete())
            {
                if (m_currentRenderGroup != nextRequest->RenderGroup)
                {
                    //init faces array with transparent colour - this doesn't reallocate memory unless the size changes
                    m_faces.Init(FColor(0, 0, 0, 0), nextRequest->ImageData.Num() * 6);
                    m_faceTexs.Empty();
                }

                m_currentRenderGroup = nextRequest->RenderGroup;
                m_lastFace = nextRequest->FaceNumber;
                m_width = nextRequest->ImageWidth;
                m_height = nextRequest->ImageHeight * 6;

                /*FString str = FString::Printf(TEXT("Adding face %d (render group %d)"), m_lastFace, m_currentRenderGroup);
UE_LOG(LogTemp, Warning, TEXT("CubeComponent: %s"), *str);*/

                m_faces.Insert(nextRequest->ImageData, nextRequest->ImageData.Num() * (int32)m_lastFace);
                m_faceTexs.Add(nextRequest->RenderCommandContext.OutTex);

                PopEnvmapRenderRequest();
            }
        }

        if (m_lastFace == (unsigned short)ECubeFace::CubeFace_MAX - 1)
        {
            m_lastFace = 0;
            m_lastRenderGroup = m_currentRenderGroup;

            //this is the old way of doing it
            //void* vData = (void*)m_faces.GetData();

            //this is the latest RS way of doing it
            for (int i = 0; i < m_faceTexs.Num(); i++)
            {
                FString str = FString::Printf(TEXT("output tex faces %p (%d items)"), (void*)&m_faceTexs, m_faceTexs.Num());
                UE_LOG(LogTemp, Warning, TEXT("CubeComponent: %s"), *str);

                //TODO: this could easily end up being wrong
                RenderStreamLink::EnvmapCaptureType face = (RenderStreamLink::EnvmapCaptureType)(i + 1);
                str = FString::Printf(TEXT("face tex %d: %p (is %s)"), i, (void*)&m_faceTexs[i], m_faceTexs[i]->IsValid() ? TEXT("valid") : TEXT("NOT VALID!"));
                UE_LOG(LogTemp, Warning, TEXT("CubeComponent: %s"), *str);

                if(m_faceTexs[i] != nullptr && m_faceTexs[i]->IsValid())
                {
                    void* vData = static_cast<void*>(m_faceTexs[i]->GetNativeResource());

                    if (vData)
                    {
                        if (SendSurfaceData) SendSurfaceData(face, vData, 0, 0);
                        //send all at once
                        //if (SendSurfaceData) SendSurfaceData(RenderStreamLink::EnvmapCaptureType::ENV_ALL, vData, m_width, m_height);
                        //old way of doing this
                        //if (m_stream) m_stream->SendFrameNDI_RenderingThreead(frameInfo, vData, m_width, m_height);
                    }
                }
            }
        }
    }
}

void URenderStreamCubeCaptureComponent::SignalCameraMoved(const FVector& newLocation, const FRotator& newRotation)
{
    CubemapActor->SetActorLocationAndRotation(newLocation, newRotation);

    m_envmapThread.SignalCameraMoved();
}

void URenderStreamCubeCaptureComponent::ReadPixels(TSharedPtr<FRenderRequest> RenderRequest, FTextureRenderTargetCubeResource* target, FReadSurfaceDataFlags InFlags, FIntRect InRect)
{
    if (InRect == FIntRect(0, 0, 0, 0))
    {
        InRect = FIntRect(0, 0, target->GetSizeXY().X, target->GetSizeXY().Y);
    }

    RenderRequest->ImageData.Reset();
    RenderRequest->RenderGroup = m_renderGroupCounter;
    RenderRequest->FaceNumber = (unsigned short)InFlags.GetCubeFace();
    RenderRequest->ImageWidth = target->GetSizeX();
    RenderRequest->ImageHeight = target->GetSizeY();

    RenderRequest->RenderCommandContext.SrcRenderTarget = target;
    RenderRequest->RenderCommandContext.OutData = &(RenderRequest->ImageData);
    RenderRequest->RenderCommandContext.Rect = InRect;
    RenderRequest->RenderCommandContext.Flags = InFlags;

    FString versionToggle = FHardwareInfo::GetHardwareInfo(NAME_RHI);
    int bpp = 4; //TODO: This actually needs to be read in, not guessed

    ENQUEUE_RENDER_COMMAND(ReadSurfaceCommand)(
        [RenderRequest, versionToggle, bpp](FRHICommandListImmediate& RHICmdList)
        {
            //if this messes up in dx11, cut/paste it back into the dx12 block
            FRHIResourceCreateInfo texInfo;
            RenderRequest->RenderCommandContext.OutTex = RHICmdList.CreateTexture2D(
                RenderRequest->ImageWidth,
                RenderRequest->ImageHeight,
                RenderRequest->RenderCommandContext.SrcRenderTarget->GetTextureRHI()->GetFormat(),
                RenderRequest->RenderCommandContext.SrcRenderTarget->GetTextureRHI()->GetNumMips(),
                RenderRequest->RenderCommandContext.SrcRenderTarget->GetTextureRHI()->GetNumSamples(),
                ETextureCreateFlags::TexCreate_CPUWritable | TexCreate_CPUReadback,
                texInfo
            );

            //FTexture2DRHIRef texRef = RHICmdList.CreateTexture2D(
            //    RenderRequest->ImageWidth,
            //    RenderRequest->ImageHeight,
            //    RenderRequest->RenderCommandContext.SrcRenderTarget->GetTextureRHI()->GetFormat(),
            //    RenderRequest->RenderCommandContext.SrcRenderTarget->GetTextureRHI()->GetNumMips(),
            //    RenderRequest->RenderCommandContext.SrcRenderTarget->GetTextureRHI()->GetNumSamples(),
            //    ETextureCreateFlags::TexCreate_CPUWritable | TexCreate_CPUReadback,
            //    texInfo
            //);

            if (RHICmdList.IsOutsideRenderPass())
            {
                FRHICopyTextureInfo copyInfo;
                copyInfo.Size.X = RenderRequest->ImageWidth;
                copyInfo.Size.Y = RenderRequest->ImageHeight;
                copyInfo.SourceSliceIndex = RenderRequest->FaceNumber;

                RHICmdList.CopyTexture(
                    RenderRequest->RenderCommandContext.SrcRenderTarget->GetTextureRHI(),
                    RenderRequest->RenderCommandContext.OutTex, 
                    copyInfo);
            }
            else
            {
                FResolveParams params;
                params.Rect = FResolveRect(FIntRect(FIntPoint(0, 0), FIntPoint(RenderRequest->ImageWidth, RenderRequest->ImageHeight)));
                params.DestRect = FResolveRect(FIntRect(FIntPoint(0, 0), FIntPoint(RenderRequest->ImageWidth, RenderRequest->ImageHeight)));
                params.CubeFace = (ECubeFace)RenderRequest->FaceNumber;

                RHICmdList.CopyToResolveTarget(
                    RenderRequest->RenderCommandContext.SrcRenderTarget->GetTextureRHI(),
                    RenderRequest->RenderCommandContext.OutTex,
                    params);
            }

            //if (versionToggle == "D3D11")
            //{
            //    //READ SURFACE DATA
            //    //this only works in dx11

            //    //original dx11 code only needed to do this bit
            //    RHICmdList.ReadSurfaceData(
            //        RenderRequest->RenderCommandContext.SrcRenderTarget->GetTextureRHI(),
            //        RenderRequest->RenderCommandContext.Rect,
            //        *RenderRequest->RenderCommandContext.OutData,
            //        RenderRequest->RenderCommandContext.Flags
            //    );
            //}
            //else if (versionToggle == "D3D12")
            //{
            //    //CREATE TEXTURE / COPY TO RESOLVE TARGET
            //    //this only works in dx12 

            //    int32 outWidth, outHeight;
            //    RHICmdList.MapStagingSurface(texRef, RenderRequest->RawImageData, outWidth, outHeight);
            //    RenderRequest->RenderCommandContext.OutData->Init(FColor(0, 0, 0, 0), outWidth * outHeight);
            //    FMemory::Memcpy(RenderRequest->RenderCommandContext.OutData->GetData(), RenderRequest->RawImageData, outWidth * outHeight * bpp);
            //    RHICmdList.UnmapStagingSurface(texRef);
            //}

            //FUpdateTextureRegion2D updateRegion = FUpdateTextureRegion2D(0, 0, 0, 0, RenderRequest->ImageWidth, RenderRequest->ImageHeight);

            //RHIUpdateTexture2D(RenderRequest->RenderCommandContext.OutTex->GetTexture2D(), 0, updateRegion, RenderRequest->ImageWidth * 4, (uint8*)(&RenderRequest->RenderCommandContext.OutData));
            
        });

    RenderRequest->RenderFence.BeginFence();
}

void URenderStreamCubeCaptureComponent::EnvmapCaptureFunc()
{


    FTextureRenderTargetCubeResource* renderTarget = nullptr;
    if(m_async)
        renderTarget = static_cast<FTextureRenderTargetCubeResource*>(RenderTarget->GetRenderTargetResource());
    else
        renderTarget = static_cast<FTextureRenderTargetCubeResource*>(RenderTarget->GameThread_GetRenderTargetResource());

    TArray<FColor> tData;
    for (int face = 0; face < ECubeFace::CubeFace_MAX; face++)
    {
        TSharedPtr<FRenderRequest> renderRequest(new FRenderRequest);

        ReadPixels(renderRequest, renderTarget, FReadSurfaceDataFlags(RCM_UNorm, (ECubeFace)face));

        m_envmapRenderRequests.Enqueue(renderRequest);
    }

    m_renderGroupCounter++;
}

void URenderStreamCubeCaptureComponent::StartEnvmapCaptureThread()
{
    if (m_envmapThread.Init(CaptureTimerMillis, CaptureOnCameraMove, std::bind(&URenderStreamCubeCaptureComponent::EnvmapCaptureFunc, this), m_async))
    {
        m_envmapThreadRunning = true;
    }
}

void URenderStreamCubeCaptureComponent::StopEnvmapCaptureThread()
{
    m_envmapThreadRunning = false;
    m_envmapThread.Stop();
}