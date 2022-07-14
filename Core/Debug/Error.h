#pragma once

namespace nv::debug
{
    enum Severity
    {
        SEV_WARNING,
        SEV_CRITICAL
    };

    void        ReportError(const char* reason);
    const char* GetRecentError();
    bool        IsErrorReported();
}