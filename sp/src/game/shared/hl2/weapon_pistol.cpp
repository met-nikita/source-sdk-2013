//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Pistol - hand gun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "ammodef.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "gamestats.h"
#include "hl2_player_shared.h"
#ifdef CLIENT_DLL
#include "c_te_effect_dispatch.h"
#include "sprite.h"
#include "takedamageinfo.h"
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "baseviewmodel_shared.h"
#include "prediction.h"
#else
#include "soundent.h"
#include "game.h"
#include "te_effect_dispatch.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#endif

#ifdef EZ
#include "beam_shared.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	PISTOL_FASTEST_REFIRE_TIME		0.1f
#define	PISTOL_FASTEST_DRY_REFIRE_TIME	0.2f

#define	PISTOL_ACCURACY_SHOT_PENALTY_TIME		0.2f	// Applied amount of time each shot adds to the time we must recover from
#define	PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME	1.5f	// Maximum penalty to deal out

ConVar	pistol_use_new_accuracy( "pistol_use_new_accuracy", "1" );

#ifdef CLIENT_DLL
#define CWeaponPistol C_WeaponPistol
#define CWeaponPulsePistol C_WeaponPulsePistol
#endif

//-----------------------------------------------------------------------------
// CWeaponPistol
//-----------------------------------------------------------------------------

class CWeaponPistol : public CHLMachineGun
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponPistol, CHLMachineGun);

	CWeaponPistol(void);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	void	Precache( void );
#ifndef EZ
	void	ItemPostFrame( void );
#else
	virtual void	ItemPostFrame( void );
#endif
	void	ItemPreFrame( void );
	virtual bool IsPredicted() const { return true; };
	void	ItemBusyFrame( void );
	void	PrimaryAttack( void );
	void	AddViewKick( void );
	void	DryFire( void );
#ifndef CLIENT_DLL
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#ifdef MAPBASE
	void	FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void	Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
#endif
#endif
	void	UpdatePenaltyTime( void );

#ifndef CLIENT_DLL
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
#endif
	Activity	GetPrimaryAttackActivity( void );

	virtual bool Reload( void );

	virtual const Vector& GetBulletSpread( void )
	{		
		// Handle NPCs first
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
			
		static Vector cone;

		if ( pistol_use_new_accuracy.GetBool() )
		{
			float ramp = RemapValClamped(	m_flAccuracyPenalty, 
											0.0f, 
											PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME, 
											0.0f, 
											1.0f ); 

			// We lerp from very accurate to inaccurate over time
			VectorLerp( VECTOR_CONE_1DEGREES, VECTOR_CONE_6DEGREES, ramp, cone );
		}
		else
		{
			// Old value
			cone = VECTOR_CONE_4DEGREES;
		}

		return cone;
	}
	
	virtual int	GetMinBurst() 
	{ 
		return 1; 
	}

	virtual int	GetMaxBurst() 
	{ 
		return 3; 
	}

	virtual float GetFireRate( void ) 
	{
		return 0.5f; 
	}
#ifndef CLIENT_DLL
#ifdef MAPBASE
	// Pistols are their own backup activities
	virtual acttable_t		*GetBackupActivityList() { return NULL; }
	virtual int				GetBackupActivityListCount() { return 0; }
#endif

	DECLARE_ACTTABLE();
#endif

protected:
	float	m_flSoonestPrimaryAttack;
	float	m_flLastAttackTime;
	float	m_flAccuracyPenalty;
	int		m_nNumShotsFired;
};


IMPLEMENT_NETWORKCLASS_ALIASED(WeaponPistol, DT_WeaponPistol)

BEGIN_NETWORK_TABLE(CWeaponPistol, DT_WeaponPistol)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponPistol)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_pistol, CWeaponPistol );
PRECACHE_WEAPON_REGISTER( weapon_pistol );

BEGIN_DATADESC( CWeaponPistol )

	DEFINE_FIELD( m_flSoonestPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( m_flLastAttackTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flAccuracyPenalty,		FIELD_FLOAT ), //NOTENOTE: This is NOT tracking game time
	DEFINE_FIELD( m_nNumShotsFired,			FIELD_INTEGER ),

END_DATADESC()

#ifndef CLIENT_DLL
acttable_t	CWeaponPistol::m_acttable[] = 
{
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

#ifdef MAPBASE
	// 
	// Activities ported from weapon_alyxgun below
	// 

#if EXPANDED_HL2_WEAPON_ACTIVITIES
	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL_RELAXED,		false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_PISTOL_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_STEALTH,				ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_RELAXED,				ACT_WALK_PISTOL_RELAXED,		false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_PISTOL_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_STEALTH,				ACT_WALK_STEALTH_PISTOL,		false },

	{ ACT_RUN_RELAXED,				ACT_RUN_PISTOL_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_PISTOL_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_STEALTH,				ACT_RUN_STEALTH_PISTOL,			false },

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_PISTOL_RELAXED,		false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_PISTOL_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_AIM_STEALTH,			ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_PISTOL_RELAXED,		false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_PISTOL,			false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_AIM_STEALTH,			ACT_WALK_AIM_STEALTH_PISTOL,	false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_PISTOL_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_PISTOL,				false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_AIM_STEALTH,			ACT_RUN_AIM_STEALTH_PISTOL,		false },//always aims
	//End readiness activities
