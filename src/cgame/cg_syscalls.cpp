// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

//#include "cg_local.h"

// The cgame side of the typed interface to the engine
// (docs/decisions/0013-typed-cgame-interface.md). The cgi_* functions below are what
// cgame code calls; each forwards to the function the engine supplied.

cgame_import_t	cgi;

void	cgi_Printf( const char *fmt ) {
	cgi.Printf( fmt );
}

void	cgi_Error( const char *fmt ) {
	cgi.Error( fmt );
}

int		cgi_Milliseconds( void ) {
	return cgi.Milliseconds(); 
}

void	cgi_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags ) {
	cgi.Cvar_Register( vmCvar, varName, defaultValue, flags );
}

void	cgi_Cvar_Update( vmCvar_t *vmCvar ) {
	cgi.Cvar_Update( vmCvar );
}

void	cgi_Cvar_Set( const char *var_name, const char *value ) {
	cgi.Cvar_Set( var_name, value );
}

int		cgi_Argc( void ) {
	return cgi.Argc();
}

void	cgi_Argv( int n, char *buffer, int bufferLength ) {
	cgi.Argv( n, buffer, bufferLength );
}

void	cgi_Args( char *buffer, int bufferLength ) {
	cgi.Args( buffer, bufferLength );
}

int		cgi_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return cgi.FS_FOpenFile( qpath, f, mode );
}

int	cgi_FS_Read( void *buffer, int len, fileHandle_t f ) {
	return cgi.FS_Read( buffer, len, f );
}

int	cgi_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	return cgi.FS_Write( buffer, len, f );
}

void	cgi_FS_FCloseFile( fileHandle_t f ) {
	cgi.FS_FCloseFile( f );
}

void	cgi_SendConsoleCommand( const char *text ) {
	cgi.SendConsoleCommand( text );
}

void	cgi_AddCommand( const char *cmdName ) {
	cgi.AddCommand( cmdName );
}

void	cgi_SendClientCommand( const char *s ) {
	cgi.SendClientCommand( s );
}

void	cgi_UpdateScreen( void ) {
	cgi.UpdateScreen();
}

//RMG BEGIN
void cgi_RMG_Init(int terrainID, const char *terrainInfo)
{
	cgi.RMG_Init( terrainID, terrainInfo);
}

int cgi_CM_RegisterTerrain(const char *terrainInfo)
{
	return cgi.CM_RegisterTerrain( terrainInfo);
}

void cgi_RE_InitRendererTerrain( const char *terrainInfo )
{
	cgi.RE_InitRendererTerrain( terrainInfo);
}
//RMG END

void	cgi_CM_LoadMap( const char *mapname, qboolean subBSP ) {
	cgi.CM_LoadMap( mapname, subBSP );
}

int		cgi_CM_NumInlineModels( void ) {
	return cgi.CM_NumInlineModels();
}

clipHandle_t cgi_CM_InlineModel( int index ) {
	return cgi.CM_InlineModel( index );
}

clipHandle_t cgi_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs ) {//, const int contents ) {
	return cgi.CM_TempBoxModel( mins, maxs );//, contents );
}

int		cgi_CM_PointContents( const vec3_t p, clipHandle_t model ) {
	return cgi.CM_PointContents( p, model );
}

int		cgi_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles ) {
	return cgi.CM_TransformedPointContents( p, model, origin, angles );
}

void	cgi_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask ) {
	cgi.CM_BoxTrace( results, start, end, mins, maxs, model, brushmask );
}

void	cgi_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles ) {
	cgi.CM_TransformedBoxTrace( results, start, end, mins, maxs, model, brushmask, origin, angles );
}

int		cgi_CM_MarkFragments( int numPoints, const vec3_t *points, 
				const vec3_t projection,
				int maxPoints, vec3_t pointBuffer,
				int maxFragments, markFragment_t *fragmentBuffer ) {
	return cgi.CM_MarkFragments( numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer );
}

void cgi_CM_SnapPVS(vec3_t origin,byte *buffer)
{
	cgi.CM_SnapPVS( origin,buffer);
}

void cgi_S_StopSounds( void ) 
{
	cgi.S_StopSounds();
}

void	cgi_S_StartSound( const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx ) {
	cgi.S_StartSound( origin, entityNum, entchannel, sfx );
}

