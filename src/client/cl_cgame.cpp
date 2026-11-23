// cl_cgame.c  -- client system interaction with client game

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"
#include "../ui/ui_shared.h"

#include "../rmg/RM_Headers.h"

	   		

#include "client.h"
#include "../sys/sys_public.h"

// the cgame half of the game module, or NULL while it is not loaded
cgame_export_t	*cge;
/*
Ghoul2 Insert Start
*/

#if !defined(G2_H_INC)
	#include "../ghoul2/G2.h"
#endif

/*
Ghoul2 Insert End
*/

//FIXME: Temp
extern void S_UpdateAmbientSet ( const char *name, vec3_t origin );
extern int S_AddLocalSet( const char *name, vec3_t listener_origin, vec3_t origin, int entID, int time );
extern void AS_ParseSets( void );
extern sfxHandle_t AS_GetBModelSound( const char *name, int stage );
extern void	AS_AddPrecacheEntry( const char *name );
extern menuDef_t *Menus_FindByName(const char *p);

extern qboolean R_inPVS( vec3_t p1, vec3_t p2 );

void UI_SetActiveMenu( const char* menuname,const char *menuID );

/*
====================
CL_GetGameState
====================
*/
void CL_GetGameState( gameState_t *gs ) {
	*gs = cl.gameState;
}

/*
====================
CL_GetGlconfig
====================
*/
void CL_GetGlconfig( glconfig_t *glconfig ) {
	*glconfig = cls.glconfig;
}


/*
====================
CL_GetUserCmd
====================
*/
qboolean CL_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if ( cmdNumber > cl.cmdNumber ) {
		Com_Error( ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber );
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if ( cmdNumber <= cl.cmdNumber - CMD_BACKUP ) {
		return qfalse;
	}

	*ucmd = cl.cmds[ cmdNumber & CMD_MASK ];

	return qtrue;
}

int CL_GetCurrentCmdNumber( void ) {
	return cl.cmdNumber;
}


/*
====================
CL_GetParseEntityState
====================
*/
/*
qboolean	CL_GetParseEntityState( int parseEntityNumber, entityState_t *state ) {
	// can't return anything that hasn't been parsed yet
	if ( parseEntityNumber >= cl.parseEntitiesNum ) {
		Com_Error( ERR_DROP, "CL_GetParseEntityState: %i >= %i",
			parseEntityNumber, cl.parseEntitiesNum );
	}

	// can't return anything that has been overwritten in the circular buffer
	if ( parseEntityNumber <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES ) {
		return qfalse;
	}

	*state = cl.parseEntities[ parseEntityNumber & ( MAX_PARSE_ENTITIES - 1 ) ];
	return qtrue;
}
*/

/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
void	CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	*snapshotNumber = cl.frame.messageNum;
	*serverTime = cl.frame.serverTime;
}

