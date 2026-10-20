#pragma once
#ifdef FIS4_PS4
#include <sstream>
#include <orbis/libkernel.h>
struct DebugLog {
    std::ostringstream ss;
    ~DebugLog() { ss << "\n"; sceKernelDebugOutText(0, ss.str().c_str()); }
    template<typename T> DebugLog& operator<<(const T& v){ ss << v; return *this; }
};
#define DEBUGLOG DebugLog()
#else
#include <iostream>
#define DEBUGLOG std::cout
#endif
