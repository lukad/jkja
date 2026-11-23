// Filename:-	g_savegame.cpp
//
// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"


#include "../icarus/IcarusInterface.h"
#include "Q3_Interface.h"
#include "g_local.h"
#include "objectives.h"
#include "../cgame/cg_camera.h"
#include "../qcommon/sstring.h"
#include "g_savetables.h"
#include "g_vehicles.h"

#include <string>
#include <vector>

extern void OBJ_LoadTacticalInfo(void);

extern void G_LoadSave_WriteMiscData(void);
extern void G_LoadSave_ReadMiscData(void);
extern void G_ReloadSaberData( gentity_t *ent );
extern void FX_Read( void );
extern void FX_Write( void );

/////////// char * /////////////
//
//

static char *GetStringPtr(const char *sString, char *psOriginal/*may be NULL*/)
{
	if (psOriginal && gi.bIsFromZone(psOriginal, TAG_G_ALLOC)) {
		if (!strcmp(psOriginal,sString))
		{//it's a legal ptr and they're the same so let's just reuse it instead of free/alloc
			return psOriginal;
		}
		gi.Free(psOriginal);
	}

	return G_NewString(sString);
}
//
//
////////////////////////////////




/////////// gentity_t * ////////
//
//
static int GetGEntityNum(gentity_t* ent)
{
	assert( ent != (gentity_t *) 0xcdcdcdcd);

	if (ent == NULL)
	{
		return -1;
	}

	// note that I now validate the return value (to avoid triggering asserts on re-load) because of the
	//	way that the level_locals_t alertEvents struct contains a count of which ones are valid, so I'm guessing
	//	that some of them aren't (valid)...
	//
	int iReturnIndex = ent - g_entities;

	if (iReturnIndex < 0 || iReturnIndex >= MAX_GENTITIES)
	{	
		iReturnIndex = -1;	// will get a NULL ptr on reload
	}
	return iReturnIndex;
}

static gentity_t *GetGEntityPtr(int iEntNum)
{
	if (iEntNum == -1)
	{
		return NULL;
	}
	assert(iEntNum >= 0);
	assert(iEntNum < MAX_GENTITIES);
	return (g_entities + iEntNum);
}
//
//
////////////////////////////////



static int GetGroupNumber(AIGroupInfo_t *pGroup)
{
	assert( pGroup != (AIGroupInfo_t *) 0xcdcdcdcd);

	if (pGroup == NULL)
	{
		return -1;
	}

	int iReturnIndex = pGroup - level.groups;
	if (iReturnIndex < 0 || iReturnIndex >= (sizeof(level.groups) / sizeof(level.groups[0])) )
	{	
		iReturnIndex = -1;	// will get a NULL ptr on reload
	}
	return iReturnIndex;
}

static AIGroupInfo_t *GetGroupPtr(int iGroupNum)
{
	if (iGroupNum == -1)
	{
		return NULL;
	}
	assert(iGroupNum >= 0);
	assert(iGroupNum < (sizeof(level.groups) / sizeof(level.groups[0])));
	return (level.groups + iGroupNum);
}



/////////// gclient_t * ////////
//
//
static int GetGClientNum(gclient_t *c, gentity_t *ent)
{
	// unfortunately, I now need to see if this is a 'real' client (and therefore resolve to an enum), or
	//	whether it's one of the NPC or SP_misc_weapon_shooter
	//
	assert(c != (gclient_t *)0xcdcdcdcd);

	if (c == NULL)
	{
		return -1;
	}
	
	if (ent->s.number < MAX_CLIENTS)
	{	// regular client...
		return (c - level.clients);	
	}
	else
	{	// this must be an NPC or weapon_shooter, so mark it as special...
		return -2;	// yeuch, but distinguishes it from a valid 0 index, or -1 for client==NULL
	}
}