/*
====================
CL_GetSnapshot
====================
*/
qboolean	CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	clSnapshot_t	*clSnap;
	int				i, count;

	if ( snapshotNumber > cl.frame.messageNum ) {
		Com_Error( ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.frame.messageNum" );
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if ( cl.frame.messageNum - snapshotNumber >= PACKET_BACKUP ) {
		return qfalse;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.frames[snapshotNumber & PACKET_MASK];
	if ( !clSnap->valid ) {
		return qfalse;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if ( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES ) {
		return qfalse;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
	snapshot->cmdNum = clSnap->cmdNum;
	snapshot->ps = clSnap->ps;
	count = clSnap->numEntities;
	if ( count > MAX_ENTITIES_IN_SNAPSHOT ) {
		Com_DPrintf( "CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;
/*
Ghoul2 Insert Start
*/
 	for ( i = 0 ; i < count ; i++ ) 
	{

		int entNum =  ( clSnap->parseEntitiesNum + i ) & (MAX_PARSE_ENTITIES-1) ;		
		snapshot->entities[i] = cl.parseEntities[ entNum ];
	}
/*
Ghoul2 Insert End
*/


	// FIXME: configstring changes and server commands!!!

	return qtrue;
}

//bg_public.h won't cooperate in here
#define EF_PERMANENT   0x00080000

qboolean CL_GetDefaultState(int index, entityState_t *state)
{
	if (index < 0 || index >= MAX_GENTITIES)
	{
		return qfalse;
	}

	// Is this safe? I think so. But it's still ugly as sin.
	if (!(sv.svEntities[index].baseline.eFlags & EF_PERMANENT))
//	if (!(cl.entityBaselines[index].eFlags & EF_PERMANENT))
	{
		return qfalse;
	}

	*state = sv.svEntities[index].baseline;
//	*state = cl.entityBaselines[index];

	return qtrue;
}

extern float cl_mPitchOverride;
extern float cl_mYawOverride;
void CL_SetUserCmdValue( int userCmdValue, float sensitivityScale, float mPitchOverride, float mYawOverride ) {
	cl.cgameUserCmdValue = userCmdValue;
	cl.cgameSensitivity = sensitivityScale;
	cl_mPitchOverride = mPitchOverride;
	cl_mYawOverride = mYawOverride;
}

extern vec3_t cl_overriddenAngles;
extern qboolean cl_overrideAngles;
void CL_SetUserCmdAngles( float pitchOverride, float yawOverride, float rollOverride ) {
	cl_overriddenAngles[PITCH] = pitchOverride;
	cl_overriddenAngles[YAW] = yawOverride;
	cl_overriddenAngles[ROLL] = rollOverride;
	cl_overrideAngles = qtrue;
}

void CL_AddCgameCommand( const char *cmdName ) {
	Cmd_AddCommand( cmdName, NULL );
}

void CL_CgameError( const char *string ) {
	Com_Error( ERR_DROP, "%s", string );
}


/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified( void ) {
	char		*old, *s;
	int			i, index;
	char		*dup;
	gameState_t	oldGs;
	int			len;

	index = atoi( Cmd_Argv(1) );
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "configstring > MAX_CONFIGSTRINGS" );
	}
	s = Cmd_Argv(2);

	old = cl.gameState.stringData + cl.gameState.stringOffsets[ index ];
	if ( !strcmp( old, s ) ) {
		return;		// unchanged
	}

	// build the new gameState_t
	oldGs = cl.gameState;

	memset( &cl.gameState, 0, sizeof( cl.gameState ) );

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;
		
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( i == index ) {
			dup = s;
		} else {
			dup = oldGs.stringData + oldGs.stringOffsets[ i ];
		}
		if ( !dup[0] ) {
			continue;		// leave with the default empty string
		}

		len = strlen( dup );

		if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
			Com_Error( ERR_DROP, "MAX_GAMESTATE_CHARS exceeded" );
		}

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
		memcpy( cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1 );
		cl.gameState.dataCount += len + 1;
	}

	if ( index == CS_SYSTEMINFO ) {
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}

}


/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
qboolean CL_GetServerCommand( int serverCommandNumber ) {
	char	*s;
	char	*cmd;

	// if we have irretrievably lost a reliable command, drop the connection
	if ( serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS ) {
		Com_Error( ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out" );
		return qfalse;
	}

	if ( serverCommandNumber > clc.serverCommandSequence ) {
		Com_Error( ERR_DROP, "CL_GetServerCommand: requested a command not received" );
		return qfalse;
	}

	s = clc.serverCommands[ serverCommandNumber & ( MAX_RELIABLE_COMMANDS - 1 ) ];

	Com_DPrintf( "serverCommand: %i : %s\n", serverCommandNumber, s );

	Cmd_TokenizeString( s );
	cmd = Cmd_Argv(0);

	if ( !strcmp( cmd, "disconnect" ) ) {
		Com_Error (ERR_DISCONNECT,"Server disconnected\n");
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		return qtrue;
	}

	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make apropriate adjustments,
	// but we also clear the console and notify lines here
	if ( !strcmp( cmd, "clientLevelShot" ) ) {
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if ( !com_sv_running->integer ) {
			return qfalse;
		}
		// close the console
		Con_Close();
		// take a special screenshot next frame
		Cbuf_AddText( "wait ; wait ; wait ; wait ; screenshot levelshot\n" );
		return qtrue;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return qtrue;
}


/*
====================
CL_CM_LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
void CL_CM_LoadMap( const char *mapname, qboolean subBSP ) {
	int		checksum;

	CM_LoadMap( mapname, qtrue, &checksum, subBSP );
}

/*
====================
CL_ShutdonwCGame

====================
*/
void CL_ShutdownCGame( void ) {
	cls.cgameStarted = qfalse;

	if ( !cge ) {
		return;
	}
	cge->Shutdown();
	RM_ShutdownTerrain();

//	VM_Free( cgvm );
//	cgvm = NULL;
}

//RMG
CCMLandScape *CM_RegisterTerrain(const char *config, bool server);
void RE_InitRendererTerrain( const char *info );
//RMG

extern float tr_distortionAlpha; //tr_shadows.cpp
extern float tr_distortionStretch; //tr_shadows.cpp
extern qboolean tr_distortionPrePost; //tr_shadows.cpp
extern qboolean tr_distortionNegate; //tr_shadows.cpp

float g_oldRangedFog = 0.0f;
/*
====================
The functions the cgame module calls

They fill the cgame_import_t that the module receives from GetCGameAPI
(docs/decisions/0013-typed-cgame-interface.md).
====================
*/
void CM_SnapPVS(vec3_t origin,byte *buffer);

static void CGI_Printf( const char *fmt ) {
	Com_Printf( "%s", fmt );
}

static void CGI_Error( const char *fmt ) {
	Com_Error( ERR_DROP, S_COLOR_RED"%s", fmt );
}

static int CGI_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return FS_FOpenFileByMode( qpath, f, mode );
}

static int CGI_FS_Read( void *buffer, int len, fileHandle_t f ) {
	FS_Read( buffer, len, f );
	return 0;
}

static int CGI_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	FS_Write( buffer, len, f );
	return 0;
}

static void CGI_UpdateScreen( void ) {
	// this is used during lengthy level loading, so pump message loop
	Com_EventLoop();	// FIXME: if a server restarts here, BAD THINGS HAPPEN!
	SCR_UpdateScreen();
}

static void CGI_RMG_Init( int terrainID, const char *terrainInfo ) {
	/*
	if (!com_sv_running->integer)
	{	// don't do this if we are connected locally
		if (!TheRandomMissionManager)
		{
			TheRandomMissionManager = new CRMManager;
		}
		TheRandomMissionManager->SetLandScape( cmg.landScapes[args[1]] );
		TheRandomMissionManager->LoadMission(qfalse);
		TheRandomMissionManager->SpawnMission(qfalse);
		cmg.landScapes[args[1]]->UpdatePatches();
	}
	*/ //this is SP.. I guess we're always the client and server.
//	cl.mRMGChecksum = cm.landScapes[args[1]]->get_rand_seed();
	RM_CreateRandomModels(terrainID, terrainInfo);
	//cmg.landScapes[args[1]]->rand_seed(cl.mRMGChecksum);		// restore it, in case we do a vid restart
	cmg.landScape->rand_seed(cmg.landScape->get_rand_seed());
//	TheRandomMissionManager->CreateMap();
}

static int CGI_CM_RegisterTerrain( const char *terrainInfo ) {
	return CM_RegisterTerrain(terrainInfo, false)->GetTerrainId();
}

static int CGI_CM_PointContents( const vec3_t p, clipHandle_t model ) {
	return CM_PointContents( (float *)p, model );
}

static int CGI_CM_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer ) {
	return re.MarkFragments( numPoints, (float(*)[3]) points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer );
}

static void CGI_S_StartSound( const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx ) {
	// stops an ERR_DROP internally if called illegally from game side, but note that it also gets here 
	//	legally during level start where normally the internal s_soundStarted check would return. So ok to hit this.
	if (!cls.cgameStarted){
		return;	
	}
	S_StartSound( (float *) origin, entityNum, (soundChannel_t)entchannel, sfx );
}

static void CGI_S_UpdateAmbientSet( const char *name, vec3_t origin ) {
	// stops an ERR_DROP internally if called illegally from game side, but note that it also gets here 
	//	legally during level start where normally the internal s_soundStarted check would return. So ok to hit this.
	if (!cls.cgameStarted){
		return;
	}
	S_UpdateAmbientSet( name, origin );
}

static void CGI_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	// stops an ERR_DROP internally if called illegally from game side, but note that it also gets here 
	//	legally during level start where normally the internal s_soundStarted check would return. So ok to hit this.
	if (!cls.cgameStarted){
		return;
	}
	S_StartLocalSound( sfx, channelNum );
}

