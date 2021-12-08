#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTargetCube.h"

#include "RenderRequest.generated.h"

USTRUCT()
struct FReadSurfaceContext
{
    GENERATED_BODY()

    FTextureRenderTargetCubeResource* SrcRenderTarget;
    TArray<FColor>* OutData;
    FRHITexture* OutTex;
    FIntRect Rect;
    FReadSurfaceDataFlags Flags;
};

USTRUCT()
struct FRenderRequest
{
    GENERATED_BODY()

    void* RawImageData;
    TArray<FColor> ImageData;
    FRenderCommandFence RenderFence;
    unsigned short FaceNumber;
    unsigned int RenderGroup;
    unsigned int ImageWidth;
    unsigned int ImageHeight;

    FReadSurfaceContext RenderCommandContext;
};