static gclient_t *GetGClientPtr(int c)
{
	if (c == -1)
	{
		return NULL;
	}
	if (c == -2)
	{
		return (gclient_t *) -2;	// preserve this value so that I know to load in one of Mike's private NPC clients later
	}

	assert(c >= 0);
	assert(c < level.maxclients);
	return (level.clients + c);
}
//
//
////////////////////////////////


/////////// gitem_t * //////////
//
//
static int GetGItemNum (gitem_t *pItem)
{
	assert(pItem != (gitem_t*) 0xcdcdcdcd);
	
	if (pItem == NULL)
	{
		return -1;
	}
	
	return pItem - bg_itemlist;
}

static gitem_t *GetGItemPtr(int iItem)
{
	if (iItem == -1)
	{
		return NULL;
	}

	assert(iItem >= 0);
	assert(iItem < bg_numItems);
	return &bg_itemlist[iItem];
}
//
//
////////////////////////////////


/////////// vehicleInfo_t * //////////
//
//
static int GetVehicleInfoNum(vehicleInfo_t *pVehicleInfo)
{
	assert(pVehicleInfo != (vehicleInfo_t*) 0xcdcdcdcd);
	
	if (pVehicleInfo == NULL)
	{
		return -1;
	}
	
	return pVehicleInfo - g_vehicleInfo;
}

static vehicleInfo_t *GetVehicleInfoPtr(int iVehicleIndex)
{
	if (iVehicleIndex == -1)
	{
		return NULL;
	}

	assert(iVehicleIndex > 0);
	assert(iVehicleIndex < numVehicles);
	return &g_vehicleInfo[iVehicleIndex];
}
//
//
////////////////////////////////



/////////// field tables ////////
//
// The structures are written and read member by member from the tables in
// g_savetables.cpp (docs/decisions/0015-savegame-format.md). What follows tells the
// table code what the game's pointers are.
//
static std::int32_t SaveIndexOf(void *, const char *psKind, const void *pv, const void *pvRoot)
{
	if (!strcmp(psKind, "entity"))
	{
		return GetGEntityNum((gentity_t *) pv);
	}
	if (!strcmp(psKind, "client"))
	{
		// only an entity has a pointer of this kind, and it is the entity that is being written
		return GetGClientNum((gclient_t *) pv, (gentity_t *) pvRoot);
	}
	if (!strcmp(psKind, "item"))
	{
		return GetGItemNum((gitem_t *) pv);
	}
	if (!strcmp(psKind, "group"))
	{
		return GetGroupNumber((AIGroupInfo_t *) pv);
	}
	if (!strcmp(psKind, "vehicle"))
	{
		return GetVehicleInfoNum((vehicleInfo_t *) pv);
	}
	G_Error("SaveIndexOf: unknown kind of pointer \"%s\"", psKind);
	return -1;
}

static void *SavePointerOf(void *, const char *psKind, std::int32_t iIndex)
{
	if (!strcmp(psKind, "entity"))
	{
		return GetGEntityPtr(iIndex);
	}
	if (!strcmp(psKind, "client"))
	{
		return GetGClientPtr(iIndex);
	}
	if (!strcmp(psKind, "item"))
	{
		return GetGItemPtr(iIndex);
	}
	if (!strcmp(psKind, "group"))
	{
		return GetGroupPtr(iIndex);
	}
	if (!strcmp(psKind, "vehicle"))
	{
		return GetVehicleInfoPtr(iIndex);
	}
	G_Error("SavePointerOf: unknown kind of pointer \"%s\"", psKind);
	return NULL;
}

static char *SaveNewString(void *, std::string_view sText, char *psOriginal)
{
	return GetStringPtr(std::string(sText).c_str(), psOriginal);
}

// What a pointer that is saved as "there is one" holds after loading, until the code
// that loads what it pointed to has replaced it.
static char gcSavedPointerIsPresent;

static const sg::Codec &SaveCodec()
{
	static sg::Codec codec;

	codec.indexOf = SaveIndexOf;
	codec.pointerOf = SavePointerOf;
	codec.newString = SaveNewString;
	codec.present = &gcSavedPointerIsPresent;
	return codec;
}