static void CGI_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, soundChannel_t chan ) {
	// stops an ERR_DROP internally if called illegally from game side, but note that it also gets here 
	//	legally during level start where normally the internal s_soundStarted check would return. So ok to hit this.
	if (!cls.cgameStarted){
		return;
	}
	S_AddLoopingSound( entityNum, origin, velocity, sfx, chan );
}

static int CGI_S_GetSampleLength( sfxHandle_t sfx ) {
	return S_GetSampleLengthInMilliSeconds( sfx );
}

static void CGI_R_LoadWorldMap( const char *mapname ) {
	re.LoadWorld( mapname );
}

static qhandle_t CGI_R_RegisterModel( const char *name ) {
	return re.RegisterModel( name );
}

static qhandle_t CGI_R_RegisterSkin( const char *name ) {
	return re.RegisterSkin( name );
}

static qhandle_t CGI_R_RegisterShader( const char *name ) {
	return re.RegisterShader( name );
}

static qhandle_t CGI_R_RegisterShaderNoMip( const char *name ) {
	return re.RegisterShaderNoMip( name );
}

static qhandle_t CGI_R_RegisterFont( const char *name ) {
	return re.RegisterFont( name );
}

static int CGI_R_Font_StrLenPixels( const char *text, const int iFontIndex, const float scale ) {
	return re.Font_StrLenPixels( text, iFontIndex, scale );
}

static int CGI_R_Font_StrLenChars( const char *text ) {
	return re.Font_StrLenChars( text );
}

static int CGI_R_Font_HeightPixels( const int iFontIndex, const float scale ) {
	return re.Font_HeightPixels( iFontIndex, scale );
}

static void CGI_R_Font_DrawString( int ox, int oy, const char *text, const float *rgba, const int setIndex, int iMaxPixelWidth, const float scale ) {
	re.Font_DrawString( ox, oy, text, rgba, setIndex, iMaxPixelWidth, scale );
}

static qboolean CGI_Language_IsAsian( void ) {
	return re.Language_IsAsian();
}

static qboolean CGI_Language_UsesSpaces( void ) {
	return re.Language_UsesSpaces();
}

static unsigned int CGI_AnyLanguage_ReadCharFromString( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation ) {
	return re.AnyLanguage_ReadCharFromString( psText, piAdvanceCount, pbIsTrailingPunctuation );
}

static void CGI_R_SetRefractProp( float alpha, float stretch, qboolean prepost, qboolean negate ) {
	tr_distortionAlpha = alpha;
	tr_distortionStretch = stretch;
	tr_distortionPrePost = prepost;
	tr_distortionNegate = negate;
}

