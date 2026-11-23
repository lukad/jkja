// What the renderer finds out about the OpenGL driver once there is a context: its
// strings and limits, and the extensions the renderer uses. The code is that of the
// release's win32/win_glimp.cpp without the parts that addressed Windows; the platform
// layer (sys/) provides the context and GLimp_GetProcAddress.

#include "../server/exe_headers.h"

#include "tr_local.h"

extern bool g_bDynamicGlowSupported;
extern bool g_bTextureRectangleHack;

qboolean Sys_LowPhysicalMemory();

static void R_InitTextureCompression( void )
{
	qboolean newer_tc, old_tc;

	// Check for available tc methods.
	newer_tc = ( strstr( glConfig.extensions_string, "ARB_texture_compression" )
		&& strstr( glConfig.extensions_string, "EXT_texture_compression_s3tc" )) ? qtrue : qfalse;
	old_tc = ( strstr( glConfig.extensions_string, "GL_S3_s3tc" )) ? qtrue : qfalse;

	if ( old_tc )
	{
		VID_Printf( PRINT_ALL, "...GL_S3_s3tc available\n" );
	}

	if ( newer_tc )
	{
		VID_Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc available\n" );
	}

	if ( !r_ext_compressed_textures->value )
	{
		// Compressed textures are off
		glConfig.textureCompression = TC_NONE;
		VID_Printf( PRINT_ALL, "...ignoring texture compression\n" );
	}
	else if ( !old_tc && !newer_tc )
	{
		// Requesting texture compression, but no method found
		glConfig.textureCompression = TC_NONE;
		VID_Printf( PRINT_ALL, "...no supported texture compression method found\n" );
		VID_Printf( PRINT_ALL, ".....ignoring texture compression\n" );
	}
	else
	{
		// some form of supported texture compression is avaiable, so see if the user has a preference
		if ( r_ext_preferred_tc_method->integer == TC_NONE )
		{
			// No preference, so pick the best
			if ( newer_tc )
			{
				VID_Printf( PRINT_ALL, "...no tc preference specified\n" );
				VID_Printf( PRINT_ALL, ".....using GL_EXT_texture_compression_s3tc\n" );
				glConfig.textureCompression = TC_S3TC_DXT;
			}
			else
			{
				VID_Printf( PRINT_ALL, "...no tc preference specified\n" );
				VID_Printf( PRINT_ALL, ".....using GL_S3_s3tc\n" );
				glConfig.textureCompression = TC_S3TC;
			}
		}
		else
		{
			// User has specified a preference, now see if this request can be honored
			if ( old_tc && newer_tc )
			{
				// both are avaiable, so we can use the desired tc method
				if ( r_ext_preferred_tc_method->integer == TC_S3TC )
				{
					VID_Printf( PRINT_ALL, "...using preferred tc method, GL_S3_s3tc\n" );
					glConfig.textureCompression = TC_S3TC;
				}
				else
				{
					VID_Printf( PRINT_ALL, "...using preferred tc method, GL_EXT_texture_compression_s3tc\n" );
					glConfig.textureCompression = TC_S3TC_DXT;
				}
			}
			else
			{
				// Both methods are not available, so this gets trickier
				if ( r_ext_preferred_tc_method->integer == TC_S3TC )
				{
					// Preferring to user older compression
					if ( old_tc )
					{
						VID_Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
						glConfig.textureCompression = TC_S3TC;
					}
					else
					{
						// Drat, preference can't be honored 
						VID_Printf( PRINT_ALL, "...preferred tc method, GL_S3_s3tc not available\n" );
						VID_Printf( PRINT_ALL, ".....falling back to GL_EXT_texture_compression_s3tc\n" );
						glConfig.textureCompression = TC_S3TC_DXT;
					}
				}
				else
				{
					// Preferring to user newer compression
					if ( newer_tc )
					{
						VID_Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
						glConfig.textureCompression = TC_S3TC_DXT;
					}
					else
					{
						// Drat, preference can't be honored 
						VID_Printf( PRINT_ALL, "...preferred tc method, GL_EXT_texture_compression_s3tc not available\n" );
						VID_Printf( PRINT_ALL, ".....falling back to GL_S3_s3tc\n" );
						glConfig.textureCompression = TC_S3TC;
					}
				}
			}
		}
	}
}

