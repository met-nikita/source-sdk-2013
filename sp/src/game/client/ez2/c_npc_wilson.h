#ifndef C_NPC_Wilson_H
#define C_NPC_Wilson_H
#ifdef _WIN32
#pragma once
#endif

#include "c_ai_basenpc.h"

class C_NPC_Wilson : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_NPC_Wilson, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_NPC_Wilson();
	~C_NPC_Wilson();

	void OnDataChanged( DataUpdateType_t type );
};


#endif // C_SDK_PLAYER_H