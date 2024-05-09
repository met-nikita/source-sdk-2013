//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		357 - hand gun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "gamestats.h"
#ifdef CLIENT_DLL
#include "c_te_effect_dispatch.h"
#else
#include "te_effect_dispatch.h"
#include "soundent.h"
#include "game.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeapon357
//-----------------------------------------------------------------------------

#if defined(MAPBASE) && !defined(CLIENT_DLL)
extern acttable_t *GetPistolActtable();
extern int GetPistolActtableCount();
#endif

#ifdef CLIENT_DLL
#define CWeapon357 C_Weapon357
#endif

class CWeapon357 : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeapon357, CBaseHLCombatWeapon );
public:

	CWeapon357( void );

	void	PrimaryAttack( void );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	float	WeaponAutoAimScale()	{ return 0.6f; }

	bool IsPredicted() const { return true; };

#ifdef MAPBASE
#ifndef CLIENT_DLL
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
#else
	int		CapabilitiesGet(void) { return 0; }
#endif

	virtual int	GetMinBurst() { return 1; }
	virtual int	GetMaxBurst() { return 1; }
	virtual float	GetMinRestTime( void ) { return 1.0f; }
	virtual float	GetMaxRestTime( void ) { return 2.5f; }

	virtual float GetFireRate( void )
	{
#ifdef EZ2
		if (m_hLeftHandGun != NULL)
			return 0.5f;
#endif

		return 1.0f;
	}

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_15DEGREES;
		if (!GetOwner() || !GetOwner()->IsNPC())
			return cone;

		static Vector AllyCone = VECTOR_CONE_2DEGREES;
		static Vector NPCCone = VECTOR_CONE_5DEGREES;
#ifndef CLIENT_DLL
		if( GetOwner()->MyNPCPointer()->IsPlayerAlly() )
		{
			// 357 allies should be cooler
			return AllyCone;
		}
#endif
		return NPCCone;
	}

	void	FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void	Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
#ifndef CLIENT_DLL
	virtual acttable_t		*GetBackupActivityList() { return GetPistolActtable(); }
	virtual int				GetBackupActivityListCount() { return GetPistolActtableCount(); }
#else
	virtual acttable_t		*GetBackupActivityList() { return 0; }
	virtual int				GetBackupActivityListCount() { return 0; }
#endif
#endif

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();
#if defined(MAPBASE) && !defined(CLIENT_DLL)
	DECLARE_ACTTABLE();
#endif

#ifdef EZ2
	WeaponClass_t			WeaponClassify() { return WEPCLASS_HANDGUN; }
	virtual void			SetActivity( Activity act, float duration );

	bool				CanDualWield() const { return true; }
	CBaseAnimating		*GetLeftHandGun() const { return m_hLeftHandGun; }
	void				SetLeftHandGun( CBaseAnimating *pGun ) { m_hLeftHandGun = pGun; }

private:

	CHandle<CBaseAnimating> m_hLeftHandGun;
#endif
};

LINK_ENTITY_TO_CLASS( weapon_357, CWeapon357 );

PRECACHE_WEAPON_REGISTER( weapon_357 );

IMPLEMENT_NETWORKCLASS_ALIASED(Weapon357, DT_Weapon357)

BEGIN_NETWORK_TABLE(CWeapon357, DT_Weapon357)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeapon357)
END_PREDICTION_DATA()

BEGIN_DATADESC( CWeapon357 )

#ifdef EZ2
	DEFINE_FIELD( m_hLeftHandGun, FIELD_EHANDLE ),
#endif

END_DATADESC()

