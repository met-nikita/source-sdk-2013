//=============================================================================//
//
// Purpose: Flying enemy with similar behaviors to the bullsquid.
//			Fires spiky projectiles at enemies.
// Author: 1upD
//
//=============================================================================//

#include "cbase.h"
#include "ai_squad.h"
#include "npcevent.h"
#include "npc_flyingpredator.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_flyingpredator_health( "sk_flyingpredator_health", "100" );
ConVar sk_flyingpredator_dmg_spit( "sk_flyingpredator_dmg_spit", "33" );
ConVar sk_flyingpredator_spit_gravity( "sk_flyingpredator_spit_gravity", "600" );
ConVar sk_flyingpredator_spit_arc_size( "sk_flyingpredator_spit_arc_size", "3");
ConVar sk_flyingpredator_spit_min_wait( "sk_flyingpredator_spit_min_wait", "0.5");
ConVar sk_flyingpredator_spit_max_wait( "sk_flyingpredator_spit_max_wait", "5");
ConVar sk_flyingpredator_gestation( "sk_flyingpredator_gestation", "15.0" );
ConVar sk_flyingpredator_spawn_time( "sk_flyingpredator_spawn_time", "5.0" );

LINK_ENTITY_TO_CLASS( npc_flyingpredator, CNPC_FlyingPredator );
LINK_ENTITY_TO_CLASS( npc_stukabat, CNPC_FlyingPredator );

//---------------------------------------------------------
// Activities
//---------------------------------------------------------
int ACT_EAT;
int ACT_PREDATOR_DETECT_SCENT;

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_FlyingPredator )

	DEFINE_FIELD( m_nextSoundTime,	FIELD_TIME ),

END_DATADESC()