#else
	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL,				false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_STIMULATED,			false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_STEALTH,				ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_RELAXED,				ACT_WALK,						false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_STIMULATED,			false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_STEALTH,				ACT_WALK_STEALTH_PISTOL,		false },

	{ ACT_RUN_RELAXED,				ACT_RUN,						false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_STIMULATED,				false },
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
#endif

	// Crouch activities
	{ ACT_CROUCHIDLE_STIMULATED,	ACT_CROUCHIDLE_STIMULATED,		false },
	{ ACT_CROUCHIDLE_AIM_STIMULATED,ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims
	{ ACT_CROUCHIDLE_AGITATED,		ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims

	// Readiness translations
	{ ACT_READINESS_RELAXED_TO_STIMULATED, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED, false },
	{ ACT_READINESS_RELAXED_TO_STIMULATED_WALK, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK, false },
	{ ACT_READINESS_AGITATED_TO_STIMULATED, ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED, false },
	{ ACT_READINESS_STIMULATED_TO_RELAXED, ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED, false },
#endif

#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_PISTOL,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_PISTOL,		true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_PISTOL,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_PISTOL,		true },
#endif

#if EXPANDED_HL2_COVER_ACTIVITIES
	{ ACT_RANGE_AIM_MED,			ACT_RANGE_AIM_PISTOL_MED,			false },
	{ ACT_RANGE_ATTACK1_MED,		ACT_RANGE_ATTACK_PISTOL_MED,		false },

	{ ACT_COVER_WALL_R,			ACT_COVER_WALL_R_PISTOL,		false },
	{ ACT_COVER_WALL_L,			ACT_COVER_WALL_L_PISTOL,		false },
	{ ACT_COVER_WALL_LOW_R,		ACT_COVER_WALL_LOW_R_PISTOL,	false },
	{ ACT_COVER_WALL_LOW_L,		ACT_COVER_WALL_LOW_L_PISTOL,	false },
#endif

#ifdef MAPBASE
	// HL2:DM activities (for third-person animations in SP)
	{ ACT_HL2MP_IDLE,                    ACT_HL2MP_IDLE_PISTOL,                    false },
	{ ACT_HL2MP_RUN,                    ACT_HL2MP_RUN_PISTOL,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,            ACT_HL2MP_IDLE_CROUCH_PISTOL,            false },
	{ ACT_HL2MP_WALK_CROUCH,            ACT_HL2MP_WALK_CROUCH_PISTOL,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,    ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,            ACT_HL2MP_GESTURE_RELOAD_PISTOL,        false },
	{ ACT_HL2MP_JUMP,                    ACT_HL2MP_JUMP_PISTOL,                    false },
#if EXPANDED_HL2DM_ACTIVITIES
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_PISTOL,						false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_PISTOL,		false },
#endif
#endif
};


IMPLEMENT_ACTTABLE( CWeaponPistol );

#ifdef MAPBASE
// Allows Weapon_BackupActivity() to access the pistol's activity table.
acttable_t *GetPistolActtable()
{
	return CWeaponPistol::m_acttable;
}

int GetPistolActtableCount()
{
	return ARRAYSIZE(CWeaponPistol::m_acttable);
}
#endif
#endif
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponPistol::CWeaponPistol( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_fMinRange1		= 24;
	m_fMaxRange1		= 1500;
	m_fMinRange2		= 24;
	m_fMaxRange2		= 200;

	m_bFiresUnderwater	= true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistol::Precache( void )
{
	BaseClass::Precache();
}
#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponPistol::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_PISTOL_FIRE:
		{
			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = pOperator->Weapon_ShootPosition();

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );

			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

#ifdef MAPBASE
			FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
#else
			CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

			WeaponSound( SINGLE_NPC );
			pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );
			pOperator->DoMuzzleFlash();
			m_iClip1 = m_iClip1 - 1;
#endif
		}
		break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistol::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

	WeaponSound( SINGLE_NPC );

	FireBulletsInfo_t info;
	info.m_iShots = 1;
	info.m_vecSrc = vecShootOrigin;
	info.m_vecDirShooting = vecShootDir;
	info.m_vecSpread = VECTOR_CONE_PRECALCULATED;
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;

	pOperator->FireBullets( info );
	pOperator->DoMuzzleFlash();
	m_iClip1 = m_iClip1 - 1;
}