static void CGI_R_ClearScene( void ) {
	re.ClearScene();
}

static void CGI_R_AddRefEntityToScene( const refEntity_t *ent ) {
	re.AddRefEntityToScene( ent );
}

static qboolean CGI_R_inPVS( vec3_t p1, vec3_t p2 ) {
	return R_inPVS( p1, p2 );
}

static void CGI_R_GetLighting( const vec3_t origin, vec3_t ambientLight, vec3_t directedLight, vec3_t ligthDir ) {
	re.GetLighting( origin, ambientLight, directedLight, ligthDir );
}

static void CGI_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	re.AddPolyToScene( hShader, numVerts, verts );
}

static void CGI_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	re.AddLightToScene( org, intensity, r, g, b );
}

static void CGI_R_RenderScene( const refdef_t *fd ) {
	re.RenderScene( fd );
}

static void CGI_R_SetColor( const float *rgba ) {
	re.SetColor( rgba );
}

static void CGI_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	re.DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}

static void CGI_R_ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs ) {
	re.ModelBounds( model, mins, maxs );
}

static void CGI_R_LerpTag( orientation_t *tag, qhandle_t mod, int startFrame, int endFrame, float frac, const char *tagName ) {
	re.LerpTag( tag, mod, startFrame, endFrame, frac, tagName );
}

static void CGI_R_DrawRotatePic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, float a, qhandle_t hShader ) {
	re.DrawRotatePic( x, y, w, h, s1, t1, s2, t2, a, hShader );
}

static void CGI_R_DrawRotatePic2( float x, float y, float w, float h, float s1, float t1, float s2, float t2, float a, qhandle_t hShader ) {
	re.DrawRotatePic2( x, y, w, h, s1, t1, s2, t2, a, hShader );
}

static void CGI_R_SetRangeFog( float range ) {
	if (tr.rangedFog <= 0.0f)
	{
		g_oldRangedFog = tr.rangedFog;
	}
	tr.rangedFog = range;
	if (tr.rangedFog == 0.0f && g_oldRangedFog)
	{ //restore to previous state if applicable
		tr.rangedFog = g_oldRangedFog;
	}
}

static void CGI_R_LAGoggles( void ) {
	re.LAGoggles();
}

static void CGI_R_Scissor( float x, float y, float w, float h ) {
	re.Scissor( x, y, w, h );
}

static qboolean CGI_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	return CL_GetUserCmd( cmdNumber, ucmd );
}

/*
Ghoul2 Insert Start
*/
static void CGI_G2_ListBones( CGhoul2Info *ghlInfo, int frame ) {
	G2API_ListBones( ghlInfo, frame );
}

static void CGI_G2_ListSurfaces( CGhoul2Info *ghlInfo ) {
	G2API_ListSurfaces( ghlInfo );
}

static qboolean CGI_G2_HaveWeGhoul2Models( CGhoul2Info_v &ghoul2 ) {
	return G2API_HaveWeGhoul2Models( ghoul2 );
}

static void CGI_G2_SetGhoul2ModelIndexes( CGhoul2Info_v &ghoul2, qhandle_t *modelList, qhandle_t *skinList ) {
	G2API_SetGhoul2ModelIndexes( ghoul2, modelList, skinList );
}
/*
Ghoul2 Insert End
*/

static void CGI_R_GetLightStyle( int style, color4ub_t color ) {
	re.GetLightStyle( style, color );
}

static void CGI_R_SetLightStyle( int style, int color ) {
	re.SetLightStyle( style, color );
}

static void CGI_R_GetBModelVerts( int bmodelIndex, vec3_t *verts, vec3_t normal ) {
	re.GetBModelVerts( bmodelIndex, verts, normal );
}

static void CGI_R_WorldEffectCommand( const char *command ) {
	re.WorldEffectCommand( command );
}

static void *CGI_Z_Malloc( int size, int tag ) {
	return Z_Malloc( size, (memtag_t) tag, qfalse );
}

static void CGI_Z_Free( void *ptr ) {
	Z_Free( ptr );
}

static void CGI_UI_SetActive_Menu( char *name ) {
	UI_SetActiveMenu( name, NULL );
}

static void CGI_UI_Menu_OpenByName( char *buf ) {
	Menus_OpenByName( buf );
}

static void CGI_UI_Parse_Int( int *value ) {
	PC_ParseInt( value );
}

static void CGI_UI_Parse_String( const char **value ) {
	PC_ParseString( value );
}

static void CGI_UI_Parse_Float( float *value ) {
	PC_ParseFloat( value );
}

static int CGI_UI_StartParseSession( char *menuFile, char **buf ) {
	return PC_StartParseSession( menuFile, buf );
}

static void CGI_UI_ParseExt( char **token ) {
	*token = PC_ParseExt();
}

