#ifndef C_NPC_Wilson_H
#define C_NPC_Wilson_H
#ifdef _WIN32
#pragma once
#endif

#include "c_ai_basenpc.h"
#include "flashlighteffect.h"
#include "glow_overlay.h"
#include "view.h"
#include "c_pixel_visibility.h"

// Based on env_lightglow code
class C_TurretGlowOverlay : public CGlowOverlay
{
public:

	virtual void CalcSpriteColorAndSize( float flDot, CGlowSprite *pSprite, float *flHorzSize, float *flVertSize, Vector *vColor )
	{
		*flHorzSize = pSprite->m_flHorzSize;
		*flVertSize = pSprite->m_flVertSize;
		
		Vector viewDir = ( CurrentViewOrigin() - m_vecOrigin );
		float distToViewer = VectorNormalize( viewDir );

		float fade;

		// See if we're in the outer fade distance range
		if ( m_nOuterMaxDist > m_nMaxDist && distToViewer > m_nMaxDist )
		{
			fade = RemapValClamped( distToViewer, m_nMaxDist, m_nOuterMaxDist, 1.0f, 0.0f );
		}
		else
		{
			fade = RemapValClamped( distToViewer, m_nMinDist, m_nMaxDist, 0.0f, 1.0f );
		}
		
		*vColor = pSprite->m_vColor * fade * m_flGlowObstructionScale;
	}

	void SetOrigin( const Vector &origin ) { m_vecOrigin = origin; }
	
	void SetFadeDistances( int minDist, int maxDist, int outerMaxDist )
	{
		m_nMinDist = minDist;
		m_nMaxDist = maxDist;
		m_nOuterMaxDist = outerMaxDist;
	}

protected:

	Vector	m_vecOrigin;
	int		m_nMinDist;
	int		m_nMaxDist;
	int		m_nOuterMaxDist;
};

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
	void ClientThink( void );

	bool m_bEyeLightEnabled;

	int m_iLastEyeLightBrightness;
	int m_iEyeLightBrightness;

	float m_flEyeLightBrightnessScale;
	CTurretLightEffect *m_EyeLight;

	C_TurretGlowOverlay m_Glow;
};


#endif // C_SDK_PLAYER_H