//-----------------------------------------------------------------------------
// Purpose: Some things need this. (e.g. the new Force(X)Fire inputs or blindfire actbusy)
//-----------------------------------------------------------------------------
void CWeaponPistol::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
}
#endif
#endif
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponPistol::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flSoonestPrimaryAttack	= gpGlobals->curtime + PISTOL_FASTEST_DRY_REFIRE_TIME;
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponPistol::PrimaryAttack( void )
{
	if ( ( gpGlobals->curtime - m_flLastAttackTime ) > 0.5f )
	{
		m_nNumShotsFired = 0;
	}
	else
	{
		m_nNumShotsFired++;
	}

	m_flLastAttackTime = gpGlobals->curtime;
	m_flSoonestPrimaryAttack = gpGlobals->curtime + PISTOL_FASTEST_REFIRE_TIME;
#ifndef CLIENT_DLL
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, GetOwner() );
#endif

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if( pOwner )
	{
		// Each time the player fires the pistol, reset the view punch. This prevents
		// the aim from 'drifting off' when the player fires very quickly. This may
		// not be the ideal way to achieve this, but it's cheap and it works, which is
		// great for a feature we're evaluating. (sjb)
		pOwner->ViewPunchReset();
	}

	BaseClass::PrimaryAttack();

	// Add an accuracy penalty which can move past our maximum penalty time if we're really spastic
	m_flAccuracyPenalty += PISTOL_ACCURACY_SHOT_PENALTY_TIME;

	m_iPrimaryAttacks++;