static int CGI_UI_GetMenuInfo( char *menuFile, int *x, int *y, int *w, int *h ) {
	menuDef_t *menu;
	int		result;

	menu = Menus_FindByName( menuFile );	// Get menu 
	if (menu)
	{
		*x = (int) menu->window.rect.x;
		*y = (int) menu->window.rect.y;
		*w = (int) menu->window.rect.w;
		*h = (int) menu->window.rect.h;
		result = qtrue;
	}
	else
	{
		result = qfalse;
	}

	return result;
}

static int CGI_UI_GetItemText( char *menuFile, char *itemName, char *text ) {
	menuDef_t *menu;
	itemDef_t *item;
	int		result;

	menu = Menus_FindByName( menuFile );	// Get menu 

	if (menu)
	{
		item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, itemName);
		if (item)
		{
			Q_strncpyz( text, item->text, 256 );
			result = qtrue;
		}
		else
		{
			result = qfalse;
		}
	}
	else
	{
		result = qfalse;
	}

	return result;
}

static int CGI_UI_GetMenuItemInfo( const char *menuFile, const char *itemName, int *x, int *y, int *w, int *h, vec4_t color, qhandle_t *background ) {
	menuDef_t *menu;
	itemDef_t *item;
	int		result;

	menu = Menus_FindByName( menuFile );	// Get menu 

	if (menu)
	{
		item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, itemName);
		if (item)
		{
			*x = (int) item->window.rect.x;
			*y = (int) item->window.rect.y;
			*w = (int) item->window.rect.w;
			*h = (int) item->window.rect.h;

			if (!color)
			{
				return qfalse;
			}

			color[0] = (float) item->window.foreColor[0];
			color[1] = (float) item->window.foreColor[1];
			color[2] = (float) item->window.foreColor[2];
			color[3] = (float) item->window.foreColor[3];
			if (!background)
			{
				return qfalse;
			}
			*background = item->window.background;

			result = qtrue;
		}
		else
		{
			result = qfalse;
		}
	}
	else
	{
		result = qfalse;
	}

	return result;
}

void Menu_SetItemText(const menuDef_t *menu,const char *itemName, const char *text);

static qboolean CGI_UI_MenuHasFocus( void ) {
	return Menu_GetFocused() != NULL;
}

static void CGI_UI_SetFocusedMenuItemText( const char *itemName, const char *text ) {
	Menu_SetItemText( Menu_GetFocused(), itemName, text );
}

static int CGI_SP_GetStringTextString( const char *reference, char *buffer, int bufferLength ) {
	const char* text;

	assert(reference);	
	text = SE_GetString( reference );

	if (buffer)	// only if dest buffer supplied...
	{
		if ( text[0] )
		{
			Q_strncpyz( buffer, text, bufferLength );				
		}
		else 
		{
			Com_sprintf( buffer, bufferLength, "??%s", reference );
		}
	}
	return strlen(text);
}

