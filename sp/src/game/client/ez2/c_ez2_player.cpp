#include "cbase.h"
#include "c_EZ2_player.h"
#include "point_bonusmaps_accessor.h"
#include "achievementmgr.h"
#include "basegrenade_shared.h"

#if defined( CEZ2Player )
	#undef CEZ2Player
#endif

ConVar cl_slam_glow( "cl_slam_glow", "0" );

IMPLEMENT_CLIENTCLASS_DT( C_EZ2_Player, DT_EZ2_Player, CEZ2_Player )
	RecvPropBool( RECVINFO( m_bBonusChallengeUpdate ) ),
	RecvPropEHandle( RECVINFO( m_hWarningTarget ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_EZ2_Player )
END_PREDICTION_DATA()

C_EZ2_Player::C_EZ2_Player()
{
}

C_EZ2_Player::~C_EZ2_Player()
{
	DestroyGlowTargetEffect();
	DestroySLAMGlowEffect();
}

void C_EZ2_Player::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_DATATABLE_CHANGED )
	{
		if ( m_bBonusChallengeUpdate )
		{
			// Borrow the achievement manager for this
			CAchievementMgr *pAchievementMgr = dynamic_cast<CAchievementMgr *>(engine->GetAchievementMgr());
			if (pAchievementMgr)
			{
				if (pAchievementMgr->WereCheatsEverOn())
					return;
			}

			char szChallengeFileName[128];
			char szChallengeMapName[128];
			char szChallengeName[128];
			BonusMapChallengeNames( szChallengeFileName, szChallengeMapName, szChallengeName );
			BonusMapChallengeUpdate( szChallengeFileName, szChallengeMapName, szChallengeName, GetBonusProgress() );

			m_bBonusChallengeUpdate = false;
		}

		if (cl_slam_glow.GetBool())
		{
			if ( m_HL2Local.m_iSatchelCount != m_hActiveSatchels.Count() || m_HL2Local.m_iTripmineCount != m_hActiveTripmines.Count() )
			{
				// Recollect active satchels/tripmines
				m_hActiveSatchels.RemoveAll();
				m_hActiveTripmines.RemoveAll();

				C_BaseEntity *pEntity = NULL;
				C_BaseGrenade *pGrenade = NULL;
				const CEntInfo *pInfo = ClientEntityList().FirstEntInfo();
				for ( ;pInfo; pInfo = pInfo->m_pNext )
				{
					pEntity = (C_BaseEntity *)pInfo->m_pEntity;
					if ( !pEntity )
						continue;

					if ( FStrEq( pEntity->GetClassname(), "npc_satchel" ) )
					{
						pGrenade = static_cast<C_BaseGrenade*>(pEntity);
						if (pGrenade->GetThrower() == this && !pGrenade->IsMarkedForDeletion())
							m_hActiveSatchels.AddToTail( pEntity );
					}
					else if ( FStrEq( pEntity->GetClassname(), "npc_tripmine" ) )
					{
						pGrenade = static_cast<C_BaseGrenade*>(pEntity);
						if (pGrenade->GetThrower() == this && !pGrenade->IsMarkedForDeletion())
							m_hActiveTripmines.AddToTail( pEntity );
					}
				}
			}
		}
	}

	UpdateGlowTargetEffect();
	UpdateSLAMGlowEffect();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EZ2_Player::UpdateGlowTargetEffect( void )
{
	// destroy the existing effect
	if ( m_pGlowTargetEffect )
	{
		DestroyGlowTargetEffect();
	}

	if (!m_hWarningTarget)
	{
		return;
	}

	// create a new effect
	//if ( !m_bGlowDisabled )
	{
		Vector4D vecColor( 1.0f, 0, 0, 1.0f );
		m_pGlowTargetEffect = new CGlowObject( m_hWarningTarget, vecColor.AsVector3D(), vecColor.w, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EZ2_Player::DestroyGlowTargetEffect( void )
{
	if ( m_pGlowTargetEffect )
	{
		delete m_pGlowTargetEffect;
		m_pGlowTargetEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EZ2_Player::UpdateSLAMGlowEffect( void )
{
	// destroy the existing effects
	if ( m_pSLAMGlowEffects.Count() > 0 )
	{
		DestroySLAMGlowEffect();
	}

	if (!cl_slam_glow.GetBool())
		return;

	static Vector vecSatchelColor( 1.0f, 0.2f, 0.2f );
	static Vector vecTripmineColor( 0.5f, 0.75f, 1.0f );

	for (int i = 0; i < m_hActiveSatchels.Count(); i++)
	{
		int iNewGlow = m_pSLAMGlowEffects.AddToTail();
		m_pSLAMGlowEffects[iNewGlow] = new CGlowObject( m_hActiveSatchels[i], vecSatchelColor, 1.0f, true );
	}

	for (int i = 0; i < m_hActiveTripmines.Count(); i++)
	{
		int iNewGlow = m_pSLAMGlowEffects.AddToTail();
		m_pSLAMGlowEffects[iNewGlow] = new CGlowObject( m_hActiveTripmines[i], vecTripmineColor, 1.0f, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EZ2_Player::DestroySLAMGlowEffect( void )
{
	m_pSLAMGlowEffects.PurgeAndDeleteElements();
}

