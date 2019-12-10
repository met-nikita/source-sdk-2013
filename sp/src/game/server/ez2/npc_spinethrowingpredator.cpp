//=============================================================================//
//
// Purpose: Predator that fires spiky projectiles at enemies.
//			Could be used to replicate the Pit Drone from Opposing Force
// Author: 1upD
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "NPC_SpineThrowingPredator.h"
#include "movevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_spinethrowingpredator_health( "sk_spinethrowingpredator_health", "100" );
ConVar sk_spinethrowingpredator_dmg_spit( "sk_spinethrowingpredator_dmg_spit", "33" );
ConVar sk_spinethrowingpredator_dmg_slash( "sk_spinethrowingpredator_dmg_slash", "33" );
ConVar sk_spinethrowingpredator_spit_gravity( "sk_spinethrowingpredator_spit_gravity", "600" );
ConVar sk_spinethrowingpredator_spit_arc_size( "sk_spinethrowingpredator_spit_arc_size", "3");

LINK_ENTITY_TO_CLASS( npc_spinethrowingpredator, CNPC_SpineThrowingPredator );
LINK_ENTITY_TO_CLASS( npc_pitdrone, CNPC_SpineThrowingPredator );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_SpineThrowingPredator )

	DEFINE_FIELD( m_nextSoundTime,	FIELD_TIME ),

END_DATADESC()

//=========================================================
// Spawn
//=========================================================
void CNPC_SpineThrowingPredator::Spawn()
{
	Precache( );

	SetModel( STRING( GetModelName() ) );

	SetHullType( HULL_MEDIUM );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );

	if (m_tEzVariant == EZ_VARIANT_RAD)
	{
		SetBloodColor( BLOOD_COLOR_BLUE );
	}
	else
	{
		SetBloodColor( BLOOD_COLOR_GREEN );
	}
	
	SetRenderColor( 255, 255, 255, 255 );
	
	m_iMaxHealth		= sk_spinethrowingpredator_health.GetFloat();
	m_iHealth			= m_iMaxHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;

	CapabilitiesClear();
	// For the purposes of melee attack conditions triggering attacks, we are treating the flying predator as though it has two melee attacks like the bullsquid.
	// In reality, the melee attack schedules will be translated to SCHED_RANGE_ATTACK_1.
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 | bits_CAP_SQUAD );

	m_fCanThreatDisplay	= TRUE;
	m_flNextSpitTime = gpGlobals->curtime;

	NPCInit();

	m_flDistTooFar		= 784;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_SpineThrowingPredator::Precache()
{

	if ( GetModelName() == NULL_STRING )
	{
		SetModelName( AllocPooledString( "models/pit_drone.mdl" ) );
	}

	PrecacheModel( STRING( GetModelName() ) );

	if (m_tEzVariant == EZ_VARIANT_RAD)
	{
		PrecacheParticleSystem( "blood_impact_blue_01" );
	}
	else
	{
		PrecacheParticleSystem( "blood_impact_yellow_01" );
	}

	PrecacheScriptSound( "NPC_SpineThrowingPredator.Idle" );
	PrecacheScriptSound( "NPC_SpineThrowingPredator.Pain" );
	PrecacheScriptSound( "NPC_SpineThrowingPredator.Alert" );
	PrecacheScriptSound( "NPC_SpineThrowingPredator.Death" );
	PrecacheScriptSound( "NPC_SpineThrowingPredator.Attack1" );
	PrecacheScriptSound( "NPC_SpineThrowingPredator.FoundEnemy" );
	PrecacheScriptSound( "NPC_SpineThrowingPredator.Growl" );
	PrecacheScriptSound( "NPC_SpineThrowingPredator.TailWhip");
	PrecacheScriptSound( "NPC_SpineThrowingPredator.Bite" );
	PrecacheScriptSound( "NPC_SpineThrowingPredator.Eat" );
	PrecacheScriptSound( "NPC_SpineThrowingPredator.Explode" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this monster's place in the relationship table.
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_SpineThrowingPredator::Classify( void )
{
	return CLASS_ALIEN_PREDATOR; 
}

//=========================================================
// IdleSound 
//=========================================================
void CNPC_SpineThrowingPredator::IdleSound( void )
{
	EmitSound( "NPC_SpineThrowingPredator.Idle" );
}

//=========================================================
// PainSound 
//=========================================================
void CNPC_SpineThrowingPredator::PainSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_SpineThrowingPredator.Pain" );
}

//=========================================================
// AlertSound
//=========================================================
void CNPC_SpineThrowingPredator::AlertSound( void )
{
	EmitSound( "NPC_SpineThrowingPredator.Alert" );
}

//=========================================================
// DeathSound
//=========================================================
void CNPC_SpineThrowingPredator::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_SpineThrowingPredator.Death" );
}

//=========================================================
// AttackSound
//=========================================================
void CNPC_SpineThrowingPredator::AttackSound( void )
{
	EmitSound( "NPC_SpineThrowingPredator.Attack1" );
}

//=========================================================
// FoundEnemySound
//=========================================================
void CNPC_SpineThrowingPredator::FoundEnemySound( void )
{
	if (gpGlobals->curtime >= m_nextSoundTime)
	{
		EmitSound( "NPC_SpineThrowingPredator.FoundEnemy" );
		m_nextSoundTime	= gpGlobals->curtime + random->RandomInt( 1.5, 3.0 );
	}
}

//=========================================================
// GrowlSound
//=========================================================
void CNPC_SpineThrowingPredator::GrowlSound( void )
{
	if (gpGlobals->curtime >= m_nextSoundTime)
	{
		EmitSound( "NPC_SpineThrowingPredator.Growl" );
		m_nextSoundTime	= gpGlobals->curtime + random->RandomInt(1.5,3.0);
	}
}

