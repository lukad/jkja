#pragma once
#if !defined(G_HEADERS_H_INC)
#define G_HEADERS_H_INC

#if !defined(G_LOCAL_H_INC)
	#include "g_local.h"
#endif

//#if !defined(G_WRAITH_H_INC)
//	#include "../game/g_Wraith.h"
//#endif

#if !defined(TEAMS_H_INC)
	#include "teams.h"
#endif

//#if !defined(IGINTERFACE_H_INC)
//	#include "../game/IGInterface.h"
//#endif

// What every game source needs. The release got these through its precompiled header
// on the PC; without one they are included here, as the console build did.
	#include "b_local.h"
	#include "g_functions.h"
	#include "g_nav.h"
	#include "g_navigator.h"
	#include "g_vehicles.h"
	#include "../cgame/cg_camera.h"	// Just for AI_Rancor
	#include "../cgame/cg_local.h"	// Evil? Maybe. Necessary? Absolutely.

#endif // G_HEADERS_H_INC