// The tables the file that is being read was written with.
static sg::Schema gSavedSchema;

// Writes iCount structures of one table as one chunk.
void G_SaveStruct(unsigned int ulChid, const sg::Table &table, const void *pvData, int iCount)
{
	std::vector<std::uint8_t> bytes;

	for (int i=0; i<iCount; i++)
	{
		sg::Write(bytes, table, (const byte *) pvData + i * table.size, SaveCodec());
	}
	gi.AppendToSaveGame(ulChid, bytes.data(), bytes.size());
}

// Reads a chunk of iCount structures of one table. Members the file does not have keep
// the value they have in pvData. pvOriginal is NULL or structures whose strings may be
// reused; it may be pvData itself.
void G_LoadStruct(unsigned int ulChid, const sg::Table &table, void *pvData, const void *pvOriginal, int iCount)
{
	void *pvChunk = NULL;
	const int iLength = gi.ReadFromSaveGame(ulChid, NULL, 0, &pvChunk);

	std::span<const std::uint8_t> data((const std::uint8_t *) pvChunk, pvChunk ? iLength : 0);
	std::string sError;
	bool bOk = true;

	for (int i=0; bOk && i<iCount; i++)
	{
		bOk = gSavedSchema.Read(data, table, (byte *) pvData + i * table.size,
								pvOriginal ? (const byte *) pvOriginal + i * table.size : NULL, SaveCodec(), sError);
	}
	if (bOk && !data.empty())
	{
		bOk = false;
		sError = "the chunk is longer than its description";
	}
	if (pvChunk)
	{
		gi.Free(pvChunk);
	}
	if (!bOk)
	{
		G_Error("The save game could not be read (%s): %s", table.name, sError.c_str());
	}
}

static void WriteSchema()
{
	std::vector<std::uint8_t> bytes;

	sg::WriteSchema(bytes, G_SaveTables());
	gi.AppendToSaveGame('SCHM', bytes.data(), bytes.size());
}

static void ReadSchema()
{
	void *pvChunk = NULL;
	const int iLength = gi.ReadFromSaveGame('SCHM', NULL, 0, &pvChunk);

	std::string sError = "it is missing";
	const bool bOk = pvChunk && gSavedSchema.Parse(std::span<const std::uint8_t>((const std::uint8_t *) pvChunk, iLength), sError);

	if (pvChunk)
	{
		gi.Free(pvChunk);
	}
	if (!bOk)
	{
		G_Error("The save game could not be read: %s", sError.c_str());
	}
}
//
//
////////////////////////////////


/*
==============
WriteLevelLocals

All pointer variables (except function pointers) must be handled specially.
==============
*/
static void WriteLevelLocals ()
{
	// the table leaves out everything from LEVEL_LOCALS_T_SAVESTOP on
	G_SaveStruct('LVLC', sg::TableOf<level_locals_t>(), &level, 1);
}

/*
==============
ReadLevelLocals

All pointer variables (except function pointers) must be handled specially.
==============
*/
static void ReadLevelLocals ()
{
	// Members that are not saved keep their values: the client pointer, because clients are
	// already saved/loaded through Read/Writegame, and everything from
	// LEVEL_LOCALS_T_SAVESTOP on.
	G_LoadStruct('LVLC', sg::TableOf<level_locals_t>(), &level, &level, 1);
}

