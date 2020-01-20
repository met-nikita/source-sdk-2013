//=============================================================================//
//
// Purpose:		Clone Cop, a former human bent on killing anyone who stands in his way.
//				He was trapped under Arbeit 1 for a long time (from his perspective),
//				but now he's back and he's bigger, badder, and possibly even more deranged than ever before.
//				I mean, you could just see the brains of this guy.
//
// Author:		Blixibon
//
//=============================================================================//

#include "cbase.h"

#include "npc_clonecop.h"
#include "gameweaponmanager.h"
#include "ammodef.h"
#include "grenade_hopwire.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_clonecop_health( "sk_clonecop_health","1000" );
ConVar	sk_clonecop_kick( "sk_clonecop_kick", "40" );

extern ConVar sk_plr_dmg_buckshot;
extern ConVar sk_plr_num_shotgun_pellets;

#define COMBINE_AE_GREN_DROP		( 9 )

LINK_ENTITY_TO_CLASS( npc_clonecop, CNPC_CloneCop );

CNPC_CloneCop::CNPC_CloneCop()
{
	// KV will override this if the NPC was spawned by Hammer
	AddGrenades( 10 );
	SetAlternateCapable( true );
	AddSpawnFlags( SF_COMBINE_REGENERATE );
	m_tEzVariant = EZ_VARIANT_RAD;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CloneCop::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );

	// Stronger, tougher.
	SetHealth( sk_clonecop_health.GetFloat() );
	SetMaxHealth( sk_clonecop_health.GetFloat() );
	SetKickDamage( sk_clonecop_kick.GetFloat() );

	AddEFlags( EFL_NO_DISSOLVE );

	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_MOVE_SHOOT );
	CapabilitiesAdd( bits_CAP_DOORS_GROUP );

	// We despise the player
	AddClassRelationship( CLASS_PLAYER, D_HT, 10 );

	BaseClass::Spawn();

	if ( !GetSquad() )
	{
		AddToSquad( MAKE_STRING( "cc_squad" ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_CloneCop::Precache()
{
	// For now, CC is always an elite
	m_fIsElite = true;

	if( !GetModelName() )
	{
		SetModelName( MAKE_STRING( "models/clone_cop.mdl" ) );
	}

	PrecacheModel( STRING( GetModelName() ) );

	UTIL_PrecacheOther( "item_ammo_ar2_altfire" );

	BaseClass::Precache();
}


void CNPC_CloneCop::DeathSound( const CTakeDamageInfo &info )
{
	AI_CriteriaSet set;
	ModifyOrAppendDamageCriteria(set, info);
	SpeakIfAllowed( TLK_CMB_DIE, set, SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Soldiers use CAN_RANGE_ATTACK2 to indicate whether they can throw
//			a grenade. Because they check only every half-second or so, this
//			condition must persist until it is updated again by the code
//			that determines whether a grenade can be thrown, so prevent the 
//			base class from clearing it out. (sjb)
//-----------------------------------------------------------------------------
void CNPC_CloneCop::ClearAttackConditions()
{
	bool fCanRangeAttack2 = HasCondition( COND_CAN_RANGE_ATTACK2 );

	// Call the base class.
	BaseClass::ClearAttackConditions();

	if( fCanRangeAttack2 )
	{
		// We don't allow the base class to clear this condition because we
		// don't sense for it every frame.
		SetCondition( COND_CAN_RANGE_ATTACK2 );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_CloneCop::SelectSchedule( void )
{
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_CloneCop::TranslateSchedule( int scheduleType )
{
	scheduleType = BaseClass::TranslateSchedule( scheduleType );

	switch (scheduleType)
	{
		case SCHED_COMBINE_ESTABLISH_LINE_OF_FIRE:
		{
			// Clone Cop always attempts to flank the player
			return SCHED_COMBINE_FLANK_LINE_OF_FIRE;
		} break;
	}

	return scheduleType;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPC_CloneCop::GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info )
{
	switch( iHitGroup )
	{
	case HITGROUP_HEAD:
		{
			// Soldiers take double headshot damage
			return 2.0f;
		}
	}

	return BaseClass::GetHitgroupDamageMultiplier( iHitGroup, info );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CloneCop::HandleAnimEvent( animevent_t *pEvent )
{
	bool handledEvent = false;

	if (pEvent->type & AE_TYPE_NEWEVENTSYSTEM)
	{
		BaseClass::HandleAnimEvent( pEvent );
	}
	else
	{
		switch( pEvent->event )
		{
		case COMBINE_AE_GREN_TOSS:
			{
				Vector vecSpin;
				vecSpin.x = random->RandomFloat( -1000.0, 1000.0 );
				vecSpin.y = random->RandomFloat( -1000.0, 1000.0 );
				vecSpin.z = random->RandomFloat( -1000.0, 1000.0 );

				Vector vecStart;
				GetAttachment( "lefthand", vecStart );

				if( m_NPCState == NPC_STATE_SCRIPT )
				{
					// Use a fixed velocity for grenades thrown in scripted state.
					// Grenades thrown from a script do not count against grenades remaining for the AI to use.
					Vector forward, up, vecThrow;

					GetVectors( &forward, NULL, &up );
					vecThrow = forward * 750 + up * 175;
					CBaseEntity *pGrenade = Fraggrenade_Create( vecStart, vec3_angle, vecThrow, vecSpin, this, COMBINE_GRENADE_TIMER, true );
					m_OnThrowGrenade.Set(pGrenade, pGrenade, this);
				}
				else
				{
					// Use the Velocity that AI gave us.
					CBaseEntity *pGrenade = Fraggrenade_Create( vecStart, vec3_angle, m_vecTossVelocity, vecSpin, this, COMBINE_GRENADE_TIMER, true );
					m_OnThrowGrenade.Set(pGrenade, pGrenade, this);
					AddGrenades(-1, pGrenade);
				}

				// wait six seconds before even looking again to see if a grenade can be thrown.
				m_flNextGrenadeCheck = gpGlobals->curtime + 6;
			}
			handledEvent = true;
			break;

		case COMBINE_AE_GREN_DROP:
			{
				Vector vecStart;
				QAngle angStart;
				m_vecTossVelocity.x = 15;
				m_vecTossVelocity.y = 0;
				m_vecTossVelocity.z = 0;

				GetAttachment( "lefthand", vecStart, angStart );

				CBaseEntity *pGrenade = NULL;
				if (m_NPCState == NPC_STATE_SCRIPT)
				{
					// While scripting, have the grenade face upwards like it was originally and also don't decrement grenade count.
					pGrenade = Fraggrenade_Create( vecStart, vec3_angle, m_vecTossVelocity, vec3_origin, this, COMBINE_GRENADE_TIMER, true );
				}
				else
				{
					pGrenade = Fraggrenade_Create( vecStart, angStart, m_vecTossVelocity, vec3_origin, this, COMBINE_GRENADE_TIMER, true );
					AddGrenades(-1);
				}

				// Well, technically we're not throwing, but...still.
				m_OnThrowGrenade.Set(pGrenade, pGrenade, this);
			}
			handledEvent = true;
			break;

		default:
			BaseClass::HandleAnimEvent( pEvent );
			break;
		}
	}

	if( handledEvent )
	{
		m_iLastAnimEventHandled = pEvent->event;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CNPC_CloneCop::Event_Killed( const CTakeDamageInfo &info )
{
	if( IsElite() )
	{
		if ( HasSpawnFlags( SF_COMBINE_NO_AR2DROP ) == false )
		{
			CBaseEntity *pItem;
			if (GetActiveWeapon() && FClassnameIs(GetActiveWeapon(), "weapon_smg1"))
				pItem = DropItem( "item_ammo_smg1_grenade", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
			else
				pItem = DropItem( "item_ammo_ar2_altfire", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );

			if ( pItem )
			{
				IPhysicsObject *pObj = pItem->VPhysicsGetObject();

				if ( pObj )
				{
					Vector			vel		= RandomVector( -64.0f, 64.0f );
					AngularImpulse	angImp	= RandomAngularImpulse( -300.0f, 300.0f );

					vel[2] = 0.0f;
					pObj->AddVelocity( &vel, &angImp );
				}

				if( info.GetDamageType() & DMG_DISSOLVE )
				{
					CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating*>(pItem);

					if( pAnimating )
					{
						pAnimating->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
					}
				}
				else
				{
					WeaponManager_AddManaged( pItem );
				}
			}
		}
	}

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: Clone Cop only uses gesture flinches.
//-----------------------------------------------------------------------------
Activity CNPC_CloneCop::GetFlinchActivity( bool bHeavyDamage, bool bGesture )
{
	if (!bGesture)
		return ACT_INVALID;

	return BaseClass::GetFlinchActivity( bHeavyDamage, bGesture );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CloneCop::IsHeavyDamage( const CTakeDamageInfo &info )
{
	// Combine considers AR2 fire to be heavy damage
	if ( info.GetAmmoType() == GetAmmoDef()->Index("AR2") )
		return true;

	// 357 rounds are heavy damage
	if ( info.GetAmmoType() == GetAmmoDef()->Index("357") )
		return true;

	// Shotgun blasts where at least half the pellets hit me are heavy damage
	if ( info.GetDamageType() & DMG_BUCKSHOT )
	{
		int iHalfMax = sk_plr_dmg_buckshot.GetFloat() * sk_plr_num_shotgun_pellets.GetInt() * 0.5;
		if ( info.GetDamage() >= iHalfMax )
			return true;
	}

	// Rollermine shocks
	if( (info.GetDamageType() & DMG_SHOCK) && hl2_episodic.GetBool() )
	{
		return true;
	}

	return BaseClass::IsHeavyDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: Translate base class activities
//-----------------------------------------------------------------------------
Activity CNPC_CloneCop::Weapon_TranslateActivity( Activity eNewActivity )
{
	if ( HasCondition(COND_HEAVY_DAMAGE) && IsCurSchedule(SCHED_TAKE_COVER_FROM_ENEMY) )
	{
		// When we have heavy damage and we're taking cover from an enemy,
		// try using crouch activities
		switch (eNewActivity)
		{
		case ACT_IDLE:		eNewActivity = ACT_RANGE_AIM_LOW; break;
		case ACT_WALK:		eNewActivity = ACT_WALK_CROUCH; break;
		case ACT_RUN:		eNewActivity = ACT_RUN_CROUCH; break;
		}
	}

	return BaseClass::Weapon_TranslateActivity( eNewActivity );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
WeaponProficiency_t CNPC_CloneCop::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	// TODO: Different proficiency?
	//if( pWeapon->ClassMatches( gm_isz_class_AR2 ) )
	//{
	//	return WEAPON_PROFICIENCY_VERY_GOOD;
	//}
	//else if( pWeapon->ClassMatches( gm_isz_class_Shotgun ) )
	//{
	//	return WEAPON_PROFICIENCY_PERFECT;
	//}
	//else if( pWeapon->ClassMatches( gm_isz_class_SMG1 ) )
	//{
	//	return WEAPON_PROFICIENCY_VERY_GOOD;
	//}
	//else if (FClassnameIs( pWeapon, "weapon_smg2" ))
	//{
	//	return WEAPON_PROFICIENCY_VERY_GOOD;
	//}

	return BaseClass::CalcWeaponProficiency( pWeapon );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if a reasonable jumping distance
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CNPC_CloneCop::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const
{
	const float MAX_JUMP_RISE = 96.0f; // How high CC can jump; Default 64
	const float MAX_JUMP_DISTANCE = 384.0f; // How far CC can jump; Default 384
	const float MAX_JUMP_DROP = 384.0f; // How far CC can fall; Default 160

	return BaseClass::IsJumpLegal(startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE);
}
