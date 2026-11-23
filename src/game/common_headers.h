#pragma once
#if !defined(COMMON_HEADERS_H_INC)
#define COMMON_HEADERS_H_INC

#if !defined(__Q_SHARED_H)
	#include "q_shared.h"
#endif

//#if !defined(Q_SHAREDBASIC_H_INC)
//	#include "../game/q_sharedbasic.h"			// fileHandle_t
//#endif

//#if !defined(Q_MATH_H_INC)
//	#include "../game/q_math.h"
//#endif

// What the sources that start with this header need. The release got these through
// its precompiled header on the PC; without one they are included here, as the console
// build did.
#define GAME_INCLUDE
	#include "b_local.h"
	#include "../cgame/cg_local.h"
	#include "g_navigator.h"
	#include "g_shared.h"
	#include "g_functions.h"

#endif // COMMON_HEADERS_H_INC