/*
====================
CL_BindCGame

Hands the engine's functions to the cgame half of the game module and takes its
entry points. The module itself is loaded by the server (SV_InitGameProgs).
====================
*/
qboolean CL_BindCGame( void ) {
	typedef cgame_export_t *(*GetCGameAPI_t)( int apiVersion, cgame_import_t *import );
	static cgame_import_t	import;
	GetCGameAPI_t	GetCGameAPI;

	import.Printf = CGI_Printf;
	import.Error = CGI_Error;
	import.Milliseconds = Sys_Milliseconds;

	import.Cvar_Register = Cvar_Register;
	import.Cvar_Update = Cvar_Update;
	import.Cvar_Set = Cvar_Set;

	import.Argc = Cmd_Argc;
	import.Argv = Cmd_ArgvBuffer;
	import.Args = Cmd_ArgsBuffer;

	import.FS_FOpenFile = CGI_FS_FOpenFile;
	import.FS_Read = CGI_FS_Read;
	import.FS_Write = CGI_FS_Write;
	import.FS_FCloseFile = FS_FCloseFile;

	import.SendConsoleCommand = Cbuf_AddText;
	import.AddCommand = CL_AddCgameCommand;
	import.SendClientCommand = CL_AddReliableCommand;
	import.UpdateScreen = CGI_UpdateScreen;

	import.RMG_Init = CGI_RMG_Init;
	import.CM_RegisterTerrain = CGI_CM_RegisterTerrain;
	import.RE_InitRendererTerrain = RE_InitRendererTerrain;

	import.CM_LoadMap = CL_CM_LoadMap;
	import.CM_NumInlineModels = CM_NumInlineModels;
	import.CM_InlineModel = CM_InlineModel;
	import.CM_TempBoxModel = CM_TempBoxModel;
	import.CM_PointContents = CGI_CM_PointContents;
	import.CM_TransformedPointContents = CM_TransformedPointContents;
	import.CM_BoxTrace = CM_BoxTrace;
	import.CM_TransformedBoxTrace = CM_TransformedBoxTrace;
	import.CM_MarkFragments = CGI_CM_MarkFragments;
	import.CM_SnapPVS = CM_SnapPVS;

	import.S_StartSound = CGI_S_StartSound;
	import.S_StartLocalSound = CGI_S_StartLocalSound;
	import.S_ClearLoopingSounds = S_ClearLoopingSounds;
	import.S_AddLoopingSound = CGI_S_AddLoopingSound;
	import.S_StopSounds = S_StopSounds;
	import.S_UpdateEntityPosition = S_UpdateEntityPosition;
	import.S_Respatialize = S_Respatialize;
	import.S_RegisterSound = S_RegisterSound;
	import.S_StartBackgroundTrack = S_StartBackgroundTrack;

	import.R_LoadWorldMap = CGI_R_LoadWorldMap;
	import.R_RegisterModel = CGI_R_RegisterModel;
	import.R_RegisterSkin = CGI_R_RegisterSkin;
	import.R_RegisterShader = CGI_R_RegisterShader;
	import.R_RegisterShaderNoMip = CGI_R_RegisterShaderNoMip;
	import.R_RegisterFont = CGI_R_RegisterFont;
	import.R_Font_StrLenPixels = CGI_R_Font_StrLenPixels;
	import.R_Font_StrLenChars = CGI_R_Font_StrLenChars;
	import.R_Font_HeightPixels = CGI_R_Font_HeightPixels;
	import.R_Font_DrawString = CGI_R_Font_DrawString;
	import.Language_IsAsian = CGI_Language_IsAsian;
	import.Language_UsesSpaces = CGI_Language_UsesSpaces;
	import.AnyLanguage_ReadCharFromString = CGI_AnyLanguage_ReadCharFromString;
	import.R_SetRefractProp = CGI_R_SetRefractProp;
	import.R_ClearScene = CGI_R_ClearScene;
	import.R_AddRefEntityToScene = CGI_R_AddRefEntityToScene;
	import.R_inPVS = CGI_R_inPVS;
	import.R_GetLighting = CGI_R_GetLighting;
	import.R_AddPolyToScene = CGI_R_AddPolyToScene;
	import.R_AddLightToScene = CGI_R_AddLightToScene;
	import.R_RenderScene = CGI_R_RenderScene;
	import.R_SetColor = CGI_R_SetColor;
	import.R_DrawStretchPic = CGI_R_DrawStretchPic;
	import.R_ModelBounds = CGI_R_ModelBounds;
	import.R_LerpTag = CGI_R_LerpTag;
	import.R_DrawRotatePic = CGI_R_DrawRotatePic;
	import.R_DrawRotatePic2 = CGI_R_DrawRotatePic2;
	import.R_SetRangeFog = CGI_R_SetRangeFog;
	import.R_LAGoggles = CGI_R_LAGoggles;
	import.R_Scissor = CGI_R_Scissor;

	import.GetGlconfig = CL_GetGlconfig;
	import.GetGameState = CL_GetGameState;
	import.GetCurrentSnapshotNumber = CL_GetCurrentSnapshotNumber;
	import.GetSnapshot = CL_GetSnapshot;
	import.GetDefaultState = CL_GetDefaultState;
	import.GetServerCommand = CL_GetServerCommand;
	import.GetCurrentCmdNumber = CL_GetCurrentCmdNumber;
	import.GetUserCmd = CGI_GetUserCmd;
	import.SetUserCmdValue = CL_SetUserCmdValue;
	import.SetUserCmdAngles = CL_SetUserCmdAngles;

	import.S_UpdateAmbientSet = CGI_S_UpdateAmbientSet;
	import.S_AddLocalSet = S_AddLocalSet;
	import.AS_ParseSets = AS_ParseSets;
	import.AS_AddPrecacheEntry = AS_AddPrecacheEntry;
	import.AS_GetBModelSound = AS_GetBModelSound;
	import.S_GetSampleLength = CGI_S_GetSampleLength;
	import.Com_SetOrgAngles = Com_SetOrgAngles;

	import.G2_ListBones = CGI_G2_ListBones;
	import.G2_ListSurfaces = CGI_G2_ListSurfaces;
	import.G2_HaveWeGhoul2Models = CGI_G2_HaveWeGhoul2Models;
	import.G2_SetGhoul2ModelIndexes = CGI_G2_SetGhoul2ModelIndexes;

	import.R_GetLightStyle = CGI_R_GetLightStyle;
	import.R_SetLightStyle = CGI_R_SetLightStyle;
	import.R_GetBModelVerts = CGI_R_GetBModelVerts;
	import.R_WorldEffectCommand = CGI_R_WorldEffectCommand;

	import.CIN_PlayCinematic = CIN_PlayCinematic;
	import.CIN_StopCinematic = CIN_StopCinematic;
	import.CIN_RunCinematic = CIN_RunCinematic;
	import.CIN_DrawCinematic = CIN_DrawCinematic;
	import.CIN_SetExtents = CIN_SetExtents;

	import.Z_Malloc = CGI_Z_Malloc;
	import.Z_Free = CGI_Z_Free;

	import.UI_Menu_Reset = Menu_Reset;
	import.UI_Menu_New = Menu_New;
	import.UI_SetActive_Menu = CGI_UI_SetActive_Menu;
	import.UI_Menu_OpenByName = CGI_UI_Menu_OpenByName;
	import.UI_Parse_Int = CGI_UI_Parse_Int;
	import.UI_Parse_String = CGI_UI_Parse_String;
	import.UI_Parse_Float = CGI_UI_Parse_Float;
	import.UI_StartParseSession = CGI_UI_StartParseSession;
	import.UI_EndParseSession = PC_EndParseSession;
	import.UI_ParseExt = CGI_UI_ParseExt;
	import.UI_MenuPaintAll = Menu_PaintAll;
	import.UI_MenuCloseAll = Menus_CloseAll;
	import.UI_String_Init = String_Init;
	import.UI_GetMenuInfo = CGI_UI_GetMenuInfo;
	import.SP_GetStringTextString = CGI_SP_GetStringTextString;
	import.UI_GetItemText = CGI_UI_GetItemText;
	import.UI_GetMenuItemInfo = CGI_UI_GetMenuItemInfo;
	import.UI_MenuHasFocus = CGI_UI_MenuHasFocus;
	import.UI_SetFocusedMenuItemText = CGI_UI_SetFocusedMenuItemText;

	GetCGameAPI = (GetCGameAPI_t) Sys_GetGameFunction( "GetCGameAPI" );
	if ( !GetCGameAPI ) {
		return qfalse;
	}

	cge = GetCGameAPI( CGAME_API_VERSION, &import );
	if ( !cge ) {
		Com_Error( ERR_DROP, "the cgame half of the game module does not speak interface version %i", CGAME_API_VERSION );
	}
	return qtrue;
}