/*
** R_InitExtensions
*/
static void R_InitExtensions( void )
{
	if ( !r_allowExtensions->integer )
	{
		VID_Printf( PRINT_ALL, "*** IGNORING OPENGL EXTENSIONS ***\n" );
		g_bDynamicGlowSupported = false;
		Cvar_Set( "r_DynamicGlow","0" );
		return;
	}

	VID_Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	// Select our tc scheme
	R_InitTextureCompression();

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "EXT_texture_env_add" ) )
	{
		if ( r_ext_texture_env_add->integer )
		{
			glConfig.textureEnvAddAvailable = qtrue;
			VID_Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
		}
		else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			VID_Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
		}
	}
	else
	{
		VID_Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}

	// GL_EXT_texture_filter_anisotropic
	glConfig.maxTextureFilterAnisotropy = 0;
	if ( strstr( glConfig.extensions_string, "EXT_texture_filter_anisotropic" ) )
	{
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF	//can't include glext.h here ... sigh
		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureFilterAnisotropy );
		Com_Printf ("...GL_EXT_texture_filter_anisotropic available\n" );

		if ( r_ext_texture_filter_anisotropic->integer>1 )
		{
			Com_Printf ("...using GL_EXT_texture_filter_anisotropic\n" );
		}
		else
		{
			Com_Printf ("...ignoring GL_EXT_texture_filter_anisotropic\n" );
		}
		Cvar_Set( "r_ext_texture_filter_anisotropic_avail", va("%f",glConfig.maxTextureFilterAnisotropy) );
		if ( r_ext_texture_filter_anisotropic->value > glConfig.maxTextureFilterAnisotropy )
		{
			Cvar_Set( "r_ext_texture_filter_anisotropic", va("%f",glConfig.maxTextureFilterAnisotropy) );
		}
	}
	else
	{
		Com_Printf ("...GL_EXT_texture_filter_anisotropic not found\n" );
		Cvar_Set( "r_ext_texture_filter_anisotropic_avail", "0" );
	}

	// GL_EXT_clamp_to_edge
	glConfig.clampToEdgeAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_edge_clamp" ) )
	{
		glConfig.clampToEdgeAvailable = qtrue;
		VID_Printf( PRINT_ALL, "...Using GL_EXT_texture_edge_clamp\n" );
	}

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( strstr( glConfig.extensions_string, "GL_ARB_multitexture" )  )
	{
		if ( r_ext_multitexture->integer )
		{
			qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) GLimp_GetProcAddress( "glMultiTexCoord2fARB" );
			qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) GLimp_GetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) GLimp_GetProcAddress( "glClientActiveTextureARB" );

			if ( qglActiveTextureARB )
			{
				qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.maxActiveTextures );

				if ( glConfig.maxActiveTextures > 1 )
				{
					VID_Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
				}
				else
				{
					qglMultiTexCoord2fARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					VID_Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
				}
			}
		}
		else
		{
			VID_Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
		}
	}
	else
	{
		VID_Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );
	}

	// GL_EXT_compiled_vertex_array
	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;
	if ( strstr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) )
	{
		if ( r_ext_compiled_vertex_array->integer )
		{
			VID_Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
			qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) ) GLimp_GetProcAddress( "glLockArraysEXT" );
			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) GLimp_GetProcAddress( "glUnlockArraysEXT" );
			if (!qglLockArraysEXT || !qglUnlockArraysEXT) {
				Com_Error (ERR_FATAL, "bad getprocaddress");
			}
		}
		else
		{
			VID_Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
		}
	}
	else
	{
		VID_Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
	}

	// GL_EXT_point_parameters
	qglPointParameterfEXT = NULL;
	qglPointParameterfvEXT = NULL;
	if ( strstr( glConfig.extensions_string, "GL_EXT_point_parameters" ) )
	{
		if ( r_ext_point_parameters->integer )
		{
			qglPointParameterfEXT = ( void ( APIENTRY * )( GLenum, GLfloat) ) GLimp_GetProcAddress( "glPointParameterfEXT" );
			qglPointParameterfvEXT = ( void ( APIENTRY * )( GLenum, GLfloat *) ) GLimp_GetProcAddress( "glPointParameterfvEXT" );
			if (!qglPointParameterfEXT || !qglPointParameterfvEXT) 
			{
				VID_Printf( ERR_FATAL, "Bad GetProcAddress for GL_EXT_point_parameters");
			}
			VID_Printf( PRINT_ALL, "...using GL_EXT_point_parameters\n" );
		}
		else
		{
			VID_Printf( PRINT_ALL, "...ignoring GL_EXT_point_parameters\n" );
		}
	}
	else
	{
		VID_Printf( PRINT_ALL, "...GL_EXT_point_parameters not found\n" );
	}

	// GL_NV_point_sprite
	qglPointParameteriNV = NULL;
	qglPointParameterivNV = NULL;
	if ( strstr( glConfig.extensions_string, "GL_NV_point_sprite" ) )
	{
		if ( r_ext_nv_point_sprite->integer )
		{
			qglPointParameteriNV = ( void ( APIENTRY * )( GLenum, GLint) ) GLimp_GetProcAddress( "glPointParameteriNV" );
			qglPointParameterivNV = ( void ( APIENTRY * )( GLenum, const GLint *) ) GLimp_GetProcAddress( "glPointParameterivNV" );
			if (!qglPointParameteriNV || !qglPointParameterivNV) 
			{
				VID_Printf( ERR_FATAL, "Bad GetProcAddress for GL_NV_point_sprite");
			}
			VID_Printf( PRINT_ALL, "...using GL_NV_point_sprite\n" );
		}
		else
		{
			VID_Printf( PRINT_ALL,  "...ignoring GL_NV_point_sprite\n" );
		}
	}
	else
	{
		VID_Printf( PRINT_ALL, "...GL_NV_point_sprite not found\n" );
	}

	bool bNVRegisterCombiners = false;
	// Register Combiners.
	if ( strstr( glConfig.extensions_string, "GL_NV_register_combiners" ) )
	{
		// NOTE: This extension requires multitexture support (over 2 units).
		if ( glConfig.maxActiveTextures >= 2 )
		{
			bNVRegisterCombiners = true;
			// Register Combiners function pointer address load.	- AReis
			// NOTE: VV guys will _definetly_ not be able to use regcoms. Pixel Shaders are just as good though :-)
			// NOTE: Also, this is an nVidia specific extension (of course), so fragment shaders would serve the same purpose
			// if we needed some kind of fragment/pixel manipulation support.
			qglCombinerParameterfvNV = ( PFNGLCOMBINERPARAMETERFVNV ) GLimp_GetProcAddress( "glCombinerParameterfvNV" );
			qglCombinerParameterivNV = ( PFNGLCOMBINERPARAMETERIVNV ) GLimp_GetProcAddress( "glCombinerParameterivNV" );
			qglCombinerParameterfNV = ( PFNGLCOMBINERPARAMETERFNV ) GLimp_GetProcAddress( "glCombinerParameterfNV" );
			qglCombinerParameteriNV = ( PFNGLCOMBINERPARAMETERINV ) GLimp_GetProcAddress( "glCombinerParameteriNV" );
			qglCombinerInputNV = ( PFNGLCOMBINERINPUTNV ) GLimp_GetProcAddress( "glCombinerInputNV" );
			qglCombinerOutputNV = ( PFNGLCOMBINEROUTPUTNV ) GLimp_GetProcAddress( "glCombinerOutputNV" );
			qglFinalCombinerInputNV = ( PFNGLFINALCOMBINERINPUTNV ) GLimp_GetProcAddress( "glFinalCombinerInputNV" );
			qglGetCombinerInputParameterfvNV	= ( PFNGLGETCOMBINERINPUTPARAMETERFVNV ) GLimp_GetProcAddress( "glGetCombinerInputParameterfvNV" );
			qglGetCombinerInputParameterivNV	= ( PFNGLGETCOMBINERINPUTPARAMETERIVNV ) GLimp_GetProcAddress( "glGetCombinerInputParameterivNV" );
			qglGetCombinerOutputParameterfvNV = ( PFNGLGETCOMBINEROUTPUTPARAMETERFVNV ) GLimp_GetProcAddress( "glGetCombinerOutputParameterfvNV" );
			qglGetCombinerOutputParameterivNV = ( PFNGLGETCOMBINEROUTPUTPARAMETERIVNV ) GLimp_GetProcAddress( "glGetCombinerOutputParameterivNV" );
			qglGetFinalCombinerInputParameterfvNV = ( PFNGLGETFINALCOMBINERINPUTPARAMETERFVNV ) GLimp_GetProcAddress( "glGetFinalCombinerInputParameterfvNV" );
			qglGetFinalCombinerInputParameterivNV = ( PFNGLGETFINALCOMBINERINPUTPARAMETERIVNV ) GLimp_GetProcAddress( "glGetFinalCombinerInputParameterivNV" );

			// Validate the functions we need.
			if ( !qglCombinerParameterfvNV || !qglCombinerParameterivNV || !qglCombinerParameterfNV || !qglCombinerParameteriNV || !qglCombinerInputNV ||
				 !qglCombinerOutputNV || !qglFinalCombinerInputNV || !qglGetCombinerInputParameterfvNV || !qglGetCombinerInputParameterivNV ||
				 !qglGetCombinerOutputParameterfvNV || !qglGetCombinerOutputParameterivNV || !qglGetFinalCombinerInputParameterfvNV || !qglGetFinalCombinerInputParameterivNV )
			{
				bNVRegisterCombiners = false;
				qglCombinerParameterfvNV = NULL;
				qglCombinerParameteriNV = NULL;
				Com_Printf ("...GL_NV_register_combiners failed\n" );
			}
		}
		else
		{
			bNVRegisterCombiners = false;
			Com_Printf ("...ignoring GL_NV_register_combiners\n" );
		}
	}
	else
	{
		bNVRegisterCombiners = false;
		Com_Printf ("...GL_NV_register_combiners not found\n" );
	}

	// NOTE: Vertex and Fragment Programs are very dependant on each other - this is actually a
	// good thing! So, just check to see which we support (one or the other) and load the shared
	// function pointers. ARB rocks!

	// Vertex Programs.
	bool bARBVertexProgram = false;
	if ( strstr( glConfig.extensions_string, "GL_ARB_vertex_program" ) )
	{
		bARBVertexProgram = true;
	}
	else
	{
		bARBVertexProgram = false;
		Com_Printf ("...GL_ARB_vertex_program not found\n" );
	}

	bool bARBFragmentProgram = false;
	// Fragment Programs.
	if ( strstr( glConfig.extensions_string, "GL_ARB_fragment_program" ) )
	{
		bARBFragmentProgram = true;
	}
	else
	{
		bARBFragmentProgram = false;
		Com_Printf ("...GL_ARB_fragment_program not found\n" );
	}

	// If we support one or the other, load the shared function pointers.
	if ( bARBVertexProgram || bARBFragmentProgram )
	{
		qglProgramStringARB					= (PFNGLPROGRAMSTRINGARBPROC)  GLimp_GetProcAddress("glProgramStringARB");
		qglBindProgramARB					= (PFNGLBINDPROGRAMARBPROC)    GLimp_GetProcAddress("glBindProgramARB");
		qglDeleteProgramsARB				= (PFNGLDELETEPROGRAMSARBPROC) GLimp_GetProcAddress("glDeleteProgramsARB");
		qglGenProgramsARB					= (PFNGLGENPROGRAMSARBPROC)    GLimp_GetProcAddress("glGenProgramsARB");
		qglProgramEnvParameter4dARB			= (PFNGLPROGRAMENVPARAMETER4DARBPROC)    GLimp_GetProcAddress("glProgramEnvParameter4dARB");
		qglProgramEnvParameter4dvARB		= (PFNGLPROGRAMENVPARAMETER4DVARBPROC)   GLimp_GetProcAddress("glProgramEnvParameter4dvARB");
		qglProgramEnvParameter4fARB			= (PFNGLPROGRAMENVPARAMETER4FARBPROC)    GLimp_GetProcAddress("glProgramEnvParameter4fARB");
		qglProgramEnvParameter4fvARB		= (PFNGLPROGRAMENVPARAMETER4FVARBPROC)   GLimp_GetProcAddress("glProgramEnvParameter4fvARB");
		qglProgramLocalParameter4dARB		= (PFNGLPROGRAMLOCALPARAMETER4DARBPROC)  GLimp_GetProcAddress("glProgramLocalParameter4dARB");
		qglProgramLocalParameter4dvARB		= (PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) GLimp_GetProcAddress("glProgramLocalParameter4dvARB");
		qglProgramLocalParameter4fARB		= (PFNGLPROGRAMLOCALPARAMETER4FARBPROC)  GLimp_GetProcAddress("glProgramLocalParameter4fARB");
		qglProgramLocalParameter4fvARB		= (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) GLimp_GetProcAddress("glProgramLocalParameter4fvARB");
		qglGetProgramEnvParameterdvARB		= (PFNGLGETPROGRAMENVPARAMETERDVARBPROC) GLimp_GetProcAddress("glGetProgramEnvParameterdvARB");
		qglGetProgramEnvParameterfvARB		= (PFNGLGETPROGRAMENVPARAMETERFVARBPROC) GLimp_GetProcAddress("glGetProgramEnvParameterfvARB");
		qglGetProgramLocalParameterdvARB	= (PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) GLimp_GetProcAddress("glGetProgramLocalParameterdvARB");
		qglGetProgramLocalParameterfvARB	= (PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) GLimp_GetProcAddress("glGetProgramLocalParameterfvARB");
		qglGetProgramivARB					= (PFNGLGETPROGRAMIVARBPROC)     GLimp_GetProcAddress("glGetProgramivARB");
		qglGetProgramStringARB				= (PFNGLGETPROGRAMSTRINGARBPROC) GLimp_GetProcAddress("glGetProgramStringARB");
		qglIsProgramARB						= (PFNGLISPROGRAMARBPROC)        GLimp_GetProcAddress("glIsProgramARB");

		// Validate the functions we need.
		if ( !qglProgramStringARB || !qglBindProgramARB || !qglDeleteProgramsARB || !qglGenProgramsARB ||
			 !qglProgramEnvParameter4dARB || !qglProgramEnvParameter4dvARB || !qglProgramEnvParameter4fARB ||
             !qglProgramEnvParameter4fvARB || !qglProgramLocalParameter4dARB || !qglProgramLocalParameter4dvARB ||
             !qglProgramLocalParameter4fARB || !qglProgramLocalParameter4fvARB || !qglGetProgramEnvParameterdvARB ||
             !qglGetProgramEnvParameterfvARB || !qglGetProgramLocalParameterdvARB || !qglGetProgramLocalParameterfvARB ||
             !qglGetProgramivARB || !qglGetProgramStringARB || !qglIsProgramARB )
		{
			bARBVertexProgram = false;
			bARBFragmentProgram = false;
			qglGenProgramsARB = NULL;	//clear ptrs that get checked
			qglProgramEnvParameter4fARB = NULL;
			Com_Printf ("...ignoring GL_ARB_vertex_program\n" );
			Com_Printf ("...ignoring GL_ARB_fragment_program\n" );
		}
	}

	// Figure out which texture rectangle extension to use.
	// TOTAL HACK!!! This will need to be fixed.
	// FIXMEFIXMEFIXME!
	bool bTexRectSupported = false;
	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_rectangle" ) )
	{
		g_bTextureRectangleHack = true;
		bTexRectSupported = true;
	}
	else if ( strstr( glConfig.extensions_string, "GL_NV_texture_rectangle" ) )
	{
		g_bTextureRectangleHack = false;
		bTexRectSupported = true;
	}
	
	// Find out how many general combiners they have.
	#define GL_MAX_GENERAL_COMBINERS_NV       0x854D
	GLint iNumGeneralCombiners = 0;
	qglGetIntegerv( GL_MAX_GENERAL_COMBINERS_NV, &iNumGeneralCombiners );

	// Only allow dynamic glows/flares if they have the hardware
	if ( bTexRectSupported && bARBVertexProgram && qglActiveTextureARB && glConfig.maxActiveTextures >= 4 &&
		( ( bNVRegisterCombiners && iNumGeneralCombiners >= 2 ) || bARBFragmentProgram ) )
	{
		g_bDynamicGlowSupported = true;
		// this would overwrite any achived setting gwg
		// Cvar_Set( "r_DynamicGlow", "1" );
	}
	else
	{
		g_bDynamicGlowSupported = false;
		Cvar_Set( "r_DynamicGlow","0" );
	}
}

