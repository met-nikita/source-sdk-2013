//================= Copyright LOLOLOLOL, All rights reserved. ==================//
//
// Purpose: Bad Cop's friend, a pacifist turret who could open doors for some reason.
// 
// Author: Blixibon
//
//=============================================================================//

#include "cbase.h"
#include "c_npc_wilson.h"

IMPLEMENT_CLIENTCLASS_DT( C_NPC_Wilson, DT_NPC_Wilson, CNPC_Wilson )
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_NPC_Wilson )
END_PREDICTION_DATA()

C_NPC_Wilson::C_NPC_Wilson()
{
}

C_NPC_Wilson::~C_NPC_Wilson()
{
}

void C_NPC_Wilson::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );
}