static void WriteGEntities(qboolean qbAutosave)
{
	int iCount = 0;
	int i;

	for (i=0; i<(qbAutosave?1:globals.num_entities); i++)
	{
		gentity_t* ent = &g_entities[i];

		if ( ent->inuse ) 
		{
			iCount++;
		}
	}

	gi.AppendToSaveGame('NMED', &iCount, sizeof(iCount));

	for (i=0; i<(qbAutosave?1:globals.num_entities); i++)
	{
		gentity_t* ent = &g_entities[i];

		if ( ent->inuse)
		{
			gi.AppendToSaveGame('EDNM', (void *)&i, sizeof(i));

			qboolean qbLinked = ent->linked;
			gi.unlinkentity( ent );
			gentity_t tempEnt = *ent;	// make local copy
			tempEnt.linked = qbLinked;

			if (qbLinked)
			{
				gi.linkentity( ent );
			}

			G_SaveStruct('GENT', sg::TableOf<gentity_s>(), &tempEnt, 1);

			// now for any fiddly bits that would be rather awkward to build into the enumerator...
			//
			if (tempEnt.NPC)
			{
				G_SaveStruct('GNPC', sg::TableOf<gNPC_t>(), ent->NPC, 1);
			}

			if (GetGClientNum(tempEnt.client, &tempEnt) == -2)	// I know, I know...
			{
				G_SaveStruct('GCLI', sg::TableOf<gclient_s>(), ent->client, 1);
			}

			if (tempEnt.parms)
			{
				G_SaveStruct('PARM', sg::TableOf<parms_t>(), ent->parms, 1);
			}
			
			if (tempEnt.m_pVehicle)
			{
				G_SaveStruct('VHIC', sg::TableOf<Vehicle_t>(), ent->m_pVehicle, 1);
			}
			
			// the scary ghoul2 saver stuff...  (fingers crossed)
			//
			gi.G2API_SaveGhoul2Models(tempEnt.ghoul2);								
			tempEnt.ghoul2.kill(); // this handle was shallow copied from an ent. We don't want it destroyed
		}
	}

	//Write out all entity timers
	TIMER_Save();//WriteEntityTimers();

	if (!qbAutosave)
	{
		//Save out ICARUS information
		IIcarusInterface::GetIcarus()->Save();

		// this marker needs to be here, it lets me know if Icarus doesn't load everything back later, 
		//	which has happened, and doesn't always show up onscreen until certain game situations. 
		//	This saves time debugging, and makes things easier to track.
		//
		static int iBlah = 1234;
		gi.AppendToSaveGame('ICOK', &iBlah, sizeof(iBlah));
	}
	if (!qbAutosave )//really shouldn't need to write these bits at all, just restore them from the ents...
	{
		WriteInUseBits();	
	}
}

