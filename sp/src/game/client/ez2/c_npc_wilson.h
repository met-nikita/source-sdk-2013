#ifndef C_NPC_Wilson_H
#define C_NPC_Wilson_H
#ifdef _WIN32
#pragma once
#endif

#include "c_ai_basenpc.h"
#include "flashlighteffect.h"

class C_NPC_Wilson : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_NPC_Wilson, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_NPC_Wilson();
	~C_NPC_Wilson();

	void OnDataChanged( DataUpdateType_t type );
	void Simulate( void );

	bool m_bEyeLightEnabled;

	int m_iLastEyeLightBrightness;
	int m_iEyeLightBrightness;

	float m_flEyeLightBrightnessScale;
	CTurretLightEffect *m_EyeLight;
};


#endif // C_SDK_PLAYER_H