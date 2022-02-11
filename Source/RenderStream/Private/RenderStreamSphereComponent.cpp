// Fill out your copyright notice in the Description page of Project Settings.


#include "RenderStreamSphereComponent.h"
#include "RenderStreamLink.h"
#include "ImageUtils.h"

// Sets default values for this component's properties
URenderStreamSphereComponent::URenderStreamSphereComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void URenderStreamSphereComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
    m_startTime = FDateTime::Now();
}


// Called every frame
void URenderStreamSphereComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    //for now just do it here
    FTimespan diff = FDateTime::Now() - m_startTime;
    if (CaptureTimerMillis != 0 && diff.GetTotalMilliseconds() >= CaptureTimerMillis)
    {
        EnvmapCaptureFunc();
    }
}

void URenderStreamSphereComponent::Init(std::function<void(RenderStreamLink::EnvmapCaptureType envmapCaptureType, FTexture2DRHIRef envmapTex, int width, int height)> sendCallback)
{
    m_sendCallback = sendCallback;
}


#include "CubemapUnwrapUtils.h"
#include "Rendering/Texture2DResource.h"

void URenderStreamSphereComponent::EnvmapCaptureFunc()
{
    //auto SaveImage = [this](FString imgPath, UTextureRenderTargetCube* target) -> void
    //{
    //    FText pathErr;
    //    FPaths::ValidatePath(imgPath, &pathErr);
    //    FArchive* archive = IFileManager::Get().CreateFileWriter(*imgPath);

    //    if (archive)
    //    {
    //        FBufferArchive buffer;
            //bool exported = FImageUtils::ExportRenderTargetCubeAsHDR(target, buffer);
    //        if (exported)
    //        {
    //            archive->Serialize(buffer.GetData(), buffer.Num());
    //        }
    //    }
    //};

    //SaveImage(FString::Printf(TEXT("C:\\temp\\cube%d.hdr"), m_renderGroupCounter), RenderTarget);

    TArray64<uint8> pixels;
    FIntPoint sz;
    EPixelFormat frm;
    CubemapHelpers::GenerateLongLatUnwrap(RenderTarget, pixels, sz, frm);

    FRHIResourceCreateInfo create;
    FTexture2DRHIRef rhiRef = RHICreateTexture2D(sz.X, sz.Y, frm, 1, 1, TexCreate_RenderTargetable, create);

    ENQUEUE_RENDER_COMMAND(UpdateTextureDataCommand)([rhiRef, sz, pixels](FRHICommandListImmediate& RHICmdList) {
        RHIUpdateTexture2D(
            rhiRef,
            0,
            FUpdateTextureRegion2D(0, 0, 0, 0, sz.X, sz.Y),
            rhiRef->GetSizeX() * 4,
            pixels.GetData()
        );
        });

    if (m_sendCallback)
    {
        //FTexture2DRHIRef tex = RenderTarget->GameThread_GetRenderTargetResource()->GetRenderTargetTexture();
        /*int w = RenderTarget->SizeX*2;
        int h = RenderTarget->SizeX;*/

        //UTexture2D* t = UTexture2D::CreateTransient(sz.X, sz.Y, frm);
        //uint8* texData = (uint8*)t->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
        //FMemory::Memcpy(texData, pixels.GetData(), pixels.Num());
        //t->PlatformData->Mips[0].BulkData.Unlock();




        //ENQUEUE_RENDER_COMMAND(ReadTexCmd)([pixels, sz, frm, rhiRef](FRHICommandListImmediate& RHICmdList)
        //    {
        //        void* buf = nullptr;
        //        int outW, outH;
        //        RHICmdList.MapStagingSurface(rhiRef, buf, outW, outH);
        //        FMemory::Memcpy(buf, pixels.GetData(), pixels.Num());
        //        RHICmdList.UnmapStagingSurface(rhiRef);
        //    });

        m_sendCallback(RenderStreamLink::EnvmapCaptureType::ENV_SPHERE, rhiRef, sz.X, sz.Y);
    }
}