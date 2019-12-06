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
	RecvPropBool( RECVINFO( m_bEyeLightEnabled ) ),
	RecvPropInt( RECVINFO( m_iEyeLightBrightness ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_NPC_Wilson )
END_PREDICTION_DATA()

C_NPC_Wilson::C_NPC_Wilson()
{
}

C_NPC_Wilson::~C_NPC_Wilson()
{
}

void C_NPC_Wilson::Simulate( void )
{
	// The dim light is the flashlight.
	if ( m_bEyeLightEnabled )
	{
		if ( m_EyeLight == NULL )
		{
			// Turned on the headlight; create it.
			m_EyeLight = new CTurretLightEffect;

			if ( m_EyeLight == NULL )
				return;

			m_EyeLight->m_flBrightnessScale = m_flEyeLightBrightnessScale;
			m_EyeLight->TurnOn();
		}
		else if (m_EyeLight->m_flBrightnessScale != m_flEyeLightBrightnessScale)
		{
			m_EyeLight->m_flBrightnessScale = FLerp(m_EyeLight->m_flBrightnessScale, m_flEyeLightBrightnessScale, 0.1f);
		}

		QAngle vAngle;
		Vector vVector;
		Vector vecForward, vecRight, vecUp;

		int iAttachment = LookupAttachment( "light" );

		if ( iAttachment != -1 )
		{
			GetAttachment( iAttachment, vVector, vAngle );
			AngleVectors( vAngle, &vecForward, &vecRight, &vecUp );
		
			m_EyeLight->UpdateLight( vVector, vecForward, vecRight, vecUp, 40 );
		}
	}
	else if ( m_EyeLight )
	{
		// Turned off the flashlight; delete it.
		delete m_EyeLight;
		m_EyeLight = NULL;
	}

	BaseClass::Simulate();
}

void C_NPC_Wilson::OnDataChanged( DataUpdateType_t type )
{
	if (m_iEyeLightBrightness != m_iLastEyeLightBrightness)
	{
		// 255 = Equivalent to WILSON_EYE_GLOW_DEFAULT_BRIGHT
		m_flEyeLightBrightnessScale = (float)(m_iEyeLightBrightness) / 255.0f;
		m_iLastEyeLightBrightness = m_iEyeLightBrightness;
	}

	BaseClass::OnDataChanged( type );
}
