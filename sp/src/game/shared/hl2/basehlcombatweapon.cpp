//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "hl2_player_shared.h"
#ifndef CLIENT_DLL
#include "soundent.h"
#include "ai_basenpc.h"
#include "game.h"
#endif
#include "in_buttons.h"
#include "gamestats.h"
#ifdef CSS_WEAPONS_IN_HL2
#include "ammodef.h"
#endif
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(HLMachineGun, DT_HLMachineGun)

BEGIN_NETWORK_TABLE(CHLMachineGun, DT_HLMachineGun)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CHLMachineGun)
END_PREDICTION_DATA()

//=========================================================
//	>> CHLSelectFireMachineGun
//=========================================================
BEGIN_DATADESC( CHLMachineGun )

	DEFINE_FIELD( m_nShotsFired,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextSoundTime, FIELD_TIME ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHLMachineGun::CHLMachineGun( void )
{
}

const Vector &CHLMachineGun::GetBulletSpread( void )
{
	static Vector cone = VECTOR_CONE_3DEGREES;
	return cone;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CHLMachineGun::PrimaryAttack( void )
{
	ITEM_GRAB_PREDICTED_ATTACK_FIX
	
	// Abort here to handle burst and auto fire modes
	if ( (UsesClipsForAmmo1() && m_iClip1 == 0) || ( !UsesClipsForAmmo1() && !pPlayer->GetAmmoCount(m_iPrimaryAmmoType) ) )
		return;

	m_nShotsFired++;

	pPlayer->DoMuzzleFlash();

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	int iBulletsToFire = 0;
	float fireRate = GetFireRate();

	// MUST call sound before removing a round from the clip of a CHLMachineGun
	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		WeaponSound(SINGLE, m_flNextPrimaryAttack);
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		iBulletsToFire++;
	}

	// Make sure we don't fire more than the amount in the clip, if this weapon uses clips
	if ( UsesClipsForAmmo1() )
	{
		if ( iBulletsToFire > m_iClip1 )
			iBulletsToFire = m_iClip1;
		m_iClip1 -= iBulletsToFire;
	}

	m_iPrimaryAttacks++;
#ifndef CLIENT_DLL
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
#endif

	CHL2_Player *pHLPlayer = dynamic_cast<CHL2_Player *> (pPlayer);
	// Fire the bullets
	FireBulletsInfo_t info;
	info.m_iShots = iBulletsToFire;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition();
	info.m_vecDirShooting = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
	info.m_vecSpread = pHLPlayer->GetAttackSpread(this);
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
#ifdef EZ
	info.m_iTracerFreq = 1;
#else
	info.m_iTracerFreq = 2;
#endif

#ifdef CSS_WEAPONS_IN_HL2
	if (GetDamageMultiplier() != 1.0f)
	{
		info.m_flDamage = round( GetAmmoDef()->PlrDamage( info.m_iAmmoType ) * GetDamageMultiplier() );
		info.m_nFlags |= FIRE_BULLETS_NO_AUTO_GIB_TYPE;
	}
#endif
	FireBullets( info );

	//Factor in the view kick
	AddViewKick();
#ifndef CLIENT_DLL
#ifdef CSS_WEAPONS_IN_HL2
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), IsSilenced() ? SOUNDENT_VOLUME_MACHINEGUN / 3.0 : SOUNDENT_VOLUME_MACHINEGUN, 0.2, pPlayer );
#else
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pPlayer );
#endif
#endif
	
	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	SendWeaponAnim( GetPrimaryAttackActivity() );
	pPlayer->SetAnimation(PLAYER_ATTACK1);