/*
====================
CL_UnbindCGame

The game module is about to be unloaded.
====================
*/
void CL_UnbindCGame( void ) {
	cge = NULL;
}


/*
====================
CL_InitCGame

Should only be called by CL_StartHunkUsers
====================
*/
extern qboolean Sys_LowPhysicalMemory();
void CL_InitCGame( void ) {
	const char			*info;
	const char			*mapname;
	int		t1, t2;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SERVERINFO ];
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cl.mapname, sizeof( cl.mapname ), "maps/%s.bsp", mapname );

	cls.state = CA_LOADING;

	// init for this gamestate
	cge->Init( clc.serverCommandSequence );

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	//Com_Printf( "CL_InitCGame: %5.2f seconds\n", (t2-t1)/1000.0 );
	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re.EndRegistration();

	// make sure everything is paged in
//	if (!Sys_LowPhysicalMemory()) 
	{
		Com_TouchMemory();
	}

	// clear anything that got printed
	Con_ClearNotify ();
}


/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameCommand( void ) {
	if ( cls.state != CA_ACTIVE ) {
		return qfalse;
	}

	return cge->ConsoleCommand();
}



/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering( stereoFrame_t stereo ) {
#if 0
	if ( cls.state == CA_ACTIVE ) {
		static int counter;

		if ( ++counter == 40 ) {
			VM_Debug( 2 );
		}
	}
#endif
	int timei=cl.serverTime;
	if (timei>60)
	{
		timei-=0;
	}
	G2API_SetTime(cl.serverTime,G2T_CG_TIME);
	cge->DrawActiveFrame( timei, stereo );
//	VM_Debug( 0 );
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time aproach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives, which keeps the
adjustment process framerate independent and prevents massive overadjustment
during times of significant packet loss.
=================
*/

#define	RESET_TIME	300

void CL_AdjustTimeDelta( void ) {
/*
	cl.newSnapshots = qfalse;
	// if the current time is WAY off, just correct to the current value
	if ( cls.realtime + cl.serverTimeDelta < cl.frame.serverTime - RESET_TIME 
		|| cls.realtime + cl.serverTimeDelta > cl.frame.serverTime + RESET_TIME  ) {
		cl.serverTimeDelta = cl.frame.serverTime - cls.realtime;
		cl.oldServerTime = cl.frame.serverTime;
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<RESET> " );
		}
	}

	// if any of the frames between this and the previous snapshot
	// had to be extrapolated, nudge our sense of time back a little
	if ( cl.extrapolatedSnapshot ) {
		cl.extrapolatedSnapshot = qfalse;
		cl.serverTimeDelta -= 2;
	} else {
		// otherwise, move our sense of time forward to minimize total latency
		cl.serverTimeDelta++;
	}

	if ( cl_showTimeDelta->integer ) {
		Com_Printf( "%i ", cl.serverTimeDelta );
	}
*/
	int		resetTime;
	int		newDelta;
	int		deltaDelta;

	cl.newSnapshots = qfalse;
	
	// if the current time is WAY off, just correct to the current value
	if ( com_sv_running->integer ) {
		resetTime = 100;
	} else {
		resetTime = RESET_TIME;
	}

	newDelta = cl.frame.serverTime - cls.realtime;
	deltaDelta = abs( newDelta - cl.serverTimeDelta );

	if ( deltaDelta > RESET_TIME ) {
		cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.frame.serverTime;	// FIXME: is this a problem for cgame?
		cl.serverTime = cl.frame.serverTime;
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<RESET> " );
		}
	} else if ( deltaDelta > 100 ) {
		// fast adjust, cut the difference in half
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<FAST> " );
		}
		cl.serverTimeDelta = ( cl.serverTimeDelta + newDelta ) >> 1;
	} else {
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if ( com_timescale->value == 0 || com_timescale->value == 1 ) {
			if ( cl.extrapolatedSnapshot ) {
				cl.extrapolatedSnapshot = qfalse;
				cl.serverTimeDelta -= 2;
			} else {
				// otherwise, move our sense of time forward to minimize total latency
				cl.serverTimeDelta++;
			}
		}
	}

	if ( cl_showTimeDelta->integer ) {
		Com_Printf( "%i ", cl.serverTimeDelta );
	}
}

