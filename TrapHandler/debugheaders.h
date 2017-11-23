#pragma once

#include "TraceEngine.h"

#ifndef __DEBUG_PRINT
#define __DEBUG_PRINT
#define print(x,z,...) pTrace->SendEvent(LOG_DEBUG, x, z, __VA_ARGS__);


//////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug macros
// z = message
#define dd(x,z,...)	pTrace->SendEvent(LOG_DEBUG, x, z, __VA_ARGS__);
#define dn(x,z,...)	pTrace->SendEvent(LOG_NORMAL, x, z, __VA_ARGS__);
#define dw(x,z,...)	pTrace->SendEvent(LOG_WARNING, x, z, __VA_ARGS__);
#define dmi(x,z,...)	pTrace->SendEvent(LOG_MINOR, x, z, __VA_ARGS__);
#define dma(x,z,...)	pTrace->SendEvent(LOG_MAJOR, x, z, __VA_ARGS__);
#define dc(x,z,...)	pTrace->SendEvent(LOG_WARNING, x, z, __VA_ARGS__);

class CTraceEngine;
extern CTraceEngine *pTrace;
extern bool bDebug;

#endif