//=========================================================
// Spawn
//=========================================================
void CNPC_FlyingPredator::Spawn()
{
	Precache( );

	SetModel( STRING( GetModelName() ) );

	SetHullType( HULL_WIDE_SHORT );
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
	
	m_iMaxHealth		= sk_flyingpredator_health.GetFloat();
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
void CNPC_FlyingPredator::Precache()
{

	if ( GetModelName() == NULL_STRING )
	{
		SetModelName( AllocPooledString( "models/stukabat.mdl" ) );
	}

	PrecacheModel( STRING( GetModelName() ) );

	UTIL_PrecacheOther( "grenade_spit" );

	if (m_tEzVariant == EZ_VARIANT_RAD)
	{
		PrecacheParticleSystem( "blood_impact_blue_01" );
	}
	else
	{
		PrecacheParticleSystem( "blood_impact_yellow_01" );
	}

	// Use this gib particle system to show baby squids 'molting'
	PrecacheParticleSystem( "bullsquid_explode" );

	PrecacheScriptSound( "NPC_FlyingPredator.Idle" );
	PrecacheScriptSound( "NPC_FlyingPredator.Pain" );
	PrecacheScriptSound( "NPC_FlyingPredator.Alert" );
	PrecacheScriptSound( "NPC_FlyingPredator.Death" );
	PrecacheScriptSound( "NPC_FlyingPredator.Attack1" );
	PrecacheScriptSound( "NPC_FlyingPredator.FoundEnemy" );
	PrecacheScriptSound( "NPC_FlyingPredator.Growl" );
	PrecacheScriptSound( "NPC_FlyingPredator.TailWhip");
	PrecacheScriptSound( "NPC_FlyingPredator.Bite" );
	PrecacheScriptSound( "NPC_FlyingPredator.Eat" );
	PrecacheScriptSound( "NPC_FlyingPredator.Explode" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this monster's place in the relationship table.
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_FlyingPredator::Classify( void )
{
	// TODO Replace this with CLASS_ALIEN_PREDATOR after setting up the relationship table
	return CLASS_BULLSQUID; 
}

//=========================================================
// IdleSound 
//=========================================================
void CNPC_FlyingPredator::IdleSound( void )
{
	EmitSound( "NPC_FlyingPredator.Idle" );
}

//=========================================================
// PainSound 
//=========================================================
void CNPC_FlyingPredator::PainSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_FlyingPredator.Pain" );
}

//=========================================================
// AlertSound
//=========================================================
void CNPC_FlyingPredator::AlertSound( void )
{
	EmitSound( "NPC_FlyingPredator.Alert" );
}

//=========================================================
// DeathSound
//=========================================================
void CNPC_FlyingPredator::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_FlyingPredator.Death" );
}

//=========================================================
// AttackSound
//=========================================================
void CNPC_FlyingPredator::AttackSound( void )
{
	EmitSound( "NPC_FlyingPredator.Attack1" );
}

//=========================================================
// FoundEnemySound
//=========================================================
void CNPC_FlyingPredator::FoundEnemySound( void )
{
	if (gpGlobals->curtime >= m_nextSoundTime)
	{
		EmitSound( "NPC_FlyingPredator.FoundEnemy" );
		m_nextSoundTime	= gpGlobals->curtime + random->RandomInt( 1.5, 3.0 );
	}
}

//=========================================================
// GrowlSound
//=========================================================
void CNPC_FlyingPredator::GrowlSound( void )
{
	if (gpGlobals->curtime >= m_nextSoundTime)
	{
		EmitSound( "NPC_FlyingPredator.Growl" );
		m_nextSoundTime	= gpGlobals->curtime + random->RandomInt(1.5,3.0);
	}
}

//=========================================================
// BiteSound
//=========================================================
void CNPC_FlyingPredator::BiteSound( void )
{
	EmitSound( "NPC_FlyingPredator.Bite" );
}

//=========================================================
// EatSound
//=========================================================
void CNPC_FlyingPredator::EatSound( void )
{
	EmitSound( "NPC_FlyingPredator.Eat" );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_FlyingPredator::MaxYawSpeed( void )
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
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_FlyingPredator::HandleAnimEvent( animevent_t *pEvent )
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
				float flGravity  =  sk_flyingpredator_spit_gravity.GetFloat();
				ThrowLimit( vSpitPos, vTarget, flGravity, sk_flyingpredator_spit_arc_size.GetFloat(), Vector(0,0,0), Vector(0,0,0), GetEnemy(), &vToss, &pBlocker );

				// Create a new entity with CCrossbowBolt private data
				CBaseCombatCharacter *pBolt = (CBaseCombatCharacter *)CreateEntityByName( "crossbow_bolt" );
				UTIL_SetOrigin( pBolt, vSpitPos );
				DispatchSpawn( pBolt );
				pBolt->SetAbsAngles( vec3_angle );
				pBolt->SetOwnerEntity( this );
				pBolt->SetAbsVelocity( vToss );
				pBolt->SetDamage( sk_flyingpredator_dmg_spit.GetFloat() );

				AttackSound();
			}
		}
		break;
		default:
			BaseClass::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Delays for next bullsquid spit time
//=========================================================
float CNPC_FlyingPredator::GetMaxSpitWaitTime( void )
{
	return sk_flyingpredator_spit_max_wait.GetFloat();
}

float CNPC_FlyingPredator::GetMinSpitWaitTime( void )
{
	return sk_flyingpredator_spit_max_wait.GetFloat();
}

//=========================================================
// Translate missing activities to custom ones
//=========================================================
Activity CNPC_FlyingPredator::NPC_TranslateActivity( Activity eNewActivity )
{
	if ( eNewActivity == ACT_HOP )
	{
		return (Activity) ACT_PREDATOR_DETECT_SCENT;
	}

	return BaseClass::NPC_TranslateActivity( eNewActivity );
}