// The UI sets this to a non-negative value, when the level select cheat is used
// to start a level. It will be the rank of neutral force powers that we should have.
// All others will be rank 3. =)
//int levelSelectCheat = -1;

/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot( void ) {

	RE_RegisterMedia_LevelLoadEnd();

	cls.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.serverTimeDelta = cl.frame.serverTime - cls.realtime;
	cl.oldServerTime = cl.frame.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if ( cl_activeAction->string[0] ) {
		Cbuf_AddText( cl_activeAction->string );
		Cvar_Set( "activeAction", "" );
	}
	
	Sys_BeginProfiling();


	// Goodies?
	cvar_t*	levelSelectCheat	= Cvar_Get("levelSelectCheat", "-1", CVAR_SAVEGAME);
	if(  levelSelectCheat->integer >= 0 )
	{
		int n = levelSelectCheat->integer;
		//levelSelectCheat = -1;

		// Set all Light powers to level 3:
		Cbuf_ExecuteText( EXEC_APPEND, "setForceHeal 3\nsetMindTrick 3\nsetForceProtect 3\nsetForceAbsorb 3\n" );
		// Set all Dark powers to level 3:
		Cbuf_ExecuteText( EXEC_APPEND, "setForceGrip 3\nsetForceLightning 3\nsetForceRage 3\nsetForceDrain 3\n" );

		// Special case for yavin1b - we need saber powers, but no other neutral
		if( n == 0 )
			Cbuf_ExecuteText( EXEC_APPEND, "setSaberOffense 1\nsetSaberDefense 1\nsetSaberThrow 1\n" );
		else
			Cbuf_ExecuteText( EXEC_APPEND, va("setSaberOffense %d\nsetSaberDefense %d\nsetSaberThrow %d\n", n, n, n) );

		// Set all remaining neutral powers to cheat level:
		Cbuf_ExecuteText( EXEC_APPEND, va("setForcePush %d\nsetForcePull %d\nsetForceSpeed %d\nsetForceJump %d\nsetForceSight %d\n", n, n, n, n, n) );
	}
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime( void ) {

	// getting a valid frame message ends the connection process
	if ( cls.state != CA_ACTIVE ) {
		if ( cls.state != CA_PRIMED ) {
			return;
		}
		if ( cl.newSnapshots ) {
			cl.newSnapshots = qfalse;
			CL_FirstSnapshot();
		}

		if ( cls.state != CA_ACTIVE ) {
			return;
		}
	}	

	// if we have gotten to this point, cl.frame is guaranteed to be valid
	if ( !cl.frame.valid ) {
		Com_Error( ERR_DROP, "CL_SetCGameTime: !cl.snap.valid" );
	}

	// allow pause in single player
	static int pauseStart = 0;
	static int pauseServerTimeDelta;
	if ( sv_paused->integer && cl_paused->integer && com_sv_running->integer ) {
		// paused
		if(!pauseStart) {
			pauseServerTimeDelta = cl.serverTimeDelta;
			pauseStart = cls.realtime;
		}
		return;
	}

	if ( cl.frame.serverTime < cl.oldFrameServerTime ) {
		Com_Error( ERR_DROP, "cl.frame.serverTime < cl.oldFrameServerTime" );
	}
	cl.oldFrameServerTime = cl.frame.serverTime;


	// get our current view of time

	// cl_timeNudge is a user adjustable cvar that allows more
	// or less latency to be added in the interest of better 
	// smoothness or better responsiveness.
	cl.serverTime = cls.realtime + cl.serverTimeDelta - cl_timeNudge->integer;

	//If we were paused, subtract out pause time once since we won't yet
	//have an updated server time.  Otherwise the camera gets confused when
	//time leaps forward for a frame and then resets back to normal.
	if(pauseStart && pauseServerTimeDelta == cl.serverTimeDelta) {
		cl.serverTime -= cls.realtime - pauseStart;
	} else {
		pauseStart = 0;
		pauseServerTimeDelta = 0;
	}

	// guarantee that time will never flow backwards, even if
	// serverTimeDelta made an adjustment or cl_timeNudge was changed
	if ( cl.serverTime < cl.oldServerTime ) {
		cl.serverTime = cl.oldServerTime;
	}
	cl.oldServerTime = cl.serverTime;

	// note if we are almost past the latest frame (without timeNudge),
	// so we will try and adjust back a bit when the next snapshot arrives
	if ( cls.realtime + cl.serverTimeDelta >= cl.frame.serverTime - 5 ) {
		cl.extrapolatedSnapshot = qtrue;
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if ( cl.newSnapshots ) {
		CL_AdjustTimeDelta();
	}
}

