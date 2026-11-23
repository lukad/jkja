// The window when there is none. The legacy renderer still loads maps and models and
// runs its front end; its OpenGL calls go to functions that do nothing
// (docs/design.md, section 4.5).

#include "sys_local.h"

#include "../renderer/tr_local.h"

/*
===============
NullWindow_Init

Describes an OpenGL 1.1 without extensions and with a 640 by 480 window, so the
renderer takes its simplest paths.
===============
*/
void NullWindow_Init(void) {
	QGL_BindNothing();

	glConfig.renderer_string = "none";
	glConfig.vendor_string = "none";
	glConfig.version_string = "1.1";
	glConfig.extensions_string = "";

	glConfig.maxTextureSize = 2048;
	glConfig.maxActiveTextures = 1;
	glConfig.maxTextureFilterAnisotropy = 0;

	glConfig.colorBits = 32;
	glConfig.depthBits = 24;
	glConfig.stencilBits = 8;

	glConfig.deviceSupportsGamma = qfalse;
	glConfig.textureCompression = TC_NONE;
	glConfig.textureEnvAddAvailable = qfalse;
	glConfig.textureFilterAnisotropicAvailable = qfalse;
	glConfig.clampToEdgeAvailable = qfalse;

	glConfig.vidWidth = 640;
	glConfig.vidHeight = 480;
	glConfig.displayFrequency = 60;
	glConfig.isFullscreen = qfalse;
	glConfig.stereoEnabled = qfalse;
}