#if defined(MAPBASE) && !defined(CLIENT_DLL)
acttable_t	CWeapon357::m_acttable[] =
{
#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_IDLE,						ACT_IDLE_REVOLVER,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_REVOLVER,				true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_REVOLVER,			true },
	{ ACT_RELOAD,					ACT_RELOAD_REVOLVER,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_REVOLVER,				true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_REVOLVER,				true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_REVOLVER,	true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_REVOLVER_LOW,				false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_REVOLVER_LOW,		false },
	{ ACT_COVER_LOW,				ACT_COVER_REVOLVER_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_REVOLVER_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_REVOLVER,			false },
	{ ACT_WALK,						ACT_WALK_REVOLVER,					true },
	{ ACT_RUN,						ACT_RUN_REVOLVER,					true },
#else
	{ ACT_IDLE,						ACT_IDLE_PISTOL,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,			true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,		true },
	{ ACT_RELOAD,					ACT_RELOAD_PISTOL,				true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,			true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,				true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,	false },
	{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,			false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,		false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_WALK,						ACT_WALK_PISTOL,				false },
	{ ACT_RUN,						ACT_RUN_PISTOL,					false },
#endif

	// 
	// Activities ported from weapon_alyxgun below
	// 

	// Readiness activities (not aiming)
#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL_RELAXED,		false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_PISTOL_STIMULATED,		false },
#else
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL,				false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_STIMULATED,			false },
#endif
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_STEALTH,				ACT_IDLE_STEALTH_PISTOL,		false },

#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_WALK_RELAXED,				ACT_WALK_PISTOL_RELAXED,		false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_PISTOL_STIMULATED,		false },
#else
	{ ACT_WALK_RELAXED,				ACT_WALK,						false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_STIMULATED,			false },
#endif
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_STEALTH,				ACT_WALK_STEALTH_PISTOL,		false },
	
#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_RUN_RELAXED,				ACT_RUN_PISTOL_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_PISTOL_STIMULATED,		false },
#else
	{ ACT_RUN_RELAXED,				ACT_RUN,						false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_STIMULATED,				false },
#endif
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_STEALTH,				ACT_RUN_STEALTH_PISTOL,			false },

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_PISTOL,				false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_ANGRY_PISTOL,			false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_AIM_STEALTH,			ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK,						false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_PISTOL,			false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_AIM_STEALTH,			ACT_WALK_AIM_STEALTH_PISTOL,	false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN,						false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_PISTOL,				false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_AIM_STEALTH,			ACT_RUN_AIM_STEALTH_PISTOL,		false },//always aims
	//End readiness activities

	// Crouch activities
	{ ACT_CROUCHIDLE_STIMULATED,	ACT_CROUCHIDLE_STIMULATED,		false },
	{ ACT_CROUCHIDLE_AIM_STIMULATED,ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims
	{ ACT_CROUCHIDLE_AGITATED,		ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims

	// Readiness translations
	{ ACT_READINESS_RELAXED_TO_STIMULATED, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED, false },
	{ ACT_READINESS_RELAXED_TO_STIMULATED_WALK, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK, false },
	{ ACT_READINESS_AGITATED_TO_STIMULATED, ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED, false },
	{ ACT_READINESS_STIMULATED_TO_RELAXED, ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED, false },

#if EXPANDED_HL2_COVER_ACTIVITIES
	{ ACT_RANGE_AIM_MED,			ACT_RANGE_AIM_REVOLVER_MED,			false },
	{ ACT_RANGE_ATTACK1_MED,		ACT_RANGE_ATTACK_REVOLVER_MED,		false },
#endif

#ifdef MAPBASE
	// HL2:DM activities (for third-person animations in SP)
#if EXPANDED_HL2DM_ACTIVITIES
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_REVOLVER,                    false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_REVOLVER,                    false },
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_REVOLVER,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_REVOLVER,            false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_REVOLVER,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_REVOLVER,    false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_REVOLVER,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_REVOLVER,        false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_REVOLVER,                    false },
#else
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,                    false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,            false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,        false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,                    false },
#endif
#endif
};


IMPLEMENT_ACTTABLE( CWeapon357 );

