//===-- OsLogger.cpp --------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "OsLogger.h"

#if LLDB_USE_OS_LOG

#include <os/log.h>

#include "DNBDefs.h"
#include "DNBLog.h"

#define LLDB_OS_LOG_MAX_BUFFER_LENGTH 256

namespace
{
    //----------------------------------------------------------------------
    // Darwin os_log logging callback that can be registered with
    // DNBLogSetLogCallback
    //----------------------------------------------------------------------
    void
    DarwinLogCallback(void *baton, uint32_t flags, const char *format,
                      va_list args)
    {
        if (format == nullptr)
            return;

        static os_log_t g_logger;
        if (!g_logger)
        {
            g_logger = os_log_create("com.apple.dt.lldb", "debugserver");
            if (!g_logger)
                return;
        }

        os_log_type_t log_type;
        if (flags & DNBLOG_FLAG_FATAL)        log_type = OS_LOG_TYPE_FAULT;
        else if (flags & DNBLOG_FLAG_ERROR)   log_type = OS_LOG_TYPE_ERROR;
        else if (flags & DNBLOG_FLAG_WARNING) log_type = OS_LOG_TYPE_DEFAULT;
        else if (flags & DNBLOG_FLAG_VERBOSE) log_type = OS_LOG_TYPE_DEBUG;
        else                                  log_type = OS_LOG_TYPE_DEFAULT;

        // This code is unfortunate.  os_log* only takes static strings, but
        // our current log API isn't set up to make use of that style.
        char buffer[LLDB_OS_LOG_MAX_BUFFER_LENGTH];
        vsnprintf(buffer, sizeof(buffer), format, args);
        os_log_with_type(g_logger, log_type, "%{public}s", buffer);
    }
}

DNBCallbackLog
OsLogger::GetLogFunction()
{
    return _os_log_impl ? DarwinLogCallback : nullptr;
}

#else

DNBCallbackLog
OsLogger::GetLogFunction()
{
    return nullptr;
}

#endif