static void ReadGEntities(qboolean qbAutosave)
{
	int		iCount;
	
	gi.ReadFromSaveGame('NMED', (void *)&iCount, sizeof(iCount), NULL);

	int iPreviousEntRead = -1;
	int i;
	for (i=0; i<iCount; i++)
	{
		int iEntIndex;
		gi.ReadFromSaveGame('EDNM', (void *)&iEntIndex, sizeof(iEntIndex), NULL);

		if (iEntIndex >= globals.num_entities)
		{
			globals.num_entities = iEntIndex + 1;
		}

		if (iPreviousEntRead != iEntIndex-1)
		{
			for (int j=iPreviousEntRead+1; j!=iEntIndex; j++)
			{
				if ( g_entities[j].inuse )		// not actually necessary
				{
					G_FreeEntity(&g_entities[j]);
				}
			}
		}
		iPreviousEntRead = iEntIndex;

		// slightly naff syntax here, but makes a few ops clearer later...
		//
		gentity_t  entity;
		gentity_t* pEntOriginal	= &entity;	
		gentity_t* pEnt			= &g_entities[iEntIndex];
		*pEntOriginal = *pEnt;	// struct copy, so we can refer to original
		
		pEntOriginal->ghoul2.kill();
		gi.unlinkentity(pEnt);
		Quake3Game()->FreeEntity( pEnt );
	
		//
		// sneaky:  destroy the ghoul2 object within this struct before binary-loading over the top of it...
		//
		gi.G2API_LoadSaveCodeDestructGhoul2Info(pEnt->ghoul2);
		pEnt->ghoul2.kill();
		// the entity starts out as a new one; what the file does not have stays that way
		memset((void *)pEnt, 0, sizeof(*pEnt));
		G_LoadStruct('GENT', sg::TableOf<gentity_s>(), pEnt, pEntOriginal, 1);
		pEnt->ghoul2.kill();

		// now for any fiddly bits...
		//
		if (pEnt->NPC)	// was there one or not
		{
			gNPC_t tempNPC;

			memset(&tempNPC, 0, sizeof(tempNPC));
			G_LoadStruct('GNPC', sg::TableOf<gNPC_t>(), &tempNPC, pEntOriginal->NPC, 1);
			
			// so can we pinch the original's one or do we have to alloc a new one?...
			//
			if (pEntOriginal->NPC)
			{
				// pinch this G_Alloc handle...
				//
				pEnt->NPC = pEntOriginal->NPC;
			}
			else
			{
				// original didn't have one (hmmm...), so make a new one...
				//
				//assert(0);	// I want to know about this, though not in release
				pEnt->NPC = (gNPC_t *) G_Alloc(sizeof(*pEnt->NPC));
			}

			// copy over the one we've just loaded...
			//
			*pEnt->NPC = tempNPC;	// struct copy

			//FIXME: do we need to do these too?
			/*
			if ( pEnt->s.number )
			{//not player
				G_LoadAnimFileSet( *pEnt, *pEnt->NPC_type );
				G_SetSkin( *pEnt, *pEnt->NPC_type, NULL );//  it probably wasn't the default skin, do we need this at all?
			}
			*/
		}

		if (pEnt->client == (gclient_t*) -2)	// one of Mike G's NPC clients?
		{
			gclient_t tempGClient;			

			memset((void *)&tempGClient, 0, sizeof(tempGClient));
			G_LoadStruct('GCLI', sg::TableOf<gclient_s>(), &tempGClient, pEntOriginal->client, 1);

			// can we pinch the original's client handle or do we have to alloc a new one?...
			//
			if (pEntOriginal->client)
			{
				// pinch this G_Alloc handle...
				//
				pEnt->client = pEntOriginal->client;
			}
			else
			{
				// original didn't have one (hmmm...) so make a new one...
				//				
				pEnt->client = (gclient_t *) G_Alloc(sizeof(*pEnt->client));
			}

			// copy over the one we've just loaded....
			//
			*pEnt->client = tempGClient;	// struct copy

			if ( pEnt->s.number )
			{//not player
				G_ReloadSaberData( pEnt );
			}
		}

		// Some Icarus thing... (probably)
		//
		if (pEnt->parms)	// was there one or not
		{
			parms_t tempParms;
			
			memset(&tempParms, 0, sizeof(tempParms));
			G_LoadStruct('PARM', sg::TableOf<parms_t>(), &tempParms, NULL, 1);

			// so can we pinch the original's one or do we have to alloc a new one?...
			//
			if (pEntOriginal->parms)
			{
				// pinch this G_Alloc handle...
				//
				pEnt->parms = pEntOriginal->parms;
			}
			else
			{
				// original didn't have one, so make a new one...
				//				
				pEnt->parms = (parms_t *) G_Alloc(sizeof(*pEnt->parms));
			}

			// copy over the one we've just loaded...
			//
			*pEnt->parms = tempParms;	// struct copy
		}

		if (pEnt->m_pVehicle)	// was there one or not
		{
			Vehicle_t tempVehicle;
			
			memset((void *)&tempVehicle, 0, sizeof(tempVehicle));
			G_LoadStruct('VHIC', sg::TableOf<Vehicle_t>(), &tempVehicle, pEntOriginal->m_pVehicle, 1);

			// so can we pinch the original's one or do we have to alloc a new one?...
			//
			if (pEntOriginal->m_pVehicle)
			{
				// pinch this G_Alloc handle...
				//
				pEnt->m_pVehicle = pEntOriginal->m_pVehicle;
			}
			else
			{
				// original didn't have one, so make a new one...
				//				
				pEnt->m_pVehicle = (Vehicle_t *) gi.Malloc( sizeof(Vehicle_t), TAG_G_ALLOC, qfalse );
			}

			// copy over the one we've just loaded...
			//
			*pEnt->m_pVehicle = tempVehicle;	// struct copy
		}

		// the scary ghoul2 stuff...  (fingers crossed)
		//
		{
			char *pGhoul2Data = NULL;
			gi.ReadFromSaveGame('GHL2', 0, 0, (void**)&pGhoul2Data);
			gi.G2API_LoadGhoul2Models(pEnt->ghoul2, pGhoul2Data);	// if it's going to crash anywhere...   <g>
			gi.Free(pGhoul2Data);
		}

//		gi.unlinkentity (pEntOriginal);		
//		ICARUS_FreeEnt( pEntOriginal );
//		*pEntOriginal = *pEnt;	// struct copy				
//		qboolean qbLinked = pEntOriginal->linked;
//		pEntOriginal->linked = qfalse;
//		if (qbLinked)
//		{
//			gi.linkentity (pEntOriginal);		
//		}

		// because the sytem stores sfx_t handles directly instead of the set, we have to reget the set's sfx_t...
		//
		if (pEnt->s.eType == ET_MOVER && pEnt->s.loopSound>0)
		{
			if ( VALIDSTRING( pEnt->soundSet ))
			{
				extern int BMS_MID;	// from g_mover
				pEnt->s.loopSound = CAS_GetBModelSound( pEnt->soundSet, BMS_MID );
				if (pEnt->s.loopSound == -1)
				{
					pEnt->s.loopSound = 0;
				}
			}
		}

		// NPCs and other ents store waypoints that aren't valid after a load
		pEnt->waypoint = 0;
		// Hazard troopers store a troop value that isn't valid either:
		if( pEnt->NPC )
			pEnt->NPC->troop = 0;

		qboolean qbLinked = pEnt->linked;
		pEnt->linked = qfalse;
		if (qbLinked)
		{
			gi.linkentity (pEnt);		
		}		
	}

	//Read in all the entity timers
	TIMER_Load();//ReadEntityTimers();

	if (!qbAutosave)
	{
		// now zap any g_ents that were inuse when the level was loaded, but are no longer in use in the saved version
		//	that we've just loaded...
		//
		for (i=iPreviousEntRead+1; i<globals.num_entities; i++)
		{
			if ( g_entities[i].inuse )	// not actually necessary
			{
				G_FreeEntity(&g_entities[i]);
			}
		}	

		//Load ICARUS information
		Quake3Game()->ClearEntityList();

		IIcarusInterface::GetIcarus()->Load();

		// check that Icarus has loaded everything it saved out by having a marker chunk after it...
		//
		static int iBlah = 1234;
		gi.ReadFromSaveGame('ICOK', &iBlah, sizeof(iBlah), NULL);
	}
	if (!qbAutosave)
	{
		ReadInUseBits();//really shouldn't need to read these bits in at all, just restore them from the ents...
	}
}