#ifndef CLIENT_DLL
	gamestats->Event_WeaponFired( pOwner, true, GetClassname() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistol::UpdatePenaltyTime( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// Check our penalty time decay
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistol::ItemPreFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistol::ItemBusyFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponPistol::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_bInReload )
		return;
	
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	//Allow a refire as fast as the player can click
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
	else if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
	{
		DryFire();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
Activity CWeaponPistol::GetPrimaryAttackActivity( void )
{
	if ( m_nNumShotsFired < 1 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nNumShotsFired < 2 )
		return ACT_VM_RECOIL1;

	if ( m_nNumShotsFired < 3 )
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponPistol::Reload( void )
{
	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		WeaponSound( RELOAD );
		m_flAccuracyPenalty = 0.0f;
	}
	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistol::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat( 0.25f, 0.5f );
	viewPunch.y = random->RandomFloat( -.6f, .6f );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}

#ifdef EZ2
#define	PULSE_PISTOL_FASTEST_REFIRE_TIME		0.4f
#define	PULSE_PISTOL_FASTEST_DRY_REFIRE_TIME	1.5f
#define	PULSE_PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME	10.0f

enum PulsePistolStyle
{
	DEFAULT = 0,
	PRIMARY_CHARGE,
	LANCE_NO_CHARGE,
	SELECT_FIRE
};

ConVar sk_plr_dmg_pulse_lance("sk_plr_dmg_pulse_lance", "15", FCVAR_REPLICATED);
ConVar sv_pulse_pistol_style( "sv_pulse_pistol_style", "0", FCVAR_REPLICATED, "Style of pulse pistol altfire: 0) Default 1) Charge 2) Lance" );
ConVar sv_pulse_lance_style("sv_pulse_lance_style", "0", FCVAR_REPLICATED, "Style of pulse lance: 0) Vortigaunt 1) Stalker");
ConVar sv_pulse_pistol_charge_per_extra_shot("sv_pulse_pistol_charge_per_extra_shot", "5", FCVAR_REPLICATED, "How many ammo charges equals one bullet");
ConVar sv_pulse_pistol_slide_return_charge("sv_pulse_pistol_slide_return_charge", "10", FCVAR_REPLICATED, "How much charge to restore when racking the slide on the pulse pistol");
ConVar sv_pulse_pistol_no_charge_hold("sv_pulse_pistol_no_charge_hold", "0", FCVAR_REPLICATED, "If 1, fully charged shots will fire automatically");
ConVar sv_pulse_pistol_max_charge("sv_pulse_pistol_max_charge", "40", FCVAR_REPLICATED, "Maximum ammo to charge in one shot. For a 50 charge pulse pistol, it should be 40");

//-----------------------------------------------------------------------------
// CWeaponPulsePistol, the famous melee replacement taken to a separate class.
//-----------------------------------------------------------------------------
class CWeaponPulsePistol : public CWeaponPistol
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS(CWeaponPulsePistol, CWeaponPistol);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponPulsePistol();

	void	Activate( void );
	void	Precache();
	bool IsPredicted() const { return true; };
	bool	IsChargePressed( int chargeButton, CBasePlayer * pOwner );
	void	ItemPostFrame( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack();
	void	ChargeAttack( void );
	void	AddViewKick( void );
	void	DryFire( void );
#ifndef CLIENT_DLL
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif
	virtual bool Reload( void ) { return false; } // The pulse pistol does not reload

	virtual int GetMaxClip2( void ) const { int iBase = BaseClass::GetMaxClip2(); return iBase > 0 ? iBase : sv_pulse_pistol_max_charge.GetInt(); }

	virtual const Vector& GetBulletSpread( void )
	{
		// Handle NPCs first
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if (GetOwner() && GetOwner()->IsNPC())
			return npcCone;

		static Vector cone;

		float ramp = RemapValClamped( m_flAccuracyPenalty,
			0.0f,
			PULSE_PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME,
			0.0f,
			1.0f );

		// We lerp from very accurate to inaccurate over time
		VectorLerp( VECTOR_CONE_2DEGREES, VECTOR_CONE_15DEGREES, ramp, cone );

		return cone;
	}

	virtual float GetFireRate( void )
	{
		return 3.0f;
	}

	virtual void	UpdateOnRemove( void );

	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	void	DoImpactEffect( trace_t &tr, int nDamageType );
	void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	void	DoMuzzleFlash( void );

	void	StartChargeEffects( void );
	void	SetChargeEffectBrightness( float alpha );
	void	KillChargeEffects();

	void	CreateBeamBlast( const Vector &vecOrigin );

protected:
	CHandle<CSprite>	m_hChargeSprite;

private:
	// For recharging the ammo
	void			RechargeAmmo();
	int				m_nAmmoCount = 50;
	float			m_flDrainRemainder;
	float			m_flChargeRemainder;
	float			m_flLastChargeTime;
	float			m_flLastChargeSoundTime;
};

CWeaponPulsePistol::CWeaponPulsePistol()
{
	m_iClip2 = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::Activate( void )
{
	BaseClass::Activate();
	/*
#ifndef CLIENT_DLL
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer && pPlayer->IsPredictingWeapons())
		IPredictionSystem::SuppressHostEvents(pPlayer);
#endif
	// Charge up and fire attack
	StartChargeEffects();
#ifndef CLIENT_DLL
	if (pPlayer && pPlayer->IsPredictingWeapons())
		IPredictionSystem::SuppressHostEvents(NULL);
#endif
		*/
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::Precache( void )
{
	PrecacheModel( "effects/fluttercore.vmt" );

	// TEMP assets
	// Until we know what the alt-fire is going to look like, let's just
	// steal all the assets from these two NPCs.
	UTIL_PrecacheOther( "npc_vortigaunt" );
	UTIL_PrecacheOther( "npc_stalker" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::UpdateOnRemove( void )
{
	//Turn off charge glow
	KillChargeEffects();

	// Chain at end to mimic destructor unwind order
	BaseClass::UpdateOnRemove();
}

bool CWeaponPulsePistol::IsChargePressed( int chargeButton, CBasePlayer * pOwner )
{
	// If "no charge hold" is set, and we're at the maximum charge, treat it as though the player let go of the attack button
	if (sv_pulse_pistol_no_charge_hold.GetBool() && m_iClip2 >= GetMaxClip2())
		return false;

	// If "no charge hold" is set, we're out of ammo, but we have a charge, treat it as though the player let go of the attack button
	if (sv_pulse_pistol_no_charge_hold.GetBool() && m_iClip2 > 0 && m_iClip1 <= 10)
		return false;

	return (pOwner->m_nButtons & chargeButton) > 0;
}

//-----------------------------------------------------------------------------
// Purpose: Recharge ammo before handling post-frame
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::ItemPostFrame( void )
{
	// Charge up and fire attack
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	bool chargePressed = false;
	switch ( sv_pulse_pistol_style.GetInt() )
	{
	case PRIMARY_CHARGE:
		chargePressed = IsChargePressed( IN_ATTACK, pOwner );
		break;
	case LANCE_NO_CHARGE:
		break;
	case SELECT_FIRE:
		// UNIMPLEMENTED
		DevMsg( "Select fire mode not implemented yet!\n" );
		break;
	default:
		chargePressed = IsChargePressed( IN_ATTACK2, pOwner );
		break;
	}

	// Player has released attack 2 and there is more than 1 ammo charged to fire
	if (!chargePressed && ( m_iClip2 > 0 ) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		// Reset the next attack to the current time to avoid multiple shots queuing up
		m_flNextPrimaryAttack = gpGlobals->curtime;

		// Fire
		PrimaryAttack();

		// Turn off sprite
		SetChargeEffectBrightness( 0.0f );

		return;
	}
	else if (chargePressed || ( m_iClip2 > 0 ) )
	{
		// Charge the next shot
		ChargeAttack();
		return;
	}
	// If the charge up and fire wasn't either charged or shot this frame, reset so the gun doesn't go off unexpectedly
	m_iClip2 = 0;

	RechargeAmmo();

	// Debounce the recoiling counter
	if ((pOwner->m_nButtons & IN_ATTACK) == false)
	{
		m_nShotsFired = 0;
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: The pulse pistol has a longer dryfire refire time than the 9mm
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );

	m_flSoonestPrimaryAttack	= gpGlobals->curtime + PULSE_PISTOL_FASTEST_DRY_REFIRE_TIME;
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

// Hackhack - For some reason this animation event resolves to 124 when Cyonsia uses it in the viewmodel. Why?
// After adding another shared animation event, it bumped to 125. Why?
// TODO Replace AE_WPN_INCREMENTAMMO with AE_SLIDERETURN in the model and remove this section.
#define	AE_WPN_INCREMENTAMMO LAST_SHARED_ANIMEVENT + 79

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Override AE_WPN_INCREMENTAMMO to make sure one pulse round is
//		'chambered' and play the reload sound
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::Operator_HandleAnimEvent( animevent_t * pEvent, CBaseCombatCharacter * pOperator )
{
	switch (pEvent->event)
	{
	case AE_WPN_INCREMENTAMMO:
	case AE_SLIDERETURN:
		m_iClip1 = MAX( m_iClip1, sv_pulse_pistol_slide_return_charge.GetInt() );
		WeaponSound( RELOAD, m_flNextPrimaryAttack );
		break;
	}

	BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Recharge ammo before handling weapon deploy
//-----------------------------------------------------------------------------
bool CWeaponPulsePistol::Deploy( void )
{
	RechargeAmmo();
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponPulsePistol::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if (m_iClip2 > 0)
	{
		// If there is any charge, play a click
		// Mostly just to cancel the current charge sound
		WeaponSound( EMPTY );
	}

	// Holstering resets charge
	m_iClip2 = 0;
	SetChargeEffectBrightness( 0.0f );

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Replenish ammo based on time elapsed since last charge
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::RechargeAmmo( void )
{
	// If there is a fully charged shot waiting, don't recharge
	if (m_iClip2 >= GetMaxClip2())
	{
		return;
	}

	// 1upD - how long as the pistol been charging?
	float flChargeInterval = gpGlobals->curtime - m_flLastChargeTime;
	m_flLastChargeTime = gpGlobals->curtime;

	// This code is inherited from the airboat. I've changed it to work for the pistol.
	int nMaxAmmo = 50;
	if (m_iClip1 == nMaxAmmo)
	{
		return;
	}

	float flRechargeRate = 5;
	float flChargeAmount = flRechargeRate * flChargeInterval;
	if (m_flDrainRemainder != 0.0f)
	{
		if (m_flDrainRemainder >= flChargeAmount)
		{
			m_flDrainRemainder -= flChargeAmount;
			return;
		}
		else
		{
			flChargeAmount -= m_flDrainRemainder;
			m_flDrainRemainder = 0.0f;
		}
	}

	m_flChargeRemainder += flChargeAmount;
	int nAmmoToAdd = (int)m_flChargeRemainder;
	m_flChargeRemainder -= nAmmoToAdd;
	m_iClip1 += nAmmoToAdd;
	if (m_iClip1 > nMaxAmmo)
	{
		m_iClip1 = nMaxAmmo;
		m_flChargeRemainder = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: BREADMAN - look in fx_hl2_tracers.cpp
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	float flTracerDist;
	Vector vecDir;
	Vector vecEndPos;

	vecDir = tr.endpos - vecTracerSrc;

	flTracerDist = VectorNormalize( vecDir );

	UTIL_Tracer( vecTracerSrc, tr.endpos, 1, TRACER_FLAG_USEATTACHMENT, 5000, true, "GaussTracer" );
}

//-----------------------------------------------------------------------------
// Purpose: all BREADMAN
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::PrimaryAttack( void )
{
	//this fixes shooting effect sometimes missing when holding charge button to the end
#if defined( CLIENT_DLL )
	if (prediction->InPrediction() && !prediction->IsFirstTimePredicted())
		return;
#endif
	ITEM_GRAB_PREDICTED_ATTACK_FIX
	if (m_iClip1 < 10) // Don't allow firing if we only have one shot left
	{
		DryFire();
	}
	else
	{
		// Abort here to handle burst and auto fire modes
		if ((UsesClipsForAmmo1() && m_iClip1 == 0) || (!UsesClipsForAmmo1() && !pPlayer->GetAmmoCount( m_iPrimaryAmmoType )))
			return;

		m_nNumShotsFired++;

		pPlayer->DoMuzzleFlash();

		// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
		// especially if the weapon we're firing has a really fast rate of fire.
		int iBulletsToFire = 0;
		float fireRate = GetFireRate();

		int iExtraChargeBullets = m_iClip2 / sv_pulse_pistol_charge_per_extra_shot.GetInt();

		// The pulse pistol fires one extra round per ten ammo charged
		iBulletsToFire += iExtraChargeBullets;

		if (iExtraChargeBullets > 0)
		{
			// Add the accuracy penalty for extra shots BEFORE firing to increase spread
			m_flAccuracyPenalty += PISTOL_ACCURACY_SHOT_PENALTY_TIME * iExtraChargeBullets;
		}

		// MUST call sound before removing a round from the clip of a CHLMachineGun
		while (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
			// Pulse pistol fires two bullets
			iBulletsToFire += 2;

			// If the shot is charged, play a special sound
			if ( iBulletsToFire >= 4 )
			{
				// 'Burst' will be used for the charge up sound
				WeaponSound( BURST, m_flNextPrimaryAttack );
			}
			else
			{
				WeaponSound( SINGLE, m_flNextPrimaryAttack );
			}
		}

		// Subtract the charge
		m_iClip1 = MAX(m_iClip1 - 10, 1); // Never drop the charge below 1
		m_iClip2 = 0;

		m_iPrimaryAttacks++;
#ifndef CLIENT_DLL
		gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
#endif

		//int	bulletType = GetAmmoDef()->Index("AR2");

		// Fire the bullets
		FireBulletsInfo_t info;
		info.m_iShots = iBulletsToFire;
		info.m_vecSrc = pPlayer->Weapon_ShootPosition();
		info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
		CHL2_Player *pHLPlayer = dynamic_cast<CHL2_Player *> (pPlayer);
		info.m_vecSpread = pHLPlayer->GetAttackSpread(this);
		info.m_flDistance = MAX_TRACE_LENGTH;
		info.m_iAmmoType = m_iPrimaryAmmoType;
		info.m_iTracerFreq = 1;
		FireBullets( info );

		//Factor in the view kick
		AddViewKick();

		DoMuzzleFlash();
#ifndef CLIENT_DLL
		CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pPlayer );
#endif
		if (!m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
		{
			// HEV suit - indicate out of ammo condition
			pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
		}

		SendWeaponAnim( GetPrimaryAttackActivity() );
		pPlayer->SetAnimation( PLAYER_ATTACK1 );
#ifndef CLIENT_DLL
		// Register a muzzleflash for the AI
		pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
#endif
		// Add to the accuracy penalty just like standard pistol
		m_flAccuracyPenalty += PISTOL_ACCURACY_SHOT_PENALTY_TIME;

		m_iPrimaryAttacks++;
#ifndef CLIENT_DLL
		gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
#endif
		m_flSoonestPrimaryAttack = gpGlobals->curtime + PULSE_PISTOL_FASTEST_REFIRE_TIME;
		m_flNextPrimaryAttack = gpGlobals->curtime + PULSE_PISTOL_FASTEST_REFIRE_TIME;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Charge shot with ammo
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::ChargeAttack( void )
{
	// Play the slide rack animation if we're trying to charge but have no ammo
	if ( m_iClip1 <= 10 && m_iClip2 <= 0 ) {
		if( m_flNextPrimaryAttack <= gpGlobals->curtime )
			DryFire();
		return;
	}

	int nMaxCharge = GetMaxClip2();
	// If there is only one shot left or the charge has reached maximum, do not charge!
	if (m_iClip1 <= 10 || m_iClip2 == nMaxCharge)
	{
		RechargeAmmo();
		return;
	}

	// Play sound
	if (gpGlobals->curtime - m_flLastChargeSoundTime > 4.0f && m_iClip2 == 0)
	{
		WeaponSound( SPECIAL1 );
		m_flLastChargeSoundTime = gpGlobals->curtime;
	}

	float flChargeInterval = gpGlobals->curtime - m_flLastChargeTime;
	m_flLastChargeTime = gpGlobals->curtime;

	// The charge attack charges twice as quickly as ammo recharges
	float flRechargeRate = 10;
	float flChargeAmount = flRechargeRate * flChargeInterval;

	m_flChargeRemainder += flChargeAmount;
	int nAmmoToAdd = (int)m_flChargeRemainder;
	m_flChargeRemainder -= nAmmoToAdd;
	m_iClip2 += nAmmoToAdd;
	m_iClip1 = MAX( m_iClip1 - nAmmoToAdd, 1); // Never drop the charge below 1
	if (m_iClip2 > nMaxCharge)
	{
		m_iClip2 = nMaxCharge;
		m_flChargeRemainder = 0.0f;
	}

#ifndef CLIENT_DLL
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer && pPlayer->IsPredictingWeapons())
		IPredictionSystem::SuppressHostEvents(pPlayer);
#endif
	// Display the charge sprite
	StartChargeEffects();
	SetChargeEffectBrightness( m_iClip2 );
#ifndef CLIENT_DLL
	if (pPlayer && pPlayer->IsPredictingWeapons())
		IPredictionSystem::SuppressHostEvents(NULL);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );

	if (pPlayer == NULL)
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat( -4.5f, -4.5f );
	viewPunch.y = random->RandomFloat( -.13f, .13f );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}

//-----------------------------------------------------------------------	------
// Purpose: BREADMAN
// Input  : &tr - 
//			nDamageType - 
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::DoImpactEffect( trace_t &tr, int nDamageType )
{
	CEffectData data;

	data.m_vOrigin = tr.endpos + (tr.plane.normal * 1.0f);
	data.m_vNormal = tr.plane.normal;

	DispatchEffect( "AR2Impact", data );

	BaseClass::DoImpactEffect( tr, nDamageType );
}

//-----------------------------------------------------------------------------
// Purpose: Particle muzzle flash for pulse pistol
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::DoMuzzleFlash( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (pOwner == NULL)
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if (pViewModel == NULL)
		return;

	CEffectData data;
#ifndef CLIENT_DLL
	data.m_nEntIndex = pViewModel->entindex();
#else
	data.m_hEntity = pViewModel->entindex();
#endif
	data.m_nAttachmentIndex = 1;
	data.m_flScale = 0.25f; // The pistol is much smaller than a Hunter-Chopper ( ._.)
	DispatchEffect( "ChopperMuzzleFlash", data );

	BaseClass::DoMuzzleFlash();
}

//-----------------------------------------------------------------------------
// Charge up effect methods
//
// I'm content with but not super enthusiastic about how these effects look
// right now. Perhaps in a later phase of development we can improve the look
// and sound of the pulse pistol alt-fire, but this is sufficent for alpha
// testing at the very least.
//
// 1upD
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Create effects for pulse pistol alt-fire
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::StartChargeEffects()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	//Create the charge glow
	if (pOwner != NULL && m_hChargeSprite == NULL)
	{

		m_hChargeSprite = SPRITE_CREATE_PREDICTABLE("effects/fluttercore.vmt", GetAbsOrigin(), false);
		if (m_hChargeSprite)
		{
			m_hChargeSprite->SetOwnerEntity(pOwner);
			m_hChargeSprite->SetPlayerSimulated(pOwner);
#ifndef CLIENT_DLL
			m_hChargeSprite->SetAsTemporary();
#endif
			m_hChargeSprite->SetAttachment(pOwner->GetViewModel(), 1);
			m_hChargeSprite->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone);
			m_hChargeSprite->SetBrightness(0, 0.1f);
			m_hChargeSprite->SetScale(0.05f, 0.05f);
			m_hChargeSprite->TurnOn();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Change the brightness of the alt-fire effects
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::SetChargeEffectBrightness( float alpha )
{
	if (m_hChargeSprite != NULL)
	{
		m_hChargeSprite->SetBrightness( m_iClip2, 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove pulse pistol alt-fire effects
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::KillChargeEffects()
{
	if (m_hChargeSprite != NULL)
	{
#ifndef CLIENT_DLL
		UTIL_Remove( m_hChargeSprite );
#else
		m_hChargeSprite->Remove();
#endif
		m_hChargeSprite = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Heavy damage directly forward
// Input  : nHand - Handedness of the beam
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::SecondaryAttack()
{
	ITEM_GRAB_PREDICTED_ATTACK_FIX

	float refireModifier = sv_pulse_lance_style.GetInt() == 1 ? 0.25 : 1;

	//DoMuzzleFlash();
#ifndef CLIENT_DLL
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pPlayer );
#endif

	if (!m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	}

	SendWeaponAnim( GetPrimaryAttackActivity() );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
#ifndef CLIENT_DLL
	// Register a muzzleflash for the AI
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
#endif

	m_iSecondaryAttacks++;
#ifndef CLIENT_DLL
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
#endif

	m_flNextSecondaryAttack = gpGlobals->curtime + ( PULSE_PISTOL_FASTEST_REFIRE_TIME * refireModifier );

	// Slow down ammo recharge
	m_flLastChargeTime = gpGlobals->curtime;

	// Fire the beam
	Vector forward;
	GetVectors( &forward, NULL, NULL );
	float maxRange = 100.0f;

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAim = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );

	trace_t tr;

	UTIL_TraceLine( vecSrc, vecSrc + (vecAim * maxRange), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
#ifndef CLIENT_DLL
	if (pPlayer && pPlayer->IsPredictingWeapons())
		IPredictionSystem::SuppressHostEvents(pPlayer);
#endif
	// Check to store off our view model index
	CBeam *pBeam = CBeam::BeamCreatePredictable(__FILE__, __LINE__, false, "sprites/laser.vmt", 2.0, pPlayer);


	if (pBeam != NULL)
	{
		pBeam->PointEntInit( tr.endpos, this );
		pBeam->SetEndAttachment( 1 );
		pBeam->SetWidth( 3.0 );
		pBeam->SetEndWidth( 12.8 );
		pBeam->SetBrightness( 255 );
		pBeam->SetColor( 255, 0, 0 );
		pBeam->LiveForTime( 0.1f );
		pBeam->RelinkBeam();
	}

	UTIL_DecalTrace( &tr, "RedGlowFade" );
	UTIL_DecalTrace( &tr, "FadingScorch" );

	CBaseEntity *pEntity = tr.m_pEnt;
	if (pEntity != NULL && m_takedamage)
	{
		int dmgType = sv_pulse_lance_style.GetInt() == 1 ? DMG_BURN :  DMG_SHOCK | DMG_DISSOLVE;

		CTakeDamageInfo dmgInfo( this, this, sk_plr_dmg_pulse_lance.GetFloat() * refireModifier, dmgType );
		dmgInfo.SetDamagePosition( tr.endpos );
		VectorNormalize( vecAim );// not a unit vec yet
								  // hit like a 5kg object flying 100 ft/s
		dmgInfo.SetDamageForce( 5 * 100 * 12 * vecAim );

		// Send the damage to the recipient
		pEntity->DispatchTraceAttack( dmgInfo, vecAim, &tr );
	}

	// Create a cover for the end of the beam
	CreateBeamBlast( tr.endpos );
#ifndef CLIENT_DLL
	if (pPlayer && pPlayer->IsPredictingWeapons())
		IPredictionSystem::SuppressHostEvents(NULL);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Creates a blast where the beam has struck a target
// Input  : &vecOrigin - position to eminate from
//-----------------------------------------------------------------------------
void CWeaponPulsePistol::CreateBeamBlast( const Vector &vecOrigin )
{
	CSprite *pBlastSprite = SPRITE_CREATE_PREDICTABLE("sprites/redglow1.vmt", GetAbsOrigin(), FALSE);
	if (pBlastSprite != NULL)
	{
		pBlastSprite->SetTransparency( kRenderTransAddFrameBlend, 255, 255, 255, 255, kRenderFxNone );
		pBlastSprite->SetBrightness( 255 );
		pBlastSprite->SetScale( random->RandomFloat( 0.5f, 0.75f ) );
		pBlastSprite->AnimateAndDie( 45.0f );
		
		pBlastSprite->EmitSound( "NPC_Vortigaunt.Explode" );
	}

	CPVSFilter filter( vecOrigin );
	te->GaussExplosion( filter, 0.0f, vecOrigin, Vector( 0, 0, 1 ), 0 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponPulsePistol, DT_WeaponPulsePistol)

BEGIN_NETWORK_TABLE(CWeaponPulsePistol, DT_WeaponPulsePistol)
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponPulsePistol)
DEFINE_PRED_FIELD_TOL(m_flDrainRemainder, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
DEFINE_PRED_FIELD_TOL(m_flChargeRemainder, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
DEFINE_PRED_FIELD_TOL(m_flLastChargeTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
END_PREDICTION_DATA()
#endif

BEGIN_DATADESC( CWeaponPulsePistol )
	DEFINE_FIELD( m_flLastChargeTime, FIELD_TIME ),
	DEFINE_FIELD( m_flLastChargeSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_hChargeSprite, FIELD_EHANDLE )
END_DATADESC()

LINK_ENTITY_TO_CLASS(weapon_pulsepistol, CWeaponPulsePistol);
#endif

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Apply effects when the pulse pistol is charging
//-----------------------------------------------------------------------------
class CPulsePistolMaterialProxy : public CEntityMaterialProxy
{
public:
	CPulsePistolMaterialProxy() { m_ChargeVar = NULL; }
	virtual ~CPulsePistolMaterialProxy() {}
	virtual bool Init(IMaterial *pMaterial, KeyValues *pKeyValues);
	virtual void OnBind(C_BaseEntity *pEntity);
	virtual IMaterial *GetMaterial();

private:
	IMaterialVar *m_ChargeVar;
	//IMaterialVar *m_FullyChargedVar;
};

bool CPulsePistolMaterialProxy::Init(IMaterial *pMaterial, KeyValues *pKeyValues)
{
	bool foundAnyVar = false;
	bool foundVar;

	const char *pszChargeVar = pKeyValues->GetString("outputCharge");
	m_ChargeVar = pMaterial->FindVar(pszChargeVar, &foundVar, true);
	if (foundVar)
		foundAnyVar = true;

	//const char *pszFullyChargedVar = pKeyValues->GetString( "outputFullyCharged" );
	//m_FullyChargedVar = pMaterial->FindVar( pszFullyChargedVar, &foundVar, true );
	//if (foundVar)
	//	foundAnyVar = true;

	return foundAnyVar;
}

void CPulsePistolMaterialProxy::OnBind(C_BaseEntity *pEnt)
{
	if (!m_ChargeVar)
		return;

	C_WeaponPulsePistol *pPistol = NULL;
	if (pEnt->IsBaseCombatWeapon())
		pPistol = assert_cast<C_WeaponPulsePistol*>(pEnt);
	else
	{
		C_BaseViewModel *pVM = dynamic_cast<C_BaseViewModel*>(pEnt);
		if (pVM)
			pPistol = assert_cast<C_WeaponPulsePistol*>(pVM->GetOwningWeapon());
	}

	if (pPistol)
	{
		// Keep synced with the max in weapon_pistol.cpp
		const float flMaxCharge = 40.0f;
		m_ChargeVar->SetFloatValue(((float)pPistol->m_iClip2) / flMaxCharge);

		//m_FullyChargedVar->SetFloatValue( pPistol->m_iClip2 == flMaxCharge ? 1.0f : 0.0f );
	}
	else
	{
		m_ChargeVar->SetFloatValue(0.0f);
	}
}

IMaterial *CPulsePistolMaterialProxy::GetMaterial()
{
	if (!m_ChargeVar)
		return NULL;

	return m_ChargeVar->GetOwningMaterial();
}

EXPOSE_INTERFACE(CPulsePistolMaterialProxy, IMaterialProxy, "PulsePistolCharge" IMATERIAL_PROXY_INTERFACE_VERSION);

#endif