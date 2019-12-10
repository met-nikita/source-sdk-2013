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
ConVar sk_flyingpredator_dmg_dive( "sk_flyingpredator_dmg_dive", "33" );
ConVar sk_flyingpredator_spit_gravity( "sk_flyingpredator_spit_gravity", "600" );
ConVar sk_flyingpredator_spit_arc_size( "sk_flyingpredator_spit_arc_size", "3");
ConVar sk_flyingpredator_spit_min_wait( "sk_flyingpredator_spit_min_wait", "0.5");
ConVar sk_flyingpredator_spit_max_wait( "sk_flyingpredator_spit_max_wait", "1");
ConVar sk_flyingpredator_gestation( "sk_flyingpredator_gestation", "15.0" );
ConVar sk_flyingpredator_spawn_time( "sk_flyingpredator_spawn_time", "5.0" );

LINK_ENTITY_TO_CLASS( npc_flyingpredator, CNPC_FlyingPredator );
LINK_ENTITY_TO_CLASS( npc_stukabat, CNPC_FlyingPredator );

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_TAKEOFF = LAST_SHARED_PREDATOR_TASK + 1,
	TASK_FLY_DIVE_BOMB,
	TASK_LAND,
	TASK_START_FLYING,
	TASK_START_FALLING
};

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_TAKEOFF = LAST_SHARED_SCHEDULE_PREDATOR + 1,
	SCHED_TAKEOFF_CEILING,
	SCHED_FLY_DIVE_BOMB,
	SCHED_LAND,
	SCHED_FALL
};

//-----------------------------------------------------------------------------
// monster-specific Conditions
//-----------------------------------------------------------------------------
enum
{
	COND_FORCED_FLY	= NEXT_CONDITION + 1,
	COND_CAN_LAND
};

//---------------------------------------------------------
// Activities
//---------------------------------------------------------
int ACT_FALL;
int ACT_IDLE_CEILING;
int ACT_LAND_CEILING;

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_FlyingPredator )

	DEFINE_FIELD( m_nextSoundTime,	FIELD_TIME ),
	DEFINE_KEYFIELD( m_tFlyState, FIELD_INTEGER, "FlyState"),

	DEFINE_FIELD( m_vecDiveBombDirection, FIELD_VECTOR ),
	DEFINE_FIELD( m_flDiveBombRollForce, FIELD_FLOAT ),

	DEFINE_INPUTFUNC( FIELD_STRING, "ForceFlying", InputForceFlying ),
	DEFINE_INPUTFUNC( FIELD_STRING, "Fly", InputFly ),

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
	m_flFieldOfView		= 0.75;	// indicates the width of this monster's forward view cone ( as a dotproduct result )
								// Stukabats have a wider FOV because in testing they tended to have too much tunnel vision.
								// It makes sense since they have six eyes, three on either side - they have a very wide view
	m_NPCState			= NPC_STATE_NONE;

	CapabilitiesClear();
	// For the purposes of melee attack conditions triggering attacks, we are treating the flying predator as though it has two melee attacks like the bullsquid.
	// In reality, the melee attack schedules will be translated to SCHED_RANGE_ATTACK_1.
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 | bits_CAP_SQUAD );

	m_fCanThreatDisplay	= TRUE;
	m_flNextSpitTime = gpGlobals->curtime;

	NPCInit();

	m_flDistTooFar		= 1024;

	// Reset flying state
	SetFlyingState( m_tFlyState );
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
int CNPC_FlyingPredator::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist <= m_flDistTooFar && flDot >= 0.5 && gpGlobals->curtime >= m_flNextSpitTime)
	{
		m_flNextSpitTime = gpGlobals->curtime + GetMinSpitWaitTime();
		return( COND_CAN_RANGE_ATTACK1 );
	}

	return( COND_NONE );
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
// Gather custom conditions
//=========================================================
void CNPC_FlyingPredator::GatherConditions( void )
{
	BaseClass::GatherConditions();

	switch (m_tFlyState)
	{
	case FlyState_Falling:
		if( CanLand() )
		{
			SetCondition( COND_CAN_LAND );
		}
		break;
	}
}