#ifndef CLIENT_DLL
	// Register a muzzleflash for the AI
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
void CHLMachineGun::FireBullets( const FireBulletsInfo_t &info )
{
	if(CBasePlayer *pPlayer = ToBasePlayer ( GetOwner() ) )
	{
		pPlayer->FireBullets(info);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Weapon firing conditions
//-----------------------------------------------------------------------------
int CHLMachineGun::WeaponRangeAttack1Condition( float flDot, float flDist )
{
#ifndef CLIENT_DLL
	if ( m_iClip1 <=0 )
	{
		return COND_NO_PRIMARY_AMMO;
	}
	else if ( flDist < m_fMinRange1 ) 
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	else if ( flDist > m_fMaxRange1 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if ( flDot < 0.5f )	// UNDONE: Why check this here? Isn't the AI checking this already?
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLMachineGun::DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime )
{

	#define	KICK_MIN_X			0.2f	//Degrees
	#define	KICK_MIN_Y			0.2f	//Degrees
	#define	KICK_MIN_Z			0.1f	//Degrees

	QAngle vecScratch;
	
	//Find how far into our accuracy degradation we are
	float duration	= ( fireDurationTime > slideLimitTime ) ? slideLimitTime : fireDurationTime;
	float kickPerc = duration / slideLimitTime;

	// do this to get a hard discontinuity, clear out anything under 10 degrees punch
	pPlayer->ViewPunchReset( 10 );

	//Apply this to the view angles as well
	vecScratch.x = -( KICK_MIN_X + ( maxVerticleKickAngle * kickPerc ) );
	vecScratch.y = -( KICK_MIN_Y + ( maxVerticleKickAngle * kickPerc ) ) / 3;
	vecScratch.z = KICK_MIN_Z + ( maxVerticleKickAngle * kickPerc ) / 8;

	//Wibble left and right
	if ( random->RandomInt( -1, 1 ) >= 0 )
		vecScratch.y *= -1;

	//Wobble up and down
	if ( random->RandomInt( -1, 1 ) >= 0 )
		vecScratch.z *= -1;
#ifndef CLIENT_DLL
	//If we're in easy, dampen the effect a bit
	if ( g_pGameRules->IsSkillLevel( SKILL_EASY ) )
	{
		for ( int i = 0; i < 3; i++ )
		{
			vecScratch[i] *= dampEasy;
		}
	}
#endif
	//Clip this to our desired min/max
	UTIL_ClipPunchAngleOffset( vecScratch, pPlayer->m_Local.m_vecPunchAngle, QAngle( 24.0f, 3.0f, 1.0f ) );

	//Add it to the view punch
	// NOTE: 0.5 is just tuned to match the old effect before the punch became simulated
	pPlayer->ViewPunch( vecScratch * 0.5 );
}

//-----------------------------------------------------------------------------
// Purpose: Reset our shots fired
//-----------------------------------------------------------------------------
bool CHLMachineGun::Deploy( void )
{
	m_nShotsFired = 0;

	return BaseClass::Deploy();
}



//-----------------------------------------------------------------------------
// Purpose: Make enough sound events to fill the estimated think interval
// returns: number of shots needed
//-----------------------------------------------------------------------------
int CHLMachineGun::WeaponSoundRealtime( WeaponSound_t shoot_type )
{
#ifndef CLIENT_DLL
	int numBullets = 0;

	// ran out of time, clamp to current
	if (m_flNextSoundTime < gpGlobals->curtime)
	{
		m_flNextSoundTime = gpGlobals->curtime;
	}

	// make enough sound events to fill up the next estimated think interval
	float dt = clamp( m_flAnimTime - m_flPrevAnimTime, 0, 0.2 );
	if (m_flNextSoundTime < gpGlobals->curtime + dt)
	{
		WeaponSound( SINGLE_NPC, m_flNextSoundTime );
		m_flNextSoundTime += GetFireRate();
		numBullets++;
	}
	if (m_flNextSoundTime < gpGlobals->curtime + dt)
	{
		WeaponSound( SINGLE_NPC, m_flNextSoundTime );
		m_flNextSoundTime += GetFireRate();
		numBullets++;
	}

	return numBullets;
#else
	return 0;
#endif
}




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLMachineGun::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	// Debounce the recoiling counter
	if ( ( pOwner->m_nButtons & IN_ATTACK ) == false )
	{
		m_nShotsFired = 0;
	}

	BaseClass::ItemPostFrame();
}

IMPLEMENT_NETWORKCLASS_ALIASED(HLSelectFireMachineGun, DT_HLSelectFireMachineGun)

BEGIN_NETWORK_TABLE(CHLSelectFireMachineGun, DT_HLSelectFireMachineGun)
#ifndef CLIENT_DLL
SendPropInt(SENDINFO(m_iBurstSize)),
SendPropInt(SENDINFO(m_iFireMode)),
SendPropFloat(SENDINFO(m_flNextBurstShot)),
#else
RecvPropInt(RECVINFO(m_iBurstSize)),
RecvPropInt(RECVINFO(m_iFireMode)),
RecvPropFloat(RECVINFO(m_flNextBurstShot)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CHLSelectFireMachineGun)
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD(m_iBurstSize, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD_TOL(m_flNextBurstShot, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
#endif
END_PREDICTION_DATA()

//=========================================================
//	>> CHLSelectFireMachineGun
//=========================================================
BEGIN_DATADESC( CHLSelectFireMachineGun )

	DEFINE_FIELD( m_iBurstSize,		FIELD_INTEGER ),
	DEFINE_FIELD( m_iFireMode,		FIELD_INTEGER ),
	
	// Function pinters
#ifndef CLIENT_DLL
	DEFINE_FUNCTION( BurstThink ),
#endif

END_DATADESC()



float CHLSelectFireMachineGun::GetBurstCycleRate( void )
{
	// this is the time it takes to fire an entire 
	// burst, plus whatever amount of delay we want
	// to have between bursts.
	return 0.5f;
}

float CHLSelectFireMachineGun::GetFireRate( void )
{
	switch( m_iFireMode )
	{
	case FIREMODE_FULLAUTO:
		// the time between rounds fired on full auto
		return 0.1f;	// 600 rounds per minute = 0.1 seconds per bullet
		break;

	case FIREMODE_3RNDBURST:
		// the time between rounds fired within a single burst
		return 0.1f;	// 600 rounds per minute = 0.1 seconds per bullet
		break;

	default:
		return 0.1f;
		break;
	}
}

bool CHLSelectFireMachineGun::Deploy( void )
{
	// Forget about any bursts this weapon was firing when holstered
	m_iBurstSize = 0;
	return BaseClass::Deploy();
}

void CHLSelectFireMachineGun::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	if (m_iBurstSize > 0 && m_flNextBurstShot <= gpGlobals->curtime)
	{
		BurstThink();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CHLSelectFireMachineGun::PrimaryAttack( void )
{
	ITEM_GRAB_PREDICTED_ATTACK_FIX
	if (m_bFireOnEmpty)
	{
		return;
	}
	switch( m_iFireMode )
	{
	case FIREMODE_FULLAUTO:
		BaseClass::PrimaryAttack();
		// Msg("%.3f\n", m_flNextPrimaryAttack.Get() );
		SetWeaponIdleTime( gpGlobals->curtime + 3.0f );
		break;

	case FIREMODE_3RNDBURST:
		m_iBurstSize = GetBurstSize();
		
		// Call the think function directly so that the first round gets fired immediately.
		m_flNextBurstShot = gpGlobals->curtime;
		BurstThink();
		m_flNextPrimaryAttack = gpGlobals->curtime + GetBurstCycleRate();
		m_flNextSecondaryAttack = gpGlobals->curtime + GetBurstCycleRate();
		break;
	}

	//CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		m_iPrimaryAttacks++;
#ifndef CLIENT_DLL
		gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CHLSelectFireMachineGun::SecondaryAttack( void )
{
	ITEM_GRAB_PREDICTED_ATTACK_FIX
	// change fire modes.

	switch( m_iFireMode )
	{
	case FIREMODE_FULLAUTO:
		//Msg( "Burst\n" );
		m_iFireMode = FIREMODE_3RNDBURST;
		WeaponSound(SPECIAL2);
		break;

	case FIREMODE_3RNDBURST:
		//Msg( "Auto\n" );
		m_iFireMode = FIREMODE_FULLAUTO;
		WeaponSound(SPECIAL1);
		break;
	}
	
	SendWeaponAnim( GetSecondaryAttackActivity() );

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;

	//CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		m_iSecondaryAttacks++;
#ifndef CLIENT_DLL
		gamestats->Event_WeaponFired( pPlayer, false, GetClassname() );
#endif
#ifdef MAPBASE
		pPlayer->SetAnimation( PLAYER_ATTACK2 );
#endif
	}
}

void CHLSelectFireMachineGun::BurstAttack(void)
{
	ITEM_GRAB_PREDICTED_ATTACK_FIX

		// Abort here to handle burst and auto fire modes
		if ((UsesClipsForAmmo1() && m_iClip1 == 0) || (!UsesClipsForAmmo1() && !pPlayer->GetAmmoCount(m_iPrimaryAmmoType)))
		{
			m_iBurstSize = 0;
			return;
		}
			

	m_nShotsFired++;

	pPlayer->DoMuzzleFlash();

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	int iBulletsToFire = 0;
	float fireRate = GetFireRate();

	// MUST call sound before removing a round from the clip of a CHLMachineGun
	while (m_flNextBurstShot <= gpGlobals->curtime && m_iBurstSize>0)
	{
		WeaponSound(SINGLE, m_flNextBurstShot);
		m_flNextBurstShot = m_flNextBurstShot + fireRate;
		iBulletsToFire++;
		m_iBurstSize--;
	}

	// Make sure we don't fire more than the amount in the clip, if this weapon uses clips
	if (UsesClipsForAmmo1())
	{
		if (iBulletsToFire > m_iClip1)
			iBulletsToFire = m_iClip1;
		m_iClip1 -= iBulletsToFire;
	}

	m_iPrimaryAttacks++;
#ifndef CLIENT_DLL
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());
#endif

	CHL2_Player *pHLPlayer = dynamic_cast<CHL2_Player *> (pPlayer);
	// Fire the bullets
	FireBulletsInfo_t info;
	info.m_iShots = iBulletsToFire;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition();
	info.m_vecDirShooting = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
	info.m_vecSpread = pHLPlayer->GetAttackSpread(this);
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
#ifdef EZ
	info.m_iTracerFreq = 1;
#else
	info.m_iTracerFreq = 2;
#endif

#ifdef CSS_WEAPONS_IN_HL2
	if (GetDamageMultiplier() != 1.0f)
	{
		info.m_flDamage = round(GetAmmoDef()->PlrDamage(info.m_iAmmoType) * GetDamageMultiplier());
	}
#endif
	FireBullets(info);

	//Factor in the view kick
	AddViewKick();
#ifndef CLIENT_DLL
#ifdef CSS_WEAPONS_IN_HL2
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), IsSilenced() ? SOUNDENT_VOLUME_MACHINEGUN / 3.0 : SOUNDENT_VOLUME_MACHINEGUN, 0.2, pPlayer);
#else
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pPlayer);
#endif
#endif

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	SendWeaponAnim(GetPrimaryAttackActivity());
	pPlayer->SetAnimation(PLAYER_ATTACK1);
#ifndef CLIENT_DLL
	// Register a muzzleflash for the AI
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CHLSelectFireMachineGun::BurstThink( void )
{
	BurstAttack();

	if( m_iBurstSize <= 0 )
	{
		// idle immediately to stop the firing animation
		SetWeaponIdleTime( gpGlobals->curtime );
		return;
	}
	m_flNextBurstShot = gpGlobals->curtime + GetFireRate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CHLSelectFireMachineGun::WeaponSound( WeaponSound_t shoot_type, float soundtime /*= 0.0f*/ )
{
	if (shoot_type == SINGLE)
	{
		switch( m_iFireMode )
		{
		case FIREMODE_FULLAUTO:
			BaseClass::WeaponSound( SINGLE, soundtime );
			break;

		case FIREMODE_3RNDBURST:
			if( m_iBurstSize == GetBurstSize() && m_iClip1 >= m_iBurstSize )
			{
				// First round of a burst, and enough bullets remaining in the clip to fire the whole burst
				BaseClass::WeaponSound( BURST, soundtime );
			}
			else if( m_iClip1 < m_iBurstSize )
			{
				// Not enough rounds remaining in the magazine to fire a burst, so play the gunshot
				// sounds individually as each round is fired.
				BaseClass::WeaponSound( SINGLE, soundtime );
			}

			break;
		}
		return;
	}

	BaseClass::WeaponSound( shoot_type, soundtime );
}

// BUGBUG: These need to be rethought
//-----------------------------------------------------------------------------
int CHLSelectFireMachineGun::WeaponRangeAttack1Condition( float flDot, float flDist )
{
#ifndef CLIENT_DLL
	if (m_iClip1 <=0)
	{
		return COND_NO_PRIMARY_AMMO;
	}
	else if ( flDist < m_fMinRange1) 
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	else if (flDist > m_fMaxRange1) 
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.5)	// UNDONE: Why check this here? Isn't the AI checking this already?
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
int CHLSelectFireMachineGun::WeaponRangeAttack2Condition( float flDot, float flDist )
{
#ifndef CLIENT_DLL
	return COND_NONE; // FIXME: disabled for now

	// m_iClip2 == -1 when no secondary clip is used
	if ( m_iClip2 == 0 && UsesSecondaryAmmo() )
	{
		return COND_NO_SECONDARY_AMMO;
	}
	else if ( flDist < m_fMinRange2 )
	{
		// Don't return	COND_TOO_CLOSE_TO_ATTACK only for primary attack
		return COND_NONE;
	}
	else if (flDist > m_fMaxRange2 )
	{
		// Don't return COND_TOO_FAR_TO_ATTACK only for primary attack
		return COND_NONE;
	}
	else if ( flDot < 0.5 ) // UNDONE: Why check this here? Isn't the AI checking this already?
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK2;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHLSelectFireMachineGun::CHLSelectFireMachineGun( void )
{
	m_fMinRange1	= 65;
	m_fMinRange2	= 65;
	m_fMaxRange1	= 1024;
	m_fMaxRange2	= 1024;
	m_iFireMode		= FIREMODE_FULLAUTO;
}