void	cgi_AS_ParseSets( void ) {
	cgi.AS_ParseSets();
}

void	cgi_AS_AddPrecacheEntry( const char *name ) {
	cgi.AS_AddPrecacheEntry( name );
}

void	cgi_S_UpdateAmbientSet( const char *name, vec3_t origin ) {
	cgi.S_UpdateAmbientSet( name, origin );
}

int		cgi_S_AddLocalSet( const char *name, vec3_t listener_origin, vec3_t origin, int entID, int time ) {
	return cgi.S_AddLocalSet( name, listener_origin, origin, entID, time );
}

sfxHandle_t cgi_AS_GetBModelSound( const char *name, int stage ) {
	return cgi.AS_GetBModelSound( name, stage );
}

void	cgi_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	cgi.S_StartLocalSound( sfx, channelNum );
}

void	cgi_S_ClearLoopingSounds( void ) {
	cgi.S_ClearLoopingSounds();
}

void	cgi_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, soundChannel_t chan ) {
	cgi.S_AddLoopingSound( entityNum, origin, velocity, sfx, chan );
}

void	cgi_S_UpdateEntityPosition( int entityNum, const vec3_t origin ) {
	cgi.S_UpdateEntityPosition( entityNum, origin );
}

void	cgi_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], qboolean inwater ) {
	cgi.S_Respatialize( entityNum, origin, axis, inwater );
}

sfxHandle_t	cgi_S_RegisterSound( const char *sample ) {
	return cgi.S_RegisterSound( sample );
}

void	cgi_S_StartBackgroundTrack( const char *intro, const char *loop, qboolean bForceStart ) {
	cgi.S_StartBackgroundTrack( intro, loop, bForceStart );
}

float	cgi_S_GetSampleLength( sfxHandle_t sfx ) {
	return cgi.S_GetSampleLength( sfx);
}


void	cgi_R_LoadWorldMap( const char *mapname ) {
	cgi.R_LoadWorldMap( mapname );
}

qhandle_t cgi_R_RegisterModel( const char *name ) {
	return cgi.R_RegisterModel( name );
}

qhandle_t cgi_R_RegisterSkin( const char *name ) {
	return cgi.R_RegisterSkin( name );
}

qhandle_t cgi_R_RegisterShader( const char *name ) {
	qhandle_t hShader = cgi.R_RegisterShader( name );
	assert (hShader);
	return  hShader;
}

qhandle_t cgi_R_RegisterShaderNoMip( const char *name ) {
	return cgi.R_RegisterShaderNoMip( name );
}

qhandle_t cgi_R_RegisterFont( const char *name ) {
	return cgi.R_RegisterFont( name );
}

int cgi_R_Font_StrLenPixels(const char *text, const int iFontIndex, const float scale /*= 1.0f*/) {
	return cgi.R_Font_StrLenPixels( text, iFontIndex, scale ) ;
}

int cgi_R_Font_StrLenChars(const char *text) {
	return cgi.R_Font_StrLenChars( text ) ;
}

int cgi_R_Font_HeightPixels(const int iFontIndex, const float scale /*= 1.0f*/) {
	return cgi.R_Font_HeightPixels( iFontIndex, scale );
}

qboolean cgi_Language_IsAsian( void )
{
	return cgi.Language_IsAsian();
}

qboolean cgi_Language_UsesSpaces(void)
{
	return cgi.Language_UsesSpaces();
}

unsigned int cgi_AnyLanguage_ReadCharFromString( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation /* = NULL */ )
{
	return cgi.AnyLanguage_ReadCharFromString( psText, piAdvanceCount, pbIsTrailingPunctuation );
}

void cgi_R_Font_DrawString(int ox, int oy, const char *text, const float *rgba, const int setIndex, int iMaxPixelWidth, const float scale /*= 1.0f*/) {
	cgi.R_Font_DrawString( ox, oy, text, rgba, setIndex, iMaxPixelWidth, scale );
}

//set some properties for the draw layer for my refractive effect (here primarily for mod authors) -rww
void cgi_R_SetRefractProp(float alpha, float stretch, qboolean prepost, qboolean negate)
{
	cgi.R_SetRefractProp( alpha, stretch, prepost, negate);
}

void	cgi_R_ClearScene( void ) {
	cgi.R_ClearScene();
}