void WriteLevel(qboolean qbAutosave)
{
	// the tables that everything below is written with
	WriteSchema();

	if (!qbAutosave) //-always save the client
	{
		// write out one client - us!
		//
		assert(level.maxclients == 1);	// I'll need to know if this changes, otherwise I'll need to change the way ReadGame works
		G_SaveStruct('GCLI', sg::TableOf<gclient_s>(), &level.clients[0], 1);
		WriteLevelLocals();	// level_locals_t level	
	}

	OBJ_SaveObjectiveData();
	FX_Write();

	/////////////
	WriteGEntities(qbAutosave);
	Quake3Game()->VariableSave();
	G_LoadSave_WriteMiscData();

	extern void CG_WriteTheEvilCGHackStuff(void);
	CG_WriteTheEvilCGHackStuff();

	// (Do NOT put any write-code below this line)
	//
	// put out an end-marker so that the load code can check everything was read in...
	//
	static int iDONE = 1234;
	gi.AppendToSaveGame('DONE', &iDONE, sizeof(iDONE));
}

void ReadLevel(qboolean qbAutosave, qboolean qbLoadTransition)
{
	ReadSchema();

	if ( qbLoadTransition )
	{
		// I STRONGLY SUSPECT THAT THIS WILL JUST ERR_DROP BECAUSE OF THE LOAD SWAPPING OF THE CHUNK-ORDER
		//	BELOW BETWEEN OBJECTIVES AND LEVEL_LOCALS, SO I'M GUESSING THIS IS SOME OLD EF1 JUNK?
		// IN ANY CASE, LET'S MAKE SURE...   // -ste (no idea who wrote the comment stuff below, did it ever work?)
		//
		assert(0);
		//
		//loadtransitions do not need to read the objectives and client data from the level they're going to
		//In a loadtransition, client data is carried over on the server and will be stomped later anyway.
		//The objective info (in client->sess data), however, is read in from G_ReadSessionData which is called before this func,
		//we do NOT want to stomp that session data when doing a load transition
		
		//However, we should still save this info out because these savegames may need to be
		//loaded normally later- perhaps if you die and need to respawn, perhaps as some kind
		//of emergency savegame for resuming, etc.

		//SO: We read it in, but throw it away.
		
		//Read & throw away gclient info
		gclient_t junkClient;
		memset((void *)&junkClient, 0, sizeof(junkClient));
		G_LoadStruct('GCLI', sg::TableOf<gclient_s>(), &junkClient, NULL, 1);

		//Read & throw away objective info
		objectives_t	junkObj[MAX_MISSION_OBJ];
		G_LoadStruct('OBJT', sg::TableOf<objectives_t>(), junkObj, NULL, MAX_MISSION_OBJ);

		ReadLevelLocals();	// level_locals_t level	
	}
	else
	{
		if (!qbAutosave )//always load the client unless it's an autosave
		{
			assert(level.maxclients == 1);	// I'll need to know if this changes, otherwise I'll need to change the way things work
		
			// members that are not saved, the saber's strings among them, keep their values
			G_LoadStruct('GCLI', sg::TableOf<gclient_s>(), &level.clients[0], &level.clients[0], 1);
			ReadLevelLocals();	// level_locals_t level	
		}
		
		OBJ_LoadObjectiveData();//loads mission objectives AND tactical info
	}

	FX_Read();

	/////////////

	ReadGEntities(qbAutosave);
	Quake3Game()->VariableLoad();
	G_LoadSave_ReadMiscData();

	extern void CG_ReadTheEvilCGHackStuff(void);
	CG_ReadTheEvilCGHackStuff();

	// (Do NOT put any read-code below this line)
	//
	// check that the whole file content was loaded by specifically requesting an end-marker...
	//
	static int iDONE = 1234;
	gi.ReadFromSaveGame('DONE', &iDONE, sizeof(iDONE), NULL);
}

