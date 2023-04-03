#pragma once

#include "Animation/AnimNodeBase.h"

#include "CoreMinimal.h"
#include "LiveLinkRetargetAsset.h"
#include "LiveLinkTypes.h"
#include "RenderStreamRemapAsset.h"
#include "RenderStreamLink.h"

#include "AnimNode_RenderStreamSkeletonSource.generated.h"

class ILiveLinkClient;
class ASkeletalMeshActor;

USTRUCT(BlueprintType)
struct RENDERSTREAM_API FAnimNode_RenderStreamSkeletonSource : public FAnimNode_Base
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input)
        FPoseLink BasePose;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, NoClear, Category = Retarget, meta = (PinShownByDefault))
        TSubclassOf<URenderStreamRemapAsset> RetargetAsset;

    UPROPERTY(transient)
        TObjectPtr<URenderStreamRemapAsset> CurrentRetargetAsset;

public:
    FAnimNode_RenderStreamSkeletonSource();

    virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    virtual void Evaluate_AnyThread(FPoseContext& Output) override;
    virtual bool HasPreUpdate() const { return true; }
    virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
    virtual void GatherDebugData(FNodeDebugData& DebugData) override;

    //bool Serialize(FArchive& Ar);

protected:
    virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;

    void ProcessSkeletonData(const RenderStreamLink::FSkeletalLayout* Layout, const RenderStreamLink::FSkeletalPose* Pose,
        FLiveLinkSkeletonStaticData& LiveLinkStatic, FLiveLinkAnimationFrameData& LiveLinkFrame);

    void CacheSkeletonActors(const FName& ParamName);
    void ApplyRootPose(const FName& ParamName);

    FName GetSkeletonParamName();

    void InitialiseAnimationData(const FLiveLinkSkeletonStaticData* InSkeletonData, const FLiveLinkAnimationFrameData* InFrameData, const FCompactPose& OutPose);
    void BuildPoseFromAnimationData(float DeltaTime, const FLiveLinkSkeletonStaticData* InSkeletonData, const FLiveLinkAnimationFrameData* InFrameData, FCompactPose& OutPose);

private:

    ILiveLinkClient* LiveLinkClient_AnyThread;

    // Delta time from update so that it can be passed to retargeter
    float CachedDeltaTime;

    std::vector<TWeakObjectPtr<ASkeletalMeshActor>> SkeletonActors;
    bool SkeletonActorsCached;

    // Cached pose info
    TArray<FTransform> InitialPose;
    TArray<FQuat> MeshToSourceInitialOrientations;
    TArray<FQuat> LocalInitialOrientationDifferences;
    TArray<FQuat> SourceInitialPoseRotations;
    TArray<FCompactPoseBoneIndex> SourceToMeshIndex;
    FTransform RootBoneTransform;
    bool PoseInitialised;

};