bool CNPC_FlyingPredator::CanLand() {
	trace_t tr;
	Vector checkPos = GetAbsOrigin() - Vector( 0, 0, 32 );
	AI_TraceLine( GetAbsOrigin(), checkPos, MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
	return tr.fraction < 1.0f;
}

// Shared activities from base predator
extern int ACT_EXCITED;
extern int ACT_DETECT_SCENT;
extern int ACT_INSPECT_FLOOR;

//=========================================================
// Translate missing activities to custom ones
//=========================================================
Activity CNPC_FlyingPredator::NPC_TranslateActivity( Activity eNewActivity )
{
	switch (m_tFlyState)
	{
	case FlyState_Dive:
		return ACT_RANGE_ATTACK2;
	case FlyState_Flying:
		if ( eNewActivity == ACT_WALK || eNewActivity == ACT_RUN )
			return ACT_FLY;
		else if ( eNewActivity == ACT_IDLE )
			return ACT_HOVER;
		else if ( eNewActivity == ACT_HOP )
			return ACT_HOVER;
		else if ( eNewActivity == ACT_INSPECT_FLOOR )
			return ACT_HOVER;
		else if ( eNewActivity == ACT_EXCITED )
			return ACT_HOVER;
		break;
	case FlyState_Falling:
		return (Activity) ACT_FALL;
	case FlyState_Ceiling:
		if ( eNewActivity == ACT_LEAP )
			return (Activity) ACT_FALL;
		if ( eNewActivity == ACT_IDLE )
			return ( Activity )ACT_IDLE_CEILING;
		if (eNewActivity == ACT_LAND)
			return (Activity) ACT_LAND_CEILING;
		else if (eNewActivity == ACT_HOP)
			return (Activity) ACT_IDLE_CEILING;
		else if (eNewActivity == ACT_INSPECT_FLOOR)
			return (Activity) ACT_IDLE_CEILING;
		else if (eNewActivity == ACT_EXCITED)
			return (Activity) ACT_IDLE_CEILING;
		break;
	default:
		if ( eNewActivity == ACT_HOP )
		{
			return (Activity) ACT_DETECT_SCENT;
		}
		else if ( eNewActivity == ACT_INSPECT_FLOOR )
		{
			return (Activity) ACT_DETECT_SCENT;
		}
		else if ( eNewActivity == ACT_EXCITED )
		{
			return (Activity) ACT_DETECT_SCENT;
		}
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
	case SCHED_RANGE_ATTACK1:
		if ( m_tFlyState == FlyState_Flying )
		{
			return SCHED_FLY_DIVE_BOMB;
		} 
		else
		{
			return SCHED_TAKEOFF;
		}
		break;
	case SCHED_TAKEOFF:
		if ( m_tFlyState == FlyState_Ceiling )
		{
			return SCHED_TAKEOFF_CEILING;
		}
		break;
	case SCHED_PREDATOR_RUN_EAT:
	case SCHED_PRED_SNIFF_AND_EAT:
	case SCHED_PREDATOR_EAT:
	case SCHED_PREDATOR_WALLOW:
		if ( m_tFlyState == FlyState_Ceiling || m_tFlyState == FlyState_Flying )
		{
			return SCHED_FALL;
		}
		break;
	case SCHED_PREDATOR_WANDER:
		if ( m_tFlyState == FlyState_Flying)
		{
			return SCHED_FALL;
		}
	case SCHED_CHASE_ENEMY:
	case SCHED_CHASE_ENEMY_FAILED:
	case SCHED_STANDOFF:
		if ( m_tFlyState == FlyState_Ceiling )
		{
			return SCHED_IDLE_STAND;
		}
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//=========================================================
// GetSchedule 
//=========================================================
int CNPC_FlyingPredator::SelectSchedule( void )
{
	if ( HasCondition( COND_FORCED_FLY ) )
	{
		return SCHED_TAKEOFF;
	}

	if ( HasCondition( COND_CAN_LAND ) )
	{
		return SCHED_LAND;
	}

	switch (m_tFlyState)
	{
	case FlyState_Dive:
		if (HasCondition( COND_HEAVY_DAMAGE ))
		{
			return SCHED_FALL;
		}

		return SCHED_FLY_DIVE_BOMB;
	case FlyState_Falling:
		return SCHED_FALL;
	case FlyState_Ceiling:
		// If anything 'disturbs' the stukabat, fall from this perch
		if ( ( GetEnemy() != NULL && EnemyDistance( GetEnemy() ) <= 512.0f ) || HasCondition( COND_HEAR_BULLET_IMPACT ) || HasCondition( COND_HEAR_DANGER ) || HasCondition( COND_HEAR_COMBAT ) || HasCondition( COND_PREDATOR_SMELL_FOOD ) || HasCondition( COND_SMELL ))
		{
			return SCHED_FALL;
		}

		return SCHED_IDLE_STAND;
	}

	return BaseClass::SelectSchedule();
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
	case TASK_TAKEOFF:
		SetFlyingState( FlyState_Flying );
		SetAbsOrigin( GetAbsOrigin() + Vector( 0, 0, 8 ) );
		SetAbsVelocity( Vector( 0, 0, 64.0f ) );
		TaskComplete();
		break;
	case TASK_PREDATOR_SPAWN:
		m_flNextSpawnTime = gpGlobals->curtime + sk_flyingpredator_spawn_time.GetFloat();
		SetIdealActivity( (Activity) ACT_RANGE_ATTACK1 );
		break;
	case TASK_FLY_DIVE_BOMB:
	{
		// Pick a direction to divebomb.
		if (GetEnemy() != NULL)
		{
			// Fly towards my enemy
			Vector vEnemyPos = GetEnemyLKP();
			m_vecDiveBombDirection = vEnemyPos - GetLocalOrigin();
		}
		else
		{
			// Pick a random forward and down direction.
			Vector forward;
			GetVectors( &forward, NULL, NULL );
			m_vecDiveBombDirection = forward + Vector( random->RandomFloat( -10, 10 ), random->RandomFloat( -10, 10 ), random->RandomFloat( -20, -10 ) );
		}
		VectorNormalize( m_vecDiveBombDirection );

		// Calculate a roll force.
		m_flDiveBombRollForce = random->RandomFloat( 20.0, 420.0 );
		if (random->RandomInt( 0, 1 ))
		{
			m_flDiveBombRollForce *= -1;
		}

		SetFlyingState( FlyState_Dive );
		AttackSound();
		SetIdealActivity( ACT_RANGE_ATTACK2 );
		break;
	}
	case TASK_LAND:
		SetIdealActivity( ACT_LAND );
		SetFlyingState( FlyState_Landing );
		break;
	case TASK_START_FLYING:
		// Just in case we took off, set velocity to 0
		SetAbsVelocity( vec3_origin );
		SetFlyingState( FlyState_Flying );
		TaskComplete();
		break;
	case TASK_START_FALLING:
		// If already falling, do nothing
		if ( m_tFlyState != FlyState_Falling )
		{
			// Just in case we took off, set velocity to 0
			SetAbsVelocity( vec3_origin );
			SetFlyingState( FlyState_Falling );
		}
		TaskComplete();
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
	switch ( pTask->iTask )
	{
	case TASK_FLY_DIVE_BOMB:
		if ( m_tFlyState != FlyState_Dive )
		{
			TaskComplete();
		}

		break;
	case TASK_PREDATOR_SPAWN:
	{
		// If we fall in this case, end the task when the activity ends
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;
	}
	case TASK_LAND:
		if ( IsActivityFinished() )
		{
			SetFlyingState( FlyState_Walking );
			TaskComplete();
		}
		break;
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

	CTakeDamageInfo info( this, this, sk_flyingpredator_dmg_spit.GetFloat(), DMG_BLAST_SURFACE | DMG_ACID | DMG_ALWAYSGIB );

	RadiusDamage( info, GetAbsOrigin(), 160, CLASS_NONE, this );
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

//-----------------------------------------------------------------------------
// Purpose: Return true if this NPC can hear the specified sound
//-----------------------------------------------------------------------------
bool CNPC_FlyingPredator::QueryHearSound( CSound *pSound )
{
	// Don't smell dead stukabats
	if ( pSound->FIsScent() && IsSameSpecies( pSound->m_hTarget ) )
		return false;

	return BaseClass::QueryHearSound( pSound );
}

//-----------------------------------------------------------------------------
// Purpose: Switches between flying mode and ground mode.
//-----------------------------------------------------------------------------
void CNPC_FlyingPredator::SetFlyingState( FlyState_t eState )
{
	m_tFlyState = eState;

	if (eState == FlyState_Flying || eState == FlyState_Dive )
	{
		// Flying
		SetGroundEntity( NULL );
		AddFlag( FL_FLY );
		SetNavType( NAV_FLY );
		CapabilitiesRemove( bits_CAP_MOVE_GROUND );
		CapabilitiesAdd( bits_CAP_MOVE_FLY );
		SetMoveType( MOVETYPE_STEP );
	}
	else if ( eState == FlyState_Ceiling )
	{
		// Ceiling
		SetGroundEntity( NULL );
		AddFlag( FL_FLY );
		SetNavType( NAV_FLY );
		CapabilitiesRemove( bits_CAP_MOVE_GROUND | bits_CAP_MOVE_FLY );
		SetMoveType( MOVETYPE_STEP );
	}
	else if (eState == FlyState_Walking)
	{
		// Walking
		QAngle angles = GetAbsAngles();
		angles[PITCH] = 0.0f;
		angles[ROLL] = 0.0f;
		SetAbsAngles( angles );

		RemoveFlag( FL_FLY );
		SetNavType( NAV_GROUND );
		CapabilitiesRemove( bits_CAP_MOVE_FLY );
		CapabilitiesAdd( bits_CAP_MOVE_GROUND );
		SetMoveType( MOVETYPE_STEP );
	}
	else
	{
		// Falling
		RemoveFlag( FL_FLY );
		SetNavType( NAV_GROUND );
		CapabilitiesRemove( bits_CAP_MOVE_FLY );
		CapabilitiesAdd( bits_CAP_MOVE_GROUND );
		SetMoveType( MOVETYPE_STEP );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_FlyingPredator::OverrideMove( float flInterval )
{
	// ----------------------------------------------
	//	If dive bombing
	// ----------------------------------------------
	if ( m_tFlyState == FlyState_Dive )
	{
		MoveToDivebomb( flInterval );
		return true;
	}

	return BaseClass::OverrideMove( flInterval );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Copied from Scanner
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CNPC_FlyingPredator::MoveToDivebomb( float flInterval )
{
	float myAccel = 3200;
	float myDecay = 0.05f; // decay current velocity to 10% in 1 second

	// Fly towards my enemy
	Vector vEnemyPos = GetEnemyLKP();
	Vector vFlyDirection  = vEnemyPos - GetLocalOrigin();
	VectorNormalize( vFlyDirection );

	// Set net velocity 
	MoveInDirection( flInterval, m_vecDiveBombDirection, myAccel, myAccel, myDecay );

	// Spin out of control.
	Vector forward;
	VPhysicsGetObject()->LocalToWorldVector( &forward, Vector( 1.0, 0.0, 0.0 ) );
	AngularImpulse torque = forward * m_flDiveBombRollForce;
	VPhysicsGetObject()->ApplyTorqueCenter( torque );

	// BUGBUG: why Y axis and not Z?
	Vector up;
	VPhysicsGetObject()->LocalToWorldVector( &up, Vector( 0.0, 1.0, 0.0 ) );
	VPhysicsGetObject()->ApplyForceCenter( up * 2000 );
}

void CNPC_FlyingPredator::MoveInDirection( float flInterval, const Vector &targetDir,
	float accelXY, float accelZ, float decay )
{
	decay = ExponentialDecay( decay, 1.0, flInterval );
	accelXY *= flInterval;
	accelZ  *= flInterval;

	Vector oldVelocity = GetAbsVelocity();
	Vector newVelocity;

	newVelocity.x = (decay * oldVelocity.x + accelXY * targetDir.x);
	newVelocity.y = (decay * oldVelocity.y + accelXY * targetDir.y);
	newVelocity.z = (decay * oldVelocity.z + accelZ  * targetDir.z);

	SetAbsVelocity( newVelocity );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_FlyingPredator::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	switch ( m_tFlyState )
	{
	case FlyState_Dive:
		SetFlyingState( CanLand() ? FlyState_Walking : FlyState_Falling );
		// If I fear or hate this entity, do damage!
		if ( IRelationType( pOther ) != D_LI )
		{
			CTakeDamageInfo info( this, this, sk_flyingpredator_dmg_dive.GetFloat(), DMG_CLUB | DMG_ALWAYSGIB );
			pOther->TakeDamage( info );
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that makes the crow fly away.
//-----------------------------------------------------------------------------
void CNPC_FlyingPredator::InputForceFlying( inputdata_t &inputdata )
{
	SetFlyingState( FlyState_Flying );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that makes the crow fly away.
//-----------------------------------------------------------------------------
void CNPC_FlyingPredator::InputFly( inputdata_t &inputdata )
{
	SetCondition( COND_FORCED_FLY );
}

//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( NPC_FlyingPredator, CNPC_FlyingPredator )

	DECLARE_ACTIVITY( ACT_FALL )
	DECLARE_ACTIVITY( ACT_IDLE_CEILING )
	DECLARE_ACTIVITY( ACT_LAND_CEILING )

	DECLARE_CONDITION( COND_FORCED_FLY );
	DECLARE_CONDITION( COND_CAN_LAND );

	DECLARE_TASK( TASK_TAKEOFF )
	DECLARE_TASK( TASK_FLY_DIVE_BOMB )
	DECLARE_TASK( TASK_LAND )
	DECLARE_TASK( TASK_START_FLYING )
	DECLARE_TASK( TASK_START_FALLING )

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_TAKEOFF,

		"	Tasks"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_LEAP"
		"		TASK_WAIT				0.5"
		"		TASK_TAKEOFF			0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_HOVER"
		"		TASK_WAIT				1"
		"		TASK_START_FLYING		0"
		"		"
		"	Interrupts"
		"		"
	)

	DEFINE_SCHEDULE
	(
		SCHED_TAKEOFF_CEILING,

		"	Tasks"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_FALL"
		"		TASK_START_FALLING		0"
		"		TASK_WAIT				0.1"
		"		TASK_START_FLYING		0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_HOVER"
		"		"
		"	Interrupts"
		"		"
	)

	DEFINE_SCHEDULE
	(
		SCHED_FLY_DIVE_BOMB,

		"	Tasks"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_RANGE_ATTACK2"		
		"		TASK_FACE_ENEMY			0"
		"		TASK_WAIT				0.2"
		"		TASK_FLY_DIVE_BOMB		0"
		"		"
		"	Interrupts"
		"		COND_HEAVY_DAMAGE"
		"		COND_CAN_LAND"
	)

	DEFINE_SCHEDULE
	(
		SCHED_FALL,

		"	Tasks"
		"		TASK_START_FALLING		0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_FALL"
		"		TASK_WAIT				5"
		"	Interrupts"
		"		COND_CAN_LAND"
	)

	DEFINE_SCHEDULE
	(
		SCHED_LAND,

		"	Tasks"
		"		TASK_LAND				0"
		"		"
		"	Interrupts"
		"		"
	)

AI_END_CUSTOM_NPC()
