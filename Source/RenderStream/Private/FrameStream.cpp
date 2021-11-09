#include "FrameStream.h"
#include "RenderStream.h"
#include "RSUCHelpers.inl"

FFrameStream::FFrameStream()
    : m_streamName(""), m_handle(0) 
{
    const int numTex = FMath::Log2(RenderStreamLink::EnhancedCaptureFrameType::ALL + 1);
    for (int i = 0; i < numTex; i++)
    {
        m_bufTextures.Add(nullptr);
    }
}

FFrameStream::~FFrameStream()
{
}

void FFrameStream::SendFrame_RenderingThread(FRHICommandListImmediate& RHICmdList, RenderStreamLink::CameraResponseData& FrameData, FRHITexture2D* SourceTexture, const FIntRect& ViewportRect)
{
    float ULeft = (float)ViewportRect.Min.X / (float)SourceTexture->GetSizeX();
    float URight = (float)ViewportRect.Max.X / (float)SourceTexture->GetSizeX();
    float VTop = (float)ViewportRect.Min.Y / (float)SourceTexture->GetSizeY();
    float VBottom = (float)ViewportRect.Max.Y / (float)SourceTexture->GetSizeY();
    const int bufIndex = FMath::Log2(FrameData.enhancedCaptureType);
    RSUCHelpers::SendFrame(m_handle, m_bufTextures[bufIndex], RHICmdList, FrameData, SourceTexture, SourceTexture->GetSizeXY(), { ULeft, URight }, { VTop, VBottom });
}

bool FFrameStream::Setup(const FString& name, const FIntPoint& Resolution, const FString& Channel, const RenderStreamLink::ProjectionClipping& Clipping, RenderStreamLink::StreamHandle Handle, RenderStreamLink::RSPixelFormat fmt)
{
    if (m_handle != 0)
        return false; // already have a stream handle call stop first

    m_handle = Handle;
    m_channel = Channel;
    m_clipping = Clipping;
    m_resolution = Resolution;
    m_streamName = name;

    for (FTextureRHIRef& buf : m_bufTextures)
    {
        if (!RSUCHelpers::CreateStreamResources(buf, m_resolution, fmt))
            return false; // helper method logs on failure
    }

    if (m_handle == 0) {
        UE_LOG(LogRenderStream, Error, TEXT("Unable to create stream"));
        RenderStreamStatus().Output("Error: Unable to create stream", RSSTATUS_RED);
        return false;
    }
    UE_LOG(LogRenderStream, Log, TEXT("Created stream '%s'"), *m_streamName);
    RenderStreamStatus().Output("Connected to stream", RSSTATUS_GREEN);
    
    return true;
}
