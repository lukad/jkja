// The window and its OpenGL context, for the legacy renderer.

#include "sys_local.h"

#include "../renderer/tr_local.h"

#include <SDL3/SDL.h>

#include <cmath>

namespace {

SDL_Window *window;
SDL_GLContext context;

// The mode that is used when r_mode does not name one: 640 by 480.
constexpr int fallbackMode = 3;

int Attribute(SDL_GLAttr attribute) {
	int value = 0;
	SDL_GL_GetAttribute(attribute, &value);
	return value;
}

void DestroyWindow() {
	if (context) {
		SDL_GL_DestroyContext(context);
		context = nullptr;
	}
	if (window) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}
}

} // namespace

SDL_Window *Sys_Window(void) {
	return window;
}

/*
===============
GLimp_Init

Opens the window in the mode the r_ cvars ask for, creates the OpenGL context, binds the
qgl functions to it and lets the renderer look at the driver.

A full-screen window takes the size of the desktop instead of changing the mode of the
display, so r_mode only sizes a window on the desktop.
===============
*/
void GLimp_Init(void) {
	if (Sys_Headless()) {
		NullWindow_Init();
		return;
	}

	Com_Printf("Initializing OpenGL subsystem\n");

	if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		Com_Error(ERR_FATAL,
		          "GLimp_Init() - no video system: %s\n"
		          "(+set sys_headless 1 runs without a window)\n",
		          SDL_GetError());
	}

	int width = 0;
	int height = 0;
	if (!R_GetModeInfo(&width, &height, r_mode->integer)) {
		Com_Printf("...r_mode %d is not a mode, using %d\n", r_mode->integer, fallbackMode);
		R_GetModeInfo(&width, &height, fallbackMode);
	}
	const bool fullscreen = r_fullscreen->integer != 0;

	// The renderer is written against OpenGL 1.1 with extensions and uses the fixed
	// function pipeline throughout. No version and no profile are asked for, which gets
	// the context that still has all of that.
	SDL_GL_ResetAttributes();
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, r_depthbits->integer ? r_depthbits->integer : 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, r_stencilbits->integer);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	if (fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	window = SDL_CreateWindow("Jedi Academy", width, height, flags);
	if (!window) {
		Com_Error(ERR_FATAL, "GLimp_Init() - could not open a window: %s\n", SDL_GetError());
	}
	context = SDL_GL_CreateContext(window);
	if (!context) {
		const char *reason = SDL_GetError();
		DestroyWindow();
		Com_Error(ERR_FATAL, "GLimp_Init() - could not create an OpenGL context: %s\n", reason);
	}

	if (!QGL_Bind()) {
		DestroyWindow();
		Com_Error(ERR_FATAL, "GLimp_Init() - the OpenGL context lacks functions of OpenGL 1.1\n");
	}

	// The size in pixels is what the renderer draws to; it differs from the size that
	// was asked for on a full-screen window and on a display with a scale factor. Some
	// window systems apply the full-screen state only a moment after the window exists,
	// and until then the window still has the size it was asked for: wait for them.
	SDL_SyncWindow(window);
	SDL_GetWindowSizeInPixels(window, &width, &height);
	glConfig.vidWidth = width;
	glConfig.vidHeight = height;
	glConfig.isFullscreen = fullscreen ? qtrue : qfalse;
	glConfig.stereoEnabled = qfalse;

	glConfig.colorBits = Attribute(SDL_GL_RED_SIZE) + Attribute(SDL_GL_GREEN_SIZE) +
	                     Attribute(SDL_GL_BLUE_SIZE) + Attribute(SDL_GL_ALPHA_SIZE);
	glConfig.depthBits = Attribute(SDL_GL_DEPTH_SIZE);
	glConfig.stencilBits = Attribute(SDL_GL_STENCIL_SIZE);

	const SDL_DisplayMode *displayMode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window));
	glConfig.displayFrequency =
	    displayMode ? static_cast<int>(std::lround(displayMode->refresh_rate)) : 0;

	Com_Printf("...%d by %d pixels, %s, %d colour, %d depth and %d stencil bits\n",
	           glConfig.vidWidth, glConfig.vidHeight, fullscreen ? "full screen" : "in a window",
	           glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);

	R_InitGLConfig();

	// GLimp_EndFrame sets the swap interval when the cvar has changed.
	r_swapInterval->modified = qtrue;

	// Typed characters are wanted from the start, by the console and the menus.
	SDL_StartTextInput(window);
}

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown(void) {
	if (window) {
		Com_Printf("Shutting down OpenGL subsystem\n");
		DestroyWindow();
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}

	// Nothing may call into a context that is gone.
	QGL_BindNothing();

	memset(&glConfig, 0, sizeof(glConfig));
	memset(&glState, 0, sizeof(glState));
}

/*
===============
GLimp_EndFrame
===============
*/
void GLimp_EndFrame(void) {
	if (!window) {
		return;
	}

	if (r_swapInterval->modified) {
		r_swapInterval->modified = qfalse;
		SDL_GL_SetSwapInterval(r_swapInterval->integer);
	}

	SDL_GL_SwapWindow(window);
}

/*
===============
GLimp_GetProcAddress
===============
*/
glFunction_t GLimp_GetProcAddress(const char *name) {
	return window ? SDL_GL_GetProcAddress(name) : nullptr;
}

/*
===============
GLimp_LogComment

The log of OpenGL calls that r_logFile turned on was part of the Windows binding of the
qgl functions; there is none.
===============
*/
void GLimp_LogComment(char *) {}
