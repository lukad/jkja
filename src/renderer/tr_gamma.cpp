// Brightness without a gamma ramp (ADR-0018). The release hands the table that
// R_SetColorMappings makes from r_gamma and r_overBrightBits to the display. Here the
// back end keeps it and, at the end of a frame, draws the frame back through a fragment
// program that looks every colour channel up in it.

#include "../server/exe_headers.h"

#include "tr_local.h"

#include <cstring>

extern int giTextureBindNum;

namespace {

constexpr GLenum programErrorPosition = 0x864B; // GL_PROGRAM_ERROR_POSITION_ARB

// Texture 0 is the frame, addressed in pixels; texture 1 is the table, 256 texels in a
// row. A channel value v of 255 has to land on the centre of texel v:
// (v / 255) * (255 / 256) + 0.5 / 256.
const char programText[] = "!!ARBfp1.0\n"
                           "PARAM k = { 0.99609375, 0.001953125, 0.5, 1.0 };\n"
                           "TEMP c, at, v;\n"
                           "TEX c, fragment.texcoord[0], texture[0], RECT;\n"
                           "MAD c, c, k.x, k.y;\n"
                           "MOV at, k.z;\n"
                           "MOV at.x, c.r;\n"
                           "TEX v, at, texture[1], 2D;\n"
                           "MOV result.color.r, v.r;\n"
                           "MOV at.x, c.g;\n"
                           "TEX v, at, texture[1], 2D;\n"
                           "MOV result.color.g, v.g;\n"
                           "MOV at.x, c.b;\n"
                           "TEX v, at, texture[1], 2D;\n"
                           "MOV result.color.b, v.b;\n"
                           "MOV result.color.a, k.w;\n"
                           "END\n";

byte table[256][4];
bool tableIsIdentity = true;
bool tableChanged = true;

// Zero while the context has none of them.
GLuint program;
GLuint frameTexture;
GLuint tableTexture;

// Whether frameTexture holds the frame that was shown last, without the table applied.
bool frameIsCurrent;

void FrameQuad() {
	const float width = static_cast<float>(glConfig.vidWidth);
	const float height = static_cast<float>(glConfig.vidHeight);

	qglBegin(GL_QUADS);
	qglTexCoord2f(0, 0);
	qglVertex2f(0, 0);
	qglTexCoord2f(width, 0);
	qglVertex2f(width, 0);
	qglTexCoord2f(width, height);
	qglVertex2f(width, height);
	qglTexCoord2f(0, height);
	qglVertex2f(0, height);
	qglEnd();
}

} // namespace

/*
===============
GLimp_SetGamma

The interface the release has to the display's gamma ramp. Nothing is sent to a display:
the table is kept for RB_GammaPass.
===============
*/
void GLimp_SetGamma(unsigned char red[256], unsigned char green[256], unsigned char blue[256]) {
	tableIsIdentity = true;
	for (int i = 0; i < 256; i++) {
		table[i][0] = red[i];
		table[i][1] = green[i];
		table[i][2] = blue[i];
		table[i][3] = 255;
		if (red[i] != i || green[i] != i || blue[i] != i) {
			tableIsIdentity = false;
		}
	}
	tableChanged = true;
}

/*
===============
RB_GammaInit

Called when the renderer creates its own images, with glConfig.deviceSupportsGamma
saying whether the context has what the pass needs. Should the driver refuse the
program after all, the renderer goes back to the path for devices without gamma control.
===============
*/
void RB_GammaInit(void) {
	program = 0;
	frameTexture = 0;
	tableTexture = 0;
	frameIsCurrent = false;
	tableChanged = true;

	if (!glConfig.deviceSupportsGamma) {
		return;
	}

	qglGenProgramsARB(1, &program);
	qglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, program);
	qglProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
	                    static_cast<GLsizei>(std::strlen(programText)), programText);
	GLint errorPosition = -1;
	qglGetIntegerv(programErrorPosition, &errorPosition);
	if (errorPosition != -1) {
		VID_Printf(PRINT_WARNING,
		           "RB_GammaInit: the driver refused the brightness program at character %d\n",
		           errorPosition);
		qglDeleteProgramsARB(1, &program);
		program = 0;
		glConfig.deviceSupportsGamma = qfalse;
		R_SetColorMappings();
		return;
	}

	// The renderer hands out texture names itself; these two come from the same count.
	frameTexture = 1024 + giTextureBindNum++;
	tableTexture = 1024 + giTextureBindNum++;

	qglBindTexture(GL_TEXTURE_RECTANGLE_EXT, frameTexture);
	qglTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGB8, glConfig.vidWidth, glConfig.vidHeight, 0,
	              GL_RGB, GL_UNSIGNED_BYTE, NULL);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP);
	qglTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// GL_Bind does not know of this binding of the unit's 2D texture.
	GL_SelectTexture(0);
	qglBindTexture(GL_TEXTURE_2D, tableTexture);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, table);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glState.currenttextures[0] = 0;
}

/*
===============
RB_GammaShutdown
===============
*/
void RB_GammaShutdown(void) {
	if (program) {
		qglDeleteProgramsARB(1, &program);
		qglDeleteTextures(1, &frameTexture);
		qglDeleteTextures(1, &tableTexture);
	}
	program = 0;
	frameTexture = 0;
	tableTexture = 0;
	frameIsCurrent = false;
}

/*
===============
RB_GammaPass

Called when a frame is complete and before it is shown. While the table is the identity
nothing is done, and the frame is exactly what the renderer drew.
===============
*/
void RB_GammaPass(void) {
	frameIsCurrent = false;
	if (!program || tableIsIdentity) {
		return;
	}

	const int width = glConfig.vidWidth;
	const int height = glConfig.vidHeight;

	// The last view of the frame may have covered a part of the window only.
	qglViewport(0, 0, width, height);
	qglScissor(0, 0, width, height);
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho(0, width, 0, height, -1, 1);
	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
	qglLoadIdentity();

	GL_Cull(CT_TWO_SIDED);
	GL_State(GLS_DEPTHTEST_DISABLE);

	// A fragment program reads the textures that are bound whether or not texturing is
	// enabled for their targets, so the units' enables stay as they are.
	GL_SelectTexture(1);
	qglBindTexture(GL_TEXTURE_2D, tableTexture);
	glState.currenttextures[1] = 0;
	if (tableChanged) {
		qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA, GL_UNSIGNED_BYTE, table);
		tableChanged = false;
	}

	GL_SelectTexture(0);
	qglBindTexture(GL_TEXTURE_RECTANGLE_EXT, frameTexture);
	qglCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, 0, 0, 0, 0, width, height);

	qglEnable(GL_FRAGMENT_PROGRAM_ARB);
	qglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, program);
	FrameQuad();
	qglDisable(GL_FRAGMENT_PROGRAM_ARB);

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();

	frameIsCurrent = true;
}

/*
===============
RB_ReadFrame

The whole frame, for code that draws it again: a dissolve, the picture behind a menu.
Such code must not get the frame with the table applied, because the table is applied
again to what it draws. While the pass is at work the frame is therefore read from the
copy the pass made, which is the frame that was shown last.
===============
*/
void RB_ReadFrame(GLenum format, byte *pixels) {
	if (frameIsCurrent) {
		qglBindTexture(GL_TEXTURE_RECTANGLE_EXT, frameTexture);
		qglGetTexImage(GL_TEXTURE_RECTANGLE_EXT, 0, format, GL_UNSIGNED_BYTE, pixels);
		return;
	}
	qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, format, GL_UNSIGNED_BYTE, pixels);
}