extern int killPlayerTimer;
qboolean GameAllowedToSaveHere(void)
{
	return (!in_camera&&!killPlayerTimer);
}

//////////////////// eof /////////////////////

#if 0
// !!!!!!!!!!!!!!!!!! loadsave affecting structure !!!!!!!!!!!!!!!!!!!!!!!
struct Vehicle_t
{
	// The entity who pilots/drives this vehicle.
	// NOTE: This is redundant (since m_pParentEntity->owner _should_ be the pilot). This makes things clearer though.
	gentity_t *m_pPilot;

	int m_iPilotTime; //if spawnflag to die without pilot and this < level.time then die.
	qboolean m_bHasHadPilot; //qtrue once the vehicle gets its first pilot

	// The passengers of this vehicle.
	gentity_t **m_ppPassengers;

	// The number of passengers currently in this vehicle.
	int m_iNumPassengers;

	//the droid unit NPC for this vehicle, if any
	gentity_t *m_pDroidUnit;

	// The entity from which this NPC comes from.
	gentity_t *m_pParentEntity;

	// If not zero, how long to wait before we can do anything with the vehicle (we're getting on still).
	// -1 = board from left, -2 = board from right, -3 = jump/quick board.  -4 & -5 = throw off existing pilot
	int		m_iBoarding;

