//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Teleports a named entity to a given position and restores
//			it's physics state
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "coopmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"





LINK_ENTITY_TO_CLASS(coop_manager, CCoopManager);


BEGIN_DATADESC(CCoopManager)

DEFINE_FIELD(m_eSpawnSpot, FIELD_EDICT),

DEFINE_INPUTFUNC(FIELD_STRING, "MoveSpawn", InputMoveSpawn),
DEFINE_INPUTFUNC(FIELD_STRING, "AddRespawnWeapon", InputAddRespawnWeapon),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCoopManager::Activate(void)
{
	BaseClass::Activate();
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCoopManager::InputMoveSpawn(inputdata_t &inputdata)
{
	CBaseEntity *pEntity = gEntList.FindEntityByName(inputdata.pCaller, inputdata.value.String());
	if (pEntity)
	{
		m_eSpawnSpot = pEntity;
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CCoopManager::InputAddRespawnWeapon(inputdata_t &inputdata)
{
	CBaseEntity *pEntity = gEntList.FindEntityByClassname(NULL, "game_player_equip");
	if (!pEntity)
		pEntity = CreateEntityByName("game_player_equip");
	if (!pEntity)
		return;
	char key[64];
	char value[64];
	const char *szValue = inputdata.value.String();
	// Separate key from value
	char *delimiter = Q_strstr(szValue, " ");
	if (delimiter)
	{
		Q_strncpy(key, szValue, MIN((delimiter - szValue) + 1, sizeof(key)));
		Q_strncpy(value, delimiter + 1, sizeof(value));
	}
	pEntity->KeyValue(key, value);
}