/*
** R_InitGLConfig
**
** Called by the platform layer when the context exists and the qgl functions are bound.
*/
void R_InitGLConfig( void )
{
	char	buf[MAX_STRING_CHARS];
	cvar_t *lastValidRenderer = Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );

	// get our config strings
	glConfig.vendor_string = (const char *) qglGetString (GL_VENDOR);
	glConfig.renderer_string = (const char *) qglGetString (GL_RENDERER);
	glConfig.version_string = (const char *) qglGetString (GL_VERSION);
	glConfig.extensions_string = (const char *) qglGetString (GL_EXTENSIONS);

	if (!glConfig.vendor_string || !glConfig.renderer_string || !glConfig.version_string || !glConfig.extensions_string)
	{
		Com_Error( ERR_FATAL, "R_InitGLConfig() - Invalid GL Driver\n" );
	}

	// OpenGL driver constants
	qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize );
	// stubbed or broken drivers may have reported 0...
	if ( glConfig.maxTextureSize <= 0 ) 
	{
		glConfig.maxTextureSize = 0;
	}

	//
	// chipset specific configuration
	//
	Q_strncpyz( buf, glConfig.renderer_string, sizeof(buf) );
	Q_strlwr( buf );

	//
	// NOTE: if changing cvars, do it within this block.  This allows them
	// to be overridden when testing driver fixes, etc. but only sets
	// them to their default state when the hardware is first installed/run.
	//
	if ( Q_stricmp( lastValidRenderer->string, glConfig.renderer_string ) )
	{
		if (Sys_LowPhysicalMemory())
		{
			Cvar_Set("s_khz", "11");// this will get called before S_Init
			Cvar_Set("cg_VariantSoundCap", "2");
			Cvar_Set("s_allowDynamicMusic","0");
		}
		//reset to defaults
		Cvar_Set( "r_picmip", "1" );
		
		// Savage3D and Savage4 should always have trilinear enabled
		if ( strstr( buf, "savage3d" ) || strstr( buf, "s3 savage4" ) || strstr( buf, "geforce" ) || strstr( buf, "quadro" ) )
		{
			Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
		}
		else
		{
			Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
		}

		if ( strstr( buf, "kyro" ) )	
		{
			Cvar_Set( "r_ext_texture_filter_anisotropic", "0");	//KYROs have it avail, but suck at it!
			Cvar_Set( "r_ext_preferred_tc_method", "1");			//(Use DXT1 instead of DXT5 - same quality but much better performance on KYRO)
		}

		if ( strstr( buf, "geforce2" ) )	
		{
			Cvar_Set( "cg_renderToTextureFX", "0");	// slow to zero bug fix
		}

		if ( strstr( buf, "radeon 9000" ) )	
		{
			Cvar_Set( "cg_renderToTextureFX", "0");	// white texture bug
		}

		R_InitExtensions();	//get the values for test below
		//this must be a really sucky card!
		if ( (glConfig.textureCompression == TC_NONE) || (glConfig.maxActiveTextures < 2)  || (glConfig.maxTextureSize <= 512) )
		{
			Cvar_Set( "r_picmip", "2");
			Cvar_Set( "r_colorbits", "16");
			Cvar_Set( "r_texturebits", "16");
			Cvar_Set( "r_mode", "3");	//force 640
			Cmd_ExecuteString ("exec low.cfg\n");	//get the rest which can be pulled in after init
		}
	}
	
	Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );
	R_InitExtensions();
}
