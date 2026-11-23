/*
** WIN_GAMMA.C
*/
// leave this as first line for PCH reasons...
//
//#include "../server/exe_headers.h"



#include <assert.h>
#include "../game/q_shared.h"
#include "../renderer/tr_local.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"



/*
** WG_CheckHardwareGamma
**
** Determines if the underlying hardware supports the Win32 gamma correction API.
*/
void WG_CheckHardwareGamma( void )
{
	glConfig.deviceSupportsGamma = qtrue;
}

/*
** GLimp_SetGamma
**
** This routine should only be called if glConfig.deviceSupportsGamma is TRUE
*/
void GLimp_SetGamma( float g ) {
}