//=========================================================
// BiteSound
//=========================================================
void CNPC_SpineThrowingPredator::BiteSound( void )
{
	EmitSound( "NPC_SpineThrowingPredator.Bite" );
}

//=========================================================
// EatSound
//=========================================================
void CNPC_SpineThrowingPredator::EatSound( void )
{
	EmitSound( "NPC_SpineThrowingPredator.Eat" );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_SpineThrowingPredator::MaxYawSpeed( void )
{
	float flYS = 0;

	switch ( GetActivity() )
	{
	case	ACT_WALK:			flYS = 90;	break;
	case	ACT_RUN:			flYS = 90;	break;
	case	ACT_IDLE:			flYS = 90;	break;
	case	ACT_RANGE_ATTACK1:	flYS = 90;	break;
	default:
		flYS = 90;
		break;
	}

	return flYS;
}

//=========================================================
// RangeAttack1Conditions
//=========================================================
int CNPC_SpineThrowingPredator::RangeAttack1Conditions( float flDot, float flDist )
{
	// Be sure not to use ranged attacks against potential food
	if ( IsPrey( GetEnemy() ))
	{
		// Don't spit at prey - monch it!
		return ( COND_NONE );
	}

	return(BaseClass::RangeAttack1Conditions( flDot, flDist ));
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_SpineThrowingPredator::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
		case PREDATOR_AE_SPIT:
		{
			if ( GetEnemy() )
			{
				Vector vSpitPos;

				GetAttachment( "Mouth", vSpitPos );

				Vector			vTarget = GetEnemy()->GetAbsOrigin();
				Vector			vToss;
				CBaseEntity*	pBlocker;
				float flGravity  =  GetProjectileGravity();
				ThrowLimit( vSpitPos, vTarget, flGravity, GetProjectileArc(), Vector(0,0,0), Vector(0,0,0), GetEnemy(), &vToss, &pBlocker );

				// Create a new entity with CCrossbowBolt private data
				CBaseCombatCharacter *pBolt = (CBaseCombatCharacter *)CreateEntityByName( "crossbow_bolt" );
				UTIL_SetOrigin( pBolt, vSpitPos );
				DispatchSpawn( pBolt );
				pBolt->SetAbsAngles( vec3_angle );
				pBolt->SetOwnerEntity( this );
				pBolt->SetAbsVelocity( vToss );
				pBolt->SetDamage( GetProjectileDamge() );

				AttackSound();
			}
		}
		break;
		case PREDATOR_AE_BITE:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), GetBiteDamage(), DMG_SLASH | DMG_ALWAYSGIB );
			if (pHurt)
			{
				BiteSound(); // Only play the bite sound if we have a target

				Vector forward, up;
				AngleVectors( GetAbsAngles(), &forward, NULL, &up );
				pHurt->ApplyAbsVelocityImpulse( 100 * (up-forward) * GetModelScale() );
				pHurt->SetGroundEntity( NULL );
			}
		}
		break;

		case PREDATOR_AE_WHIP_SND:
		{
			EmitSound( "NPC_SpineThrowingPredator.TailWhip" );
			break;
		}

		case PREDATOR_AE_TAILWHIP:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), GetWhipDamage(), DMG_SLASH | DMG_ALWAYSGIB );
			if (pHurt)
			{
				Vector right, up;
				AngleVectors( GetAbsAngles(), NULL, &right, &up );

				if (pHurt->GetFlags() & (FL_NPC | FL_CLIENT))
					pHurt->ViewPunch( QAngle( 20, 0, -20 ) );

				pHurt->ApplyAbsVelocityImpulse( 100 * (up+2*right) * GetModelScale() );
			}
		}
		break;

		case PREDATOR_AE_BLINK:
		{
			// Close eye. 
			// Even skins are eyes open, odd are closed
			m_nSkin = ((m_nSkin / 2) * 2) + 1;
		}
		break;

		case PREDATOR_AE_HOP:
		{
			float flGravity = GetCurrentGravity();

			// throw the squid up into the air on this frame.
			if (GetFlags() & FL_ONGROUND)
			{
				SetGroundEntity( NULL );
			}

			// jump 40 inches into the air
			Vector vecVel = GetAbsVelocity();
			vecVel.z += sqrt( flGravity * 2.0 * 40 );
			SetAbsVelocity( vecVel );
		}
		break;

		default:
			BaseClass::HandleAnimEvent( pEvent );
	}
}

float CNPC_SpineThrowingPredator::GetProjectileGravity()
{
	return sk_spinethrowingpredator_spit_gravity.GetFloat();
}

float CNPC_SpineThrowingPredator::GetProjectileArc()
{
	return sk_spinethrowingpredator_spit_arc_size.GetFloat();
}

float CNPC_SpineThrowingPredator::GetProjectileDamge()
{
	return sk_spinethrowingpredator_dmg_spit.GetFloat();
}

//=========================================================
// Damage for claw slash attack
//=========================================================
float CNPC_SpineThrowingPredator::GetBiteDamage( void )
{
	return sk_spinethrowingpredator_dmg_slash.GetFloat();
}

//=========================================================
// Damage for "whip" claw attack
//=========================================================
float CNPC_SpineThrowingPredator::GetWhipDamage( void )
{
	return sk_spinethrowingpredator_dmg_slash.GetFloat();
}

//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( NPC_SpineThrowingPredator, CNPC_SpineThrowingPredator )

AI_END_CUSTOM_NPC()
