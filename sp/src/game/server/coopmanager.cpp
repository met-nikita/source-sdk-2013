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
