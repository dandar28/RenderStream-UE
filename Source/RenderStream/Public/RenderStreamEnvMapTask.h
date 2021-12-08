#pragma once

#include <functional>

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "Delegates/Delegate.h"
#include "RenderStream.h"

class RENDERSTREAM_API URenderStreamEnvMapTask : public FRunnable
{
private:
    std::function<void()> m_envmapFunc;
    std::function<void()> m_cameraMovedCallback;
    
    int m_captureTimerMillis;
    bool m_captureOnCameraMove;

    FDateTime m_startTime;

    bool m_generating;

    bool m_cameraMovedSignal;

    bool m_async;

    FRunnableThread* m_thread;

    FThreadSafeCounter m_stopTaskCounter;

public:
    URenderStreamEnvMapTask();
    virtual ~URenderStreamEnvMapTask();

    bool Init(int captureTimerMillis, bool captureOnCameraMove, std::function<void()> EnvmapFunc, bool async);
    
    virtual uint32 Run();

    virtual void Stop();

    void GenerateAndSendEnvironmentMap();

    bool IsGenerating() const { return m_generating; }

    void RunTick();

    void SignalCameraMoved() { m_cameraMovedSignal = true; }
};