void	cgi_R_AddRefEntityToScene( const refEntity_t *re ) {
	cgi.R_AddRefEntityToScene( re );
}

qboolean	cgi_R_inPVS( vec3_t p1, vec3_t p2 )
{
	return cgi.R_inPVS( p1, p2 );
}


void	cgi_R_GetLighting( const vec3_t origin, vec3_t ambientLight, vec3_t directedLight, vec3_t ligthDir ) {
	cgi.R_GetLighting( origin, ambientLight, directedLight, ligthDir );
}

void	cgi_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts ) {
	cgi.R_AddPolyToScene( hShader, numVerts, verts );
}

void	cgi_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	cgi.R_AddLightToScene( org, intensity, r, g, b );
}

void	cgi_R_RenderScene( const refdef_t *fd ) {
	cgi.R_RenderScene( fd );
}

void	cgi_R_SetColor( const float *rgba ) {
	cgi.R_SetColor( rgba );
}

void	cgi_R_DrawStretchPic( float x, float y, float w, float h, 
							   float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	cgi.R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}

//void	cgi_R_DrawScreenShot( float x, float y, float w, float h){
//	syscall( CG_R_DRAWSCREENSHOT, x, y, w, h );
//}

void	cgi_R_ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs ) {
	cgi.R_ModelBounds( model, mins, maxs );
}

void	cgi_R_LerpTag( orientation_t *tag, qhandle_t mod, int startFrame, int endFrame, 
					   float frac, const char *tagName ) {
	cgi.R_LerpTag( tag, mod, startFrame, endFrame, frac, tagName );
}

void	cgi_R_DrawRotatePic( float x, float y, float w, float h, 
				   float s1, float t1, float s2, float t2,float a, qhandle_t hShader ) 
{
	cgi.R_DrawRotatePic( x, y, w, h, s1, t1, s2, t2, a, hShader );
}

void	cgi_R_DrawRotatePic2( float x, float y, float w, float h, 
				   float s1, float t1, float s2, float t2,float a, qhandle_t hShader ) 
{
	cgi.R_DrawRotatePic2( x, y, w, h, s1, t1, s2, t2, a, hShader );
}

//linear fogging, with settable range -rww
void cgi_R_SetRangeFog(float range)
{
	cgi.R_SetRangeFog( range);
}

void	cgi_R_LAGoggles( void )
{
	cgi.R_LAGoggles();
}

void	cgi_R_Scissor( float x, float y, float w, float h) 
{
	cgi.R_Scissor( x, y, w, h);
}

void		cgi_GetGlconfig( glconfig_t *glconfig ) {
	cgi.GetGlconfig( glconfig );
}

void		cgi_GetGameState( gameState_t *gamestate ) {
	cgi.GetGameState( gamestate );
}

void		cgi_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	cgi.GetCurrentSnapshotNumber( snapshotNumber, serverTime );
}

qboolean	cgi_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	return cgi.GetSnapshot( snapshotNumber, snapshot );
}

qboolean	cgi_GetDefaultState(int entityIndex, entityState_t *state )
{
	return cgi.GetDefaultState( entityIndex, state );
}

qboolean	cgi_GetServerCommand( int serverCommandNumber ) {
	return cgi.GetServerCommand( serverCommandNumber );
}

int			cgi_GetCurrentCmdNumber( void ) {
	return cgi.GetCurrentCmdNumber();
}

qboolean	cgi_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	return cgi.GetUserCmd( cmdNumber, ucmd );
}

void		cgi_SetUserCmdValue( int stateValue, float sensitivityScale, float mPitchOverride, float mYawOverride ) {
	cgi.SetUserCmdValue( stateValue, sensitivityScale, mPitchOverride, mYawOverride );
}

void		cgi_SetUserCmdAngles( float pitchOverride, float yawOverride, float rollOverride ) {
	cgi.SetUserCmdAngles( pitchOverride, yawOverride, rollOverride );
}
/*
Ghoul2 Insert Start
*/
// CG Specific API calls
void		trap_G2_SetGhoul2ModelIndexes(CGhoul2Info_v &ghoul2, qhandle_t *modelList, qhandle_t *skinList)
{
	cgi.G2_SetGhoul2ModelIndexes( ghoul2, modelList, skinList);
}
/*
Ghoul2 Insert End
*/

