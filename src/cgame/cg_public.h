#ifndef _CG_PUBLIC_H
#define _CG_PUBLIC_H

#define NUM_EXPLOSION_SHADERS	8
#define NUM_EXPLOSION_FRAMES	3

#define	CMD_BACKUP			64	
#define	CMD_MASK			(CMD_BACKUP - 1)
// allow a lot of command backups for very fast systems
// multiple commands may be combined into a single packet, so this
// needs to be larger than PACKET_BACKUP


#define	MAX_ENTITIES_IN_SNAPSHOT	512

#define	SNAPFLAG_RATE_DELAYED		1		// the server withheld a packet to save bandwidth
#define	SNAPFLAG_DROPPED_COMMANDS	2		// the server lost some cmds coming from the client

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
struct snapshot_s 
{
	int				snapFlags;			// SNAPFLAG_RATE_DELAYED, SNAPFLAG_DROPPED_COMMANDS
	int				ping;

	int				serverTime;		// server time the message is valid for (in msec)

	byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	int				cmdNum;			// the next cmdNum the server is expecting
									// client side prediction should start with this cmd
	playerState_t	ps;						// complete information about the current player at this time

	int				numEntities;			// all of the entities that need to be presented
	entityState_t	entities[MAX_ENTITIES_IN_SNAPSHOT];	// at the time of this snapshot

	int				numConfigstringChanges;	// configstrings that have changed since the last
	int				configstringNum;		// acknowledged snapshot_t (which is usually NOT the previous snapshot!)

	int				numServerCommands;		// text based server commands to execute when this
	int				serverCommandSequence;	// snapshot becomes current
};

typedef snapshot_s snapshot_t;


/*
==================================================================

The interface between the main executable and the cgame half of the
game module (docs/decisions/0013-typed-cgame-interface.md).

The module exports

	cgame_export_t *GetCGameAPI( int apiVersion, cgame_import_t *import );

which returns NULL unless apiVersion is CGAME_API_VERSION.

==================================================================
*/

#define	CGAME_API_VERSION	5

/*
==================================================================

functions imported from the main executable

==================================================================
*/

