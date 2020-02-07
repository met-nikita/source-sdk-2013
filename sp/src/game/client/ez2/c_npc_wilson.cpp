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
	m_Glow.m_bDirectional = false;
	m_Glow.m_bInSky = false;
}

C_NPC_Wilson::~C_NPC_Wilson()
{
}

void C_NPC_Wilson::Simulate( void )
{
	// The dim light is the flashlight.
	if ( m_bEyeLightEnabled && (m_EyeLight == NULL || m_EyeLight->m_flBrightnessScale > 0.0f) )
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

			// Update the flashlight
			m_EyeLight->UpdateLight( vVector, vecForward, vecRight, vecUp, 40 );

			// Update the glow
			// The glow needs to be a few units in front of the eye
			Vector vGlowPos = vVector + (vecForward.Normalized() * 2.0f);
			m_Glow.SetOrigin( vGlowPos );
			m_Glow.m_vPos = vGlowPos;
			m_Glow.m_flHDRColorScale = m_EyeLight->m_flBrightnessScale;
		}
	}
	else
	{
		if ( m_EyeLight )
		{
			// Turned off the flashlight; delete it.
			delete m_EyeLight;
			m_EyeLight = NULL;
		}

		m_Glow.Deactivate();
	}

	BaseClass::Simulate();
}

void C_NPC_Wilson::OnDataChanged( DataUpdateType_t type )
{
	if (m_iEyeLightBrightness != m_iLastEyeLightBrightness)
	{
		// 255 = Equivalent to sprite color byte
		m_flEyeLightBrightnessScale = (float)(m_iEyeLightBrightness) / 255.0f;
		m_iLastEyeLightBrightness = m_iEyeLightBrightness;
	}

	if ( type == DATA_UPDATE_CREATED )
	{
		// Setup our light glow.
		m_Glow.m_nSprites = 1;

		m_Glow.m_Sprites[0].m_flVertSize = 4.0f;
		m_Glow.m_Sprites[0].m_flHorzSize = 4.0f;
		m_Glow.m_Sprites[0].m_vColor = Vector( 255, 0, 0 );
		
		m_Glow.SetOrigin( GetAbsOrigin() );
		m_Glow.SetFadeDistances( 512, 2048, 3072 );
		m_Glow.m_flProxyRadius = 2.0f;

		SetNextClientThink( gpGlobals->curtime + RandomFloat(0,3.0) );
	}

	BaseClass::OnDataChanged( type );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Wilson::ClientThink( void )
{
	Vector mins = GetAbsOrigin();
	if ( engine->IsBoxVisible( mins, mins ) )
	{
		m_Glow.Activate();
	}
	else
	{
		m_Glow.Deactivate();
	}

	SetNextClientThink( gpGlobals->curtime + RandomFloat(1.0,3.0) );
}
