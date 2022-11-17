// Fill out your copyright notice in the Description page of Project Settings.


#include "RenderStreamLiveLinkSource.h"

#include "Roles/LiveLinkAnimationRole.h"
#include "Roles/LiveLinkAnimationTypes.h"

#include "RenderStreamHelper.h"

void FRenderStreamLiveLinkSource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
{
    Client = InClient;
    SourceGuid = InSourceGuid;
}

bool FRenderStreamLiveLinkSource::IsSourceStillValid() const
{
    return Client != NULL;
}

bool FRenderStreamLiveLinkSource::RequestSourceShutdown()
{
    return true;
}

void FRenderStreamLiveLinkSource::Update()
{
    
}

void FRenderStreamLiveLinkSource::PushFrameAnimData(const FName& SubjectName, const RenderStreamLink::FSkeletalLayout& Layout, const RenderStreamLink::FSkeletalPose& Pose)
{
    check(Client);

    FLiveLinkSubjectKey SubjectKey{ SourceGuid, SubjectName };
    { // Static data
        FLiveLinkStaticDataStruct StaticDataStruct = FLiveLinkStaticDataStruct(FLiveLinkSkeletonStaticData::StaticStruct());
        FLiveLinkSkeletonStaticData& StaticSkeleton = *StaticDataStruct.Cast<FLiveLinkSkeletonStaticData>();

        for (const auto& Joint : Layout.joints)
        {
            // TODO: send bone names over RenderStream ?
            StaticSkeleton.BoneNames.Add(FName(FString::FromInt(static_cast<int32>(Joint.id))));
            int32 idx = Layout.joints.IndexOfByPredicate([&Joint](const auto& OtherJoint) { return OtherJoint.id == Joint.parentId; });
            StaticSkeleton.BoneParents.Add(idx); // Root bone is indicated by negative index, which IndexOfByPredicate will return if not found
        }

        Client->PushSubjectStaticData_AnyThread(SubjectKey, ULiveLinkAnimationRole::StaticClass(), MoveTemp(StaticDataStruct));
    }

    { // Frame Data

        static const FMatrix YUpMatrix(FVector(0.0f, 0.0f, 1.0f), FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f));

        FLiveLinkFrameDataStruct FrameDataStruct = FLiveLinkFrameDataStruct(FLiveLinkAnimationFrameData::StaticStruct());
        FLiveLinkAnimationFrameData& FrameData = *FrameDataStruct.Cast<FLiveLinkAnimationFrameData>();

        for (const auto& Joint : Pose.joints)
        {
            FVector Scale(1, 1, 1);
            FQuat Rot(Joint.transform.rx, Joint.transform.ry, Joint.transform.rz, Joint.transform.rw);
            FVector Trans(Joint.transform.x, Joint.transform.y, Joint.transform.z);

            FTransform Transform = d3ToUEHelpers::Convertd3TransformToUE(Scale, Rot, Trans, YUpMatrix);

            FrameData.Transforms.Add(Transform);
        }

        Client->PushSubjectFrameData_AnyThread(SubjectKey, MoveTemp(FrameDataStruct));
    }
}

FText FRenderStreamLiveLinkSource::GetSourceType() const
{
    return FText::FromString("RenderStream LiveLink");
}

FText FRenderStreamLiveLinkSource::GetSourceMachineName() const
{
    return FText();
}

FText FRenderStreamLiveLinkSource::GetSourceStatus() const
{
    return FText();
}

void FRenderStreamLiveLinkSource::InitializeSettings(ULiveLinkSourceSettings* Settings)
{
    
}

void FRenderStreamLiveLinkSource::OnSettingsChanged(ULiveLinkSourceSettings* Settings, const FPropertyChangedEvent& PropertyChangedEvent)
{
    
}
