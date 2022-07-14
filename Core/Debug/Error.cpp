#include "pch.h"
#include "Error.h"

#include <Engine/Instance.h>

namespace nv::debug
{ 

    void nv::debug::ReportError(const char* reason)
    {
        Instance::SetError(true, reason);
    }
    
    const char* nv::debug::GetRecentError()
    {
        return Instance::GetErrorReason();
    }
    
    bool nv::debug::IsErrorReported()
    {
        return Instance::HasError();
    }
}
