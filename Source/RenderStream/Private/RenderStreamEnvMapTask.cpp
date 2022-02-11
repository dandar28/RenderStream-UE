#include "RenderStreamEnvMapTask.h"

URenderStreamEnvMapTask::URenderStreamEnvMapTask()
{
    m_thread = nullptr;
    m_startTime = 0;
    m_generating = false;
}

URenderStreamEnvMapTask::~URenderStreamEnvMapTask()
{

}

bool URenderStreamEnvMapTask::Init(int captureTimerMillis, bool captureOnCameraMove, std::function<void()> EnvmapFunc, bool async)
{
    m_captureTimerMillis = captureTimerMillis;
    m_captureOnCameraMove = captureOnCameraMove;
    m_envmapFunc = EnvmapFunc;
    m_async = async;
    
    if (m_captureTimerMillis > 0)
    {
        m_startTime = FDateTime::Now();
    }

    if (!m_thread && m_async)
    {
        m_thread = FRunnableThread::Create(this, TEXT("EnvmapTask"), 0, TPri_BelowNormal);
    }

    return m_thread != nullptr || async;
}

uint32 URenderStreamEnvMapTask::Run()
{
    while (m_stopTaskCounter.GetValue() == 0)
    {
        RunTick();
        FPlatformProcess::Sleep(0.01f);
    }

    return 0;
}

void URenderStreamEnvMapTask::RunTick()
{
    FDateTime ms = FDateTime::Now();
    FTimespan diff = ms - m_startTime;

    if ((m_captureOnCameraMove && m_cameraMovedSignal) || (m_captureTimerMillis != 0 && diff.GetTotalMilliseconds() >= m_captureTimerMillis))
    {
        m_cameraMovedSignal = false;

        GenerateAndSendEnvironmentMap();

        m_startTime = FDateTime::Now();
    }
}

void URenderStreamEnvMapTask::Stop()
{
    m_stopTaskCounter.Increment();
}

void URenderStreamEnvMapTask::GenerateAndSendEnvironmentMap()
{
    if (!m_generating && m_envmapFunc)
    {
        m_generating = true;

        m_envmapFunc();

        m_generating = false;
    }
}