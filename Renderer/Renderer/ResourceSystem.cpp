#include "pch.h"

#include "ResourceSystem.h"
#include <Renderer/ResourceManager.h>
#include <Engine/JobSystem.h>

namespace nv::graphics
{
    void ResourceSystem::Init()
    {
    }

    Handle<jobs::Job> mAsyncLoadJob = Null<jobs::Job>();

    void ResourceSystem::Update(float deltaTime, float totalTime)
    {
        if (nv::jobs::IsFinished(mAsyncLoadJob))
        {
            mAsyncLoadJob = Null<jobs::Job>();
        }

        if (gResourceManager->GetAsyncLoadQueueSize() > 0 && mAsyncLoadJob.IsNull())
        {
            mAsyncLoadJob = nv::jobs::Execute([](void* pContext)
            {
                gResourceManager->ProcessAsyncLoadQueue();
            });
        }
    }
}