void	trap_Com_SetOrgAngles(vec3_t org,vec3_t angles)
{
	cgi.Com_SetOrgAngles( org,angles);
}

void	trap_R_GetLightStyle(int style, color4ub_t color)
{
	cgi.R_GetLightStyle( style, color);
}

void	trap_R_SetLightStyle(int style, int color)
{
	cgi.R_SetLightStyle( style, color);
}

void	cgi_R_GetBModelVerts(int bmodelIndex, vec3_t *verts, vec3_t normal )
{
	cgi.R_GetBModelVerts( bmodelIndex, verts, normal );
}

void	cgi_R_WorldEffectCommand( const char *command )
{
	cgi.R_WorldEffectCommand( command );
}

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits, const char *psAudioFile /* = NULL */) {
  return cgi.CIN_PlayCinematic( arg0, xpos, ypos, width, height, bits, psAudioFile);
}
 
// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status trap_CIN_StopCinematic(int handle) {
  return cgi.CIN_StopCinematic( handle);
}


// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status trap_CIN_RunCinematic (int handle) {
  return cgi.CIN_RunCinematic( handle);
}
 

// draws the current frame
void trap_CIN_DrawCinematic (int handle) {
  cgi.CIN_DrawCinematic( handle);
}
 

// allows you to resize the animation dynamically
void trap_CIN_SetExtents (int handle, int x, int y, int w, int h) {
  cgi.CIN_SetExtents( handle, x, y, w, h);
}

void *cgi_Z_Malloc( int size, int tag )
{
	return cgi.Z_Malloc( size,tag);
}

void cgi_Z_Free( void *ptr )
{
	cgi.Z_Free( ptr);
}

void cgi_UI_SetActive_Menu(char *name)
{
	cgi.UI_SetActive_Menu( name);
}

void cgi_UI_Menu_OpenByName(char *buf)
{
	cgi.UI_Menu_OpenByName( buf);
}

void cgi_UI_Menu_Reset(void)
{
	cgi.UI_Menu_Reset();
}

void cgi_UI_Menu_New(char *buf)
{
	cgi.UI_Menu_New( buf);
}

void cgi_UI_Parse_Int(int *value)
{
	cgi.UI_Parse_Int( value);
}

void cgi_UI_Parse_String(char *buf)
{
	cgi.UI_Parse_String( (const char **)buf);
}

void cgi_UI_Parse_Float(float *value)
{
	cgi.UI_Parse_Float( value);
}

int cgi_UI_StartParseSession(char *menuFile,char **buf)
{
	return(int) cgi.UI_StartParseSession( menuFile,buf);
}

void cgi_UI_EndParseSession(char *buf)
{
	cgi.UI_EndParseSession( buf);
}

void cgi_UI_ParseExt(char **token)
{
	cgi.UI_ParseExt( token);
}

void cgi_UI_MenuCloseAll(void)
{
	cgi.UI_MenuCloseAll();
}

void cgi_UI_MenuPaintAll(void)
{
	cgi.UI_MenuPaintAll();
}

void cgi_UI_String_Init(void)
{
	cgi.UI_String_Init();
}

int cgi_UI_GetMenuInfo(char *menuFile,int *x,int *y,int *w,int *h)
{
	return(int) cgi.UI_GetMenuInfo( menuFile,x,y,w,h);
}

int cgi_UI_GetMenuItemInfo(const char *menuFile,const char *itemName, int *x,int *y,int *w,int *h,vec4_t color,qhandle_t *background)
{
	return(int) cgi.UI_GetMenuItemInfo( menuFile,itemName,x,y,w,h,color,background);
}

int cgi_UI_GetItemText(char *menuFile,char *itemName, char* text)
{
	return(int) cgi.UI_GetItemText( menuFile,itemName,text);
}

qboolean cgi_UI_MenuHasFocus(void)
{
	return cgi.UI_MenuHasFocus();
}

void cgi_UI_SetFocusedMenuItemText(const char *itemName, const char *text)
{
	cgi.UI_SetFocusedMenuItemText( itemName, text );
}

int cgi_SP_GetStringTextString(const char *text, char *buffer, int bufferLength)
{
	return cgi.SP_GetStringTextString( text, buffer, bufferLength );
}

int cgi_EndGame(void)
{
	cgi.SendConsoleCommand( "cam_disable; disconnect\n" );//; cinematic outcast
	return 0;
}