//=========================================================
// Schedule translation
//=========================================================
int CNPC_FlyingPredator::TranslateSchedule( int scheduleType )
{
	switch (scheduleType)
	{
	// npc_flyingpredator treats ranged attacks as melee attacks so that the melee attack conditions can be used as intterupts
	case SCHED_MELEE_ATTACK1:
	case SCHED_MELEE_ATTACK2:
		return SCHED_RANGE_ATTACK1;
	default:
		return BaseClass::TranslateSchedule( scheduleType );
	}
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule. 
//
// Overridden for bullsquids to play specific activities
//=========================================================
void CNPC_FlyingPredator::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_PREDATOR_PLAY_EAT_ACT:
		EatSound();
		SetIdealActivity( (Activity) ACT_EAT );
		break;
	case TASK_PREDATOR_PLAY_EXCITE_ACT:
	case TASK_PREDATOR_PLAY_SNIFF_ACT:
	case TASK_PREDATOR_PLAY_INSPECT_ACT:
		SetIdealActivity( (Activity) ACT_PREDATOR_DETECT_SCENT );
		break;
	case TASK_PREDATOR_SPAWN:
		m_flNextSpawnTime = gpGlobals->curtime + sk_flyingpredator_spawn_time.GetFloat();
		SetIdealActivity( (Activity) ACT_RANGE_ATTACK1 );
		break;
	default:
	{
		BaseClass::StartTask( pTask );
		break;
	}
	}
}

//=========================================================
// RunTask
//=========================================================
void CNPC_FlyingPredator::RunTask ( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_PREDATOR_SPAWN:
	{
		// If we fall in this case, end the task when the activity ends
		if (IsActivityFinished())
		{
			TaskComplete();
		}
		break;
	}
	default:
	{
		BaseClass::RunTask( pTask );
		break;
	}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_FlyingPredator::CorpseGib( const CTakeDamageInfo &info )
{
	ExplosionEffect();

	// TODO flying predators might need unique gibs
	return BaseClass::CorpseGib( info );
}

void CNPC_FlyingPredator::ExplosionEffect( void )
{
	DispatchParticleEffect( "bullsquid_explode", WorldSpaceCenter(), GetAbsAngles() );
	EmitSound( "NPC_FlyingPredator.Explode" );
}

//-----------------------------------------------------------------------------
// Purpose: Create a new flying predator / stukabat
// Output : True if the new offspring is created
//
// TODO Stukabats should lay eggs
//-----------------------------------------------------------------------------
bool CNPC_FlyingPredator::SpawnNPC( const Vector position )
{
	// Try to create entity
	CNPC_FlyingPredator *pChild = dynamic_cast< CNPC_FlyingPredator * >(CreateEntityByName( "NPC_FlyingPredator" ));
	if ( pChild )
	{
		pChild->m_bIsBaby = true;
		pChild->m_tEzVariant = this->m_tEzVariant;
		pChild->m_tWanderState = this->m_tWanderState;
		pChild->m_bSpawningEnabled = true;
		pChild->SetModelName( this->GetModelName() );
		pChild->m_nSkin = this->m_nSkin;
		pChild->Precache();

		DispatchSpawn( pChild );

		// Now attempt to drop into the world
		pChild->Teleport( &position, NULL, NULL );
		UTIL_DropToFloor( pChild, MASK_NPCSOLID );

		// Now check that this is a valid location for the new npc to be
		Vector	vUpBit = pChild->GetAbsOrigin();
		vUpBit.z += 1;

		trace_t tr;
		AI_TraceHull( pChild->GetAbsOrigin(), vUpBit, pChild->GetHullMins(), pChild->GetHullMaxs(),
			MASK_NPCSOLID, pChild, COLLISION_GROUP_NONE, &tr );
		if (tr.startsolid || (tr.fraction < 1.0))
		{
			pChild->SUB_Remove();
			DevMsg( "Can't create baby stukabat. Bad Position!\n" );
			return false;
		}

		pChild->SetSquad( this->GetSquad() );
		pChild->Activate();

		// Decrement feeding counter
		m_iTimesFed--;
		if ( m_iTimesFed <= 0 )
		{
			m_bReadyToSpawn = false;
		}

		// Fire output
		variant_t value;
		value.SetEntity( pChild );
		m_OnSpawnNPC.CBaseEntityOutput::FireOutput( value, this, this );

		return true;
	}

	// Couldn't instantiate NPC
	return false;
}

//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( NPC_FlyingPredator, CNPC_FlyingPredator )

	DECLARE_ACTIVITY( ACT_EAT )
	DECLARE_ACTIVITY( ACT_PREDATOR_DETECT_SCENT )

AI_END_CUSTOM_NPC()