typedef struct {
	void		(*Printf)( const char *fmt );
	void		(*Error)( const char *fmt );
	int			(*Milliseconds)( void );

	void		(*Cvar_Register)( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
	void		(*Cvar_Update)( vmCvar_t *vmCvar );
	void		(*Cvar_Set)( const char *var_name, const char *value );

	int			(*Argc)( void );
	void		(*Argv)( int n, char *buffer, int bufferLength );
	void		(*Args)( char *buffer, int bufferLength );

	int			(*FS_FOpenFile)( const char *qpath, fileHandle_t *f, fsMode_t mode );
	int			(*FS_Read)( void *buffer, int len, fileHandle_t f );
	int			(*FS_Write)( const void *buffer, int len, fileHandle_t f );
	void		(*FS_FCloseFile)( fileHandle_t f );

	void		(*SendConsoleCommand)( const char *text );
	void		(*AddCommand)( const char *cmdName );
	void		(*SendClientCommand)( const char *s );
	void		(*UpdateScreen)( void );

	void		(*RMG_Init)( int terrainID, const char *terrainInfo );
	int			(*CM_RegisterTerrain)( const char *terrainInfo );
	void		(*RE_InitRendererTerrain)( const char *terrainInfo );

	void		(*CM_LoadMap)( const char *mapname, qboolean subBSP );
	int			(*CM_NumInlineModels)( void );
	clipHandle_t (*CM_InlineModel)( int index );
	clipHandle_t (*CM_TempBoxModel)( const vec3_t mins, const vec3_t maxs );
	int			(*CM_PointContents)( const vec3_t p, clipHandle_t model );
	int			(*CM_TransformedPointContents)( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
	void		(*CM_BoxTrace)( trace_t *results, const vec3_t start, const vec3_t end,
							  const vec3_t mins, const vec3_t maxs,
							  clipHandle_t model, int brushmask );
	void		(*CM_TransformedBoxTrace)( trace_t *results, const vec3_t start, const vec3_t end,
							  const vec3_t mins, const vec3_t maxs,
							  clipHandle_t model, int brushmask,
							  const vec3_t origin, const vec3_t angles );
	int			(*CM_MarkFragments)( int numPoints, const vec3_t *points,
							  const vec3_t projection,
							  int maxPoints, vec3_t pointBuffer,
							  int maxFragments, markFragment_t *fragmentBuffer );
	void		(*CM_SnapPVS)( vec3_t origin, byte *buffer );

	void		(*S_StartSound)( const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
	void		(*S_StartLocalSound)( sfxHandle_t sfx, int channelNum );
	void		(*S_ClearLoopingSounds)( void );
	void		(*S_AddLoopingSound)( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, soundChannel_t chan );
	void		(*S_StopSounds)( void );
	void		(*S_UpdateEntityPosition)( int entityNum, const vec3_t origin );
	void		(*S_Respatialize)( int entityNum, const vec3_t origin, vec3_t axis[3], qboolean inwater );
	sfxHandle_t	(*S_RegisterSound)( const char *sample );
	void		(*S_StartBackgroundTrack)( const char *intro, const char *loop, qboolean bForceStart );

	void		(*R_LoadWorldMap)( const char *mapname );
	qhandle_t	(*R_RegisterModel)( const char *name );
	qhandle_t	(*R_RegisterSkin)( const char *name );
	qhandle_t	(*R_RegisterShader)( const char *name );
	qhandle_t	(*R_RegisterShaderNoMip)( const char *name );
	qhandle_t	(*R_RegisterFont)( const char *name );
	int			(*R_Font_StrLenPixels)( const char *text, const int iFontIndex, const float scale );
	int			(*R_Font_StrLenChars)( const char *text );
	int			(*R_Font_HeightPixels)( const int iFontIndex, const float scale );
	void		(*R_Font_DrawString)( int ox, int oy, const char *text, const float *rgba, const int setIndex, int iMaxPixelWidth, const float scale );
	qboolean	(*Language_IsAsian)( void );
	qboolean	(*Language_UsesSpaces)( void );
	unsigned int (*AnyLanguage_ReadCharFromString)( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation );
	void		(*R_SetRefractProp)( float alpha, float stretch, qboolean prepost, qboolean negate );
	void		(*R_ClearScene)( void );
	void		(*R_AddRefEntityToScene)( const refEntity_t *re );
	qboolean	(*R_inPVS)( vec3_t p1, vec3_t p2 );
	void		(*R_GetLighting)( const vec3_t origin, vec3_t ambientLight, vec3_t directedLight, vec3_t ligthDir );
	void		(*R_AddPolyToScene)( qhandle_t hShader, int numVerts, const polyVert_t *verts );
	void		(*R_AddLightToScene)( const vec3_t org, float intensity, float r, float g, float b );
	void		(*R_RenderScene)( const refdef_t *fd );
	void		(*R_SetColor)( const float *rgba );
	void		(*R_DrawStretchPic)( float x, float y, float w, float h,
							  float s1, float t1, float s2, float t2, qhandle_t hShader );
	void		(*R_ModelBounds)( qhandle_t model, vec3_t mins, vec3_t maxs );
	void		(*R_LerpTag)( orientation_t *tag, qhandle_t mod, int startFrame, int endFrame,
							  float frac, const char *tagName );
	void		(*R_DrawRotatePic)( float x, float y, float w, float h,
							  float s1, float t1, float s2, float t2, float a, qhandle_t hShader );
	void		(*R_DrawRotatePic2)( float x, float y, float w, float h,
							  float s1, float t1, float s2, float t2, float a, qhandle_t hShader );
	void		(*R_SetRangeFog)( float range );
	void		(*R_LAGoggles)( void );
	void		(*R_Scissor)( float x, float y, float w, float h );

	void		(*GetGlconfig)( glconfig_t *glconfig );
	void		(*GetGameState)( gameState_t *gamestate );
	void		(*GetCurrentSnapshotNumber)( int *snapshotNumber, int *serverTime );
	qboolean	(*GetSnapshot)( int snapshotNumber, snapshot_t *snapshot );
	qboolean	(*GetDefaultState)( int entityIndex, entityState_t *state );
	qboolean	(*GetServerCommand)( int serverCommandNumber );
	int			(*GetCurrentCmdNumber)( void );
	qboolean	(*GetUserCmd)( int cmdNumber, usercmd_t *ucmd );
	void		(*SetUserCmdValue)( int stateValue, float sensitivityScale, float mPitchOverride, float mYawOverride );
	void		(*SetUserCmdAngles)( float pitchOverride, float yawOverride, float rollOverride );

	void		(*S_UpdateAmbientSet)( const char *name, vec3_t origin );
	int			(*S_AddLocalSet)( const char *name, vec3_t listener_origin, vec3_t origin, int entID, int time );
	void		(*AS_ParseSets)( void );
	void		(*AS_AddPrecacheEntry)( const char *name );
	sfxHandle_t	(*AS_GetBModelSound)( const char *name, int stage );
	int			(*S_GetSampleLength)( sfxHandle_t sfx );	// in whole milliseconds
	void		(*Com_SetOrgAngles)( vec3_t org, vec3_t angles );

/*
Ghoul2 Insert Start
*/
	void		(*G2_ListBones)( CGhoul2Info *ghlInfo, int frame );
	void		(*G2_ListSurfaces)( CGhoul2Info *ghlInfo );
	qboolean	(*G2_HaveWeGhoul2Models)( CGhoul2Info_v &ghoul2 );
	void		(*G2_SetGhoul2ModelIndexes)( CGhoul2Info_v &ghoul2, qhandle_t *modelList, qhandle_t *skinList );
/*
Ghoul2 Insert End
*/

	void		(*R_GetLightStyle)( int style, color4ub_t color );
	void		(*R_SetLightStyle)( int style, int color );
	void		(*R_GetBModelVerts)( int bmodelIndex, vec3_t *verts, vec3_t normal );
	void		(*R_WorldEffectCommand)( const char *command );

	int			(*CIN_PlayCinematic)( const char *arg0, int xpos, int ypos, int width, int height, int bits, const char *psAudioFile );
	e_status	(*CIN_StopCinematic)( int handle );
	e_status	(*CIN_RunCinematic)( int handle );
	void		(*CIN_DrawCinematic)( int handle );
	void		(*CIN_SetExtents)( int handle, int x, int y, int w, int h );

	void		*(*Z_Malloc)( int size, int tag );
	void		(*Z_Free)( void *ptr );

	void		(*UI_Menu_Reset)( void );
	void		(*UI_Menu_New)( char *buf );
	void		(*UI_SetActive_Menu)( char *name );
	void		(*UI_Menu_OpenByName)( char *buf );
	void		(*UI_Parse_Int)( int *value );
	void		(*UI_Parse_String)( const char **value );
	void		(*UI_Parse_Float)( float *value );
	int			(*UI_StartParseSession)( char *menuFile, char **buf );
	void		(*UI_EndParseSession)( char *buf );
	void		(*UI_ParseExt)( char **token );
	void		(*UI_MenuPaintAll)( void );
	void		(*UI_MenuCloseAll)( void );
	void		(*UI_String_Init)( void );
	int			(*UI_GetMenuInfo)( char *menuFile, int *x, int *y, int *w, int *h );
	int			(*SP_GetStringTextString)( const char *text, char *buffer, int bufferLength );
	int			(*UI_GetItemText)( char *menuFile, char *itemName, char *text );
	int			(*UI_GetMenuItemInfo)( const char *menuFile, const char *itemName, int *x, int *y, int *w, int *h, vec4_t color, qhandle_t *background );
	qboolean	(*UI_MenuHasFocus)( void );
	void		(*UI_SetFocusedMenuItemText)( const char *itemName, const char *text );
} cgame_import_t;

/*
==================================================================

functions exported to the main executable

==================================================================
*/

typedef struct {
	void		(*Init)( int serverCommandSequence );
	void		(*Shutdown)( void );
	qboolean	(*ConsoleCommand)( void );
	void		(*DrawActiveFrame)( int serverTime, stereoFrame_t stereoView );
	int			(*CrosshairPlayer)( void );
	int			(*CameraPos)( vec3_t camerapos );
	int			(*CameraAng)( vec3_t cameraang );
/*
Ghoul2 Insert Start
*/
	void		(*ResizeG2Bolt)( boltInfo_v *bolt, int newCount );
	void		(*ResizeG2)( CGhoul2Info_v *ghoul2, int newCount );
	void		(*ResizeG2Bone)( boneInfo_v *bone, int newCount );
	void		(*ResizeG2Surface)( surfaceInfo_v *surface, int newCount );
	void		(*ResizeG2TempBone)( mdxaBone_v *tempBone, int newCount );
/*
Ghoul2 Insert End
*/
	void		(*DrawDataPadObjectives)( void );
	void		(*DrawDataPadWeapons)( void );
	void		(*DrawDataPadInventory)( void );
	void		(*DrawDataPadForcePowers)( void );
	void		(*SetDataPadForceText)( void );
	void		(*SetDataPadWeaponText)( void );
} cgame_export_t;

//----------------------------------------------

#endif // _CG_PUBLIC_H