// Allows Weapon_BackupActivity() to access the 357's activity table.
acttable_t *Get357Acttable()
{
	return CWeapon357::m_acttable;
}

int Get357ActtableCount()
{
	return ARRAYSIZE(CWeapon357::m_acttable);
}
#endif

#ifdef EZ2
void CWeapon357::SetActivity( Activity act, float duration )
{
	// HACKHACK: Can't recompile all of the models to have this right now
	if (act == ACT_RANGE_ATTACK_DUAL_PISTOLS && SelectWeightedSequence( act ) == ACTIVITY_NOT_AVAILABLE)
		act = ACT_RANGE_ATTACK_REVOLVER;

	BaseClass::SetActivity( act, duration );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeapon357::CWeapon357( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;

#ifdef MAPBASE
	m_fMinRange1		= 24;
	m_fMaxRange1		= 1000;
	m_fMinRange2		= 24;
	m_fMaxRange2		= 200;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeapon357::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
#ifndef CLIENT_DLL
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	switch( pEvent->event )
	{
		case EVENT_WEAPON_RELOAD:
			{
				CEffectData data;

				// Emit six spent shells
				for ( int i = 0; i < 6; i++ )
				{
					data.m_vOrigin = pOwner->WorldSpaceCenter() + RandomVector( -4, 4 );
					data.m_vAngles = QAngle( 90, random->RandomInt( 0, 360 ), 0 );
					data.m_nEntIndex = entindex();

					DispatchEffect( "ShellEject", data );
				}

				break;
			}
#ifdef MAPBASE
		case EVENT_WEAPON_PISTOL_FIRE:
			{
#ifdef EZ2
				// HACKHACK: Ignore the regular firing event while dual-wielding
				if (GetLeftHandGun())
					return;
#endif

				Vector vecShootOrigin, vecShootDir;
				vecShootOrigin = pOperator->Weapon_ShootPosition();

				CAI_BaseNPC *npc = pOperator->MyNPCPointer();
				ASSERT( npc != NULL );

				vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

				FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
			}
			break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
#endif
	}
#endif
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeapon357::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
#ifndef CLIENT_DLL
	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

	WeaponSound( SINGLE_NPC );
	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1 );
	pOperator->DoMuzzleFlash();
	m_iClip1 = m_iClip1 - 1;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Some things need this. (e.g. the new Force(X)Fire inputs or blindfire actbusy)
//-----------------------------------------------------------------------------
void CWeapon357::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
#ifndef CLIENT_DLL
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
#endif
}
#endif

#if defined(EZ2) && !defined(CLIENT_DLL)
// Hack for 357 achievement (see achievements_EZ2.cpp)
extern bool g_bEZ2357AchievementHack;
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeapon357::PrimaryAttack( void )
{
	ITEM_GRAB_PREDICTED_ATTACK_FIX

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_iPrimaryAttacks++;
#ifndef CLIENT_DLL
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
#endif

	WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );	

#if defined(EZ2) && !defined(CLIENT_DLL)
	// Hack for 357 achievement
	g_bEZ2357AchievementHack = true;
#endif
	// Fire the bullets
	FireBulletsInfo_t info;
	info.m_iShots = 1;
	info.m_vecSrc = vecSrc;
	info.m_vecDirShooting = vecAiming;
	info.m_vecSpread = vec3_origin;
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 0;

	pPlayer->FireBullets( info );

#if defined(EZ2) && !defined(CLIENT_DLL)
	// Hack for 357 achievement
	g_bEZ2357AchievementHack = false;
#endif

#ifndef CLIENT_DLL
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt( -1, 1 );
	angles.y += random->RandomInt( -1, 1 );
	angles.z = 0;


	pPlayer->SnapEyeAngles( angles );
#endif
	pPlayer->ViewPunch( QAngle( -8, random->RandomFloat( -2, 2 ), 0 ) );
#ifndef CLIENT_DLL
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner() );
#endif

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}
}