	// Used to check if we've just started the boarding process
	bool	m_bWasBoarding;

	// The speed the vehicle maintains while boarding occurs (often zero)
	vec3_t	m_vBoardingVelocity;

	// Time modifier (must only be used in ProcessMoveCommands() and ProcessOrientCommands() and is updated in Update()).
	float m_fTimeModifier;

	// Ghoul2 Animation info.
	// NOTE: Since each vehicle has their own model instance, these bolts must be local to each vehicle as well.
	int m_iLeftWingBone;
	int m_iRightWingBone;
	//int m_iDriverTag;
	int m_iExhaustTag[MAX_VEHICLE_EXHAUSTS];
	int m_iMuzzleTag[MAX_VEHICLE_MUZZLES];
	int m_iDroidUnitTag;
	int	m_iGunnerViewTag[MAX_VEHICLE_TURRETS];//Where to put the view origin of the gunner (index)

	// This vehicles weapon muzzles.
	Muzzle m_Muzzles[MAX_VEHICLE_MUZZLES];

	// The user commands structure.
	usercmd_t m_ucmd;

	// The direction an entity will eject from the vehicle towards.
	int m_EjectDir;

	// Flags that describe the vehicles behavior.
	unsigned int m_ulFlags;

	// NOTE: Vehicle Type ID, Orientation, and Armor MUST be transmitted over the net.

	// Current angles of this vehicle.
	vec3_t		m_vOrientation;

	// How long you have strafed left or right (increments every frame that you strafe to right, decrements every frame you strafe left)
	int			m_fStrafeTime;	

	// Previous angles of this vehicle.
	vec3_t		m_vPrevOrientation;

	// When control is lost on a speeder, current angular velocity is stored here and applied until landing
	float		m_vAngularVelocity;

	vec3_t		m_vFullAngleVelocity;

	// Current armor and shields of your vehicle (explodes if armor to 0).
	int			m_iArmor;	//hull strength - STAT_HEALTH on NPC
	int			m_iShields;	//energy shielding - STAT_ARMOR on NPC

	// Timer for all cgame-FX...? ex: exhaust?
	int			m_iLastFXTime; 

	// When to die.
	int			m_iDieTime;

	// This pointer is to a valid VehicleInfo (which could be an animal, speeder, fighter, whatever). This
	// contains the functions actually used to do things to this specific kind of vehicle as well as shared
	// information (max speed, type, etc...).
	vehicleInfo_t *m_pVehicleInfo;

	// This trace tells us if we're within landing height.
	trace_t m_LandTrace;

	//bitflag of surfaces that have broken off
	int			m_iRemovedSurfaces;

	// the last time this vehicle fired a turbo burst
	int			m_iTurboTime;

	//how long it should drop like a rock for after freed from SUSPEND
	int			m_iDropTime;

	int			m_iSoundDebounceTimer;

	//last time we incremented the shields
	int			lastShieldInc;

	//so we don't hold it down and toggle it back and forth
	qboolean	linkWeaponToggleHeld;

	//info about our weapons (linked, ammo, etc.)
	vehWeaponStatus_t	weaponStatus[MAX_VEHICLE_WEAPONS];
	vehTurretStatus_t	turretStatus[MAX_VEHICLE_TURRETS];

	//the guy who was previously the pilot
	gentity_t*	m_pOldPilot;

	// don't need these in mp
	int			m_safeJumpMountTime;
	float		m_safeJumpMountRightDot;
};

#endif
