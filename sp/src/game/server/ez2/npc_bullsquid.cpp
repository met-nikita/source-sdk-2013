//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the bullsquid
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_squad.h"
#include "npcevent.h"
#include "npc_bullsquid.h"
#include "grenade_spit.h"
#include "particle_parse.h"
#include "movevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_bullsquid_health( "sk_bullsquid_health", "100" );
ConVar sk_bullsquid_dmg_bite( "sk_bullsquid_dmg_bite", "15" );
ConVar sk_bullsquid_dmg_whip( "sk_bullsquid_dmg_whip", "25" );
ConVar sk_bullsquid_spit_arc_size( "sk_bullsquid_spit_arc_size", "3");
ConVar sk_bullsquid_spit_min_wait( "sk_bullsquid_spit_min_wait", "0.5");
ConVar sk_bullsquid_spit_max_wait( "sk_bullsquid_spit_max_wait", "5");
ConVar sk_bullsquid_gestation( "sk_bullsquid_gestation", "15.0" );
ConVar sk_bullsquid_spawn_time( "sk_bullsquid_spawn_time", "5.0" );
ConVar sk_bullsquid_monster_infighting( "sk_bullsquid_monster_infighting", "1" );
ConVar sk_max_squad_squids( "sk_max_squad_squids", "4" ); // How many squids in a pack before offspring start branching off into their own pack?

//=========================================================
// Interactions
//=========================================================
int	g_interactionBullsquidThrow		= 0;
int	g_interactionBullsquidMonch		= 0;

LINK_ENTITY_TO_CLASS( npc_bullsquid, CNPC_Bullsquid );

int ACT_SQUID_EXCITED;
int ACT_SQUID_EAT;
int ACT_SQUID_DETECT_SCENT;
int ACT_SQUID_INSPECT_FLOOR;

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_BULLSQUID_GROW = 	LAST_SHARED_SCHEDULE_PREDATOR+ 1,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_SQUID_GROW = LAST_SHARED_PREDATOR_TASK + 1,
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Bullsquid )

	DEFINE_FIELD( m_nextSquidSoundTime,	FIELD_TIME ),

END_DATADESC()

//=========================================================
// Spawn
//=========================================================
void CNPC_Bullsquid::Spawn()
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
	
	m_iMaxHealth		= sk_bullsquid_health.GetFloat();
	m_iHealth			= m_iMaxHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 | bits_CAP_SQUAD );

	if (IsBoss())
	{
		// The king bullsquid is huge
		SetModelScale( 2.5 );
	}
	else if (m_bIsBaby)
	{
		// Baby squid do do do-do do-do
		SetModelScale( 0.5 );

		// Baby squids have half health
		m_iMaxHealth *= 0.5;
		m_iHealth = m_iMaxHealth;

		// Baby squids can't spit yet!
		CapabilitiesRemove( bits_CAP_INNATE_RANGE_ATTACK1 );
	}

	m_fCanThreatDisplay	= TRUE;
	m_flNextSpitTime = gpGlobals->curtime;

	NPCInit();

	m_flDistTooFar		= 784;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Bullsquid::Precache()
{
#ifdef EZ
	if (gm_iszGooPuddle == NULL_STRING)
		gm_iszGooPuddle = AllocPooledString( "zombie_goo_puddle" );
#endif

	if ( GetModelName() == NULL_STRING )
	{
		switch (m_tEzVariant)
		{
			case EZ_VARIANT_XEN:
				SetModelName( AllocPooledString( "models/bullsquid_xen.mdl" ) );
				break;
			case EZ_VARIANT_RAD:
				SetModelName( AllocPooledString( "models/bullsquid_rad.mdl" ) );
				break;
			default:
				SetModelName( AllocPooledString( "models/bullsquid.mdl" ) );
				break;
		}
	}

	PrecacheModel( STRING( GetModelName() ) );

	m_nSquidSpitSprite = PrecacheModel("sprites/greenspit1.vmt");// client side spittle.

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

	PrecacheScriptSound( "NPC_Bullsquid.Idle" );
	PrecacheScriptSound( "NPC_Bullsquid.Pain" );
	PrecacheScriptSound( "NPC_Bullsquid.Alert" );
	PrecacheScriptSound( "NPC_Bullsquid.Death" );
	PrecacheScriptSound( "NPC_Bullsquid.Attack1" );
	PrecacheScriptSound( "NPC_Bullsquid.FoundEnemy" );
	PrecacheScriptSound( "NPC_Bullsquid.Growl" );
	PrecacheScriptSound( "NPC_Bullsquid.TailWhip");
	PrecacheScriptSound( "NPC_Bullsquid.Bite" );
	PrecacheScriptSound( "NPC_Bullsquid.Eat" );

	PrecacheScriptSound( "NPC_Antlion.PoisonShoot" );
	PrecacheScriptSound( "NPC_Antlion.PoisonBall" );
	PrecacheScriptSound( "NPC_Bullsquid.Explode" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this monster's place in the relationship table.
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Bullsquid::Classify( void )
{
	return CLASS_BULLSQUID; 
}

//=========================================================
// IdleSound 
//=========================================================
#define SQUID_ATTN_IDLE	(float)1.5
void CNPC_Bullsquid::IdleSound( void )
{
	EmitSound( "NPC_Bullsquid.Idle" );
}

//=========================================================
// PainSound 
//=========================================================
void CNPC_Bullsquid::PainSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_Bullsquid.Pain" );
}

//=========================================================
// AlertSound
//=========================================================
void CNPC_Bullsquid::AlertSound( void )
{
	EmitSound( "NPC_Bullsquid.Alert" );
}

//=========================================================
// DeathSound
//=========================================================
void CNPC_Bullsquid::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_Bullsquid.Death" );
}

//=========================================================
// AttackSound
//=========================================================
void CNPC_Bullsquid::AttackSound( void )
{
	EmitSound( "NPC_Bullsquid.Attack1" );
}

//=========================================================
// FoundEnemySound
//=========================================================
void CNPC_Bullsquid::FoundEnemySound( void )
{
	if (gpGlobals->curtime >= m_nextSquidSoundTime)
	{
		EmitSound( "NPC_Bullsquid.FoundEnemy" );
		m_nextSquidSoundTime	= gpGlobals->curtime + random->RandomInt( 1.5, 3.0 );
	}
}

//=========================================================
// GrowlSound
//=========================================================
void CNPC_Bullsquid::GrowlSound( void )
{
	if (gpGlobals->curtime >= m_nextSquidSoundTime)
	{
		EmitSound( "NPC_Bullsquid.Growl" );
		m_nextSquidSoundTime	= gpGlobals->curtime + random->RandomInt(1.5,3.0);
	}
}

//=========================================================
// BiteSound
//=========================================================
void CNPC_Bullsquid::BiteSound( void )
{
	EmitSound( "NPC_Bullsquid.Bite" );
}

//=========================================================
// EatSound
//=========================================================
void CNPC_Bullsquid::EatSound( void )
{
	EmitSound( "NPC_Bullsquid.Eat" );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_Bullsquid::MaxYawSpeed( void )
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
// RangeAttack1Conditions - don't spit at prey - monch it!
//=========================================================
int CNPC_Bullsquid::RangeAttack1Conditions( float flDot, float flDist )
{
	if (IsPrey( GetEnemy() ))
	{
		return (COND_NONE);
	}

	return(BaseClass::RangeAttack1Conditions( flDot, flDist ));
}

//=========================================================
// MeleeAttack2Conditions - don't tail whip prey - monch it!
//=========================================================
int CNPC_Bullsquid::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( IsPrey( GetEnemy() ) )
	{
		return ( COND_NONE );
	}

	return( BaseClass::MeleeAttack1Conditions( flDot, flDist ) );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_Bullsquid::HandleAnimEvent( animevent_t *pEvent )
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
				float flGravity  = SPIT_GRAVITY;
				ThrowLimit(vSpitPos, vTarget, flGravity, sk_bullsquid_spit_arc_size.GetFloat(), Vector(0,0,0), Vector(0,0,0), GetEnemy(), &vToss, &pBlocker);

				CGrenadeSpit *pGrenade = (CGrenadeSpit*)CreateNoSpawn( "grenade_spit", vSpitPos, vec3_angle, this );
				//pGrenade->KeyValue( "velocity", vToss );
				pGrenade->Spawn( );
				pGrenade->SetThrower( this );
				pGrenade->SetOwnerEntity( this );
				pGrenade->SetSpitSize( 2 );
				pGrenade->SetAbsVelocity( vToss );

				// Tumble through the air
				pGrenade->SetLocalAngularVelocity(
					QAngle(
						random->RandomFloat( -100, -500 ),
						random->RandomFloat( -100, -500 ),
						random->RandomFloat( -100, -500 )
					)
				);
						
				AttackSound();
			
				CPVSFilter filter( vSpitPos );

				// Don't show a sprite because grenade_spit now has the antlion worker model
				//te->SpriteSpray( filter, 0.0,
				//	&vSpitPos, &vToss, m_nSquidSpitSprite, 5, 10, 15 );
			}
		}
		break;

		case PREDATOR_AE_BITE:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, Vector(-16,-16,-16), Vector(16,16,16), GetBiteDamage(), DMG_SLASH );
			if ( pHurt )
			{
				BiteSound(); // Only play the bite sound if we have a target
				CBaseCombatCharacter *pVictim = ToBaseCombatCharacter( pHurt );
				// Try to eat the target
				if ( pVictim && pVictim->DispatchInteraction( g_interactionBullsquidMonch, NULL, this ) )
				{
					OnFed();
					m_flHungryTime = gpGlobals->curtime + 10.0f; // Headcrabs only satiate the squid for 10 seconds
				}
				// I don't want that!
				else
				{
					Vector forward, up;
					AngleVectors( GetAbsAngles(), &forward, NULL, &up );
					pHurt->ApplyAbsVelocityImpulse( 100 * (up-forward) * GetModelScale() );
					pHurt->SetGroundEntity( NULL );
				}				
			}
			// Reusing this activity / animation event for babysquid spawning
			else if ( m_bReadyToSpawn )
			{
				Vector forward, up, spawnPos;
				AngleVectors( GetAbsAngles(), &forward, NULL, &up );
				spawnPos = ( forward * 128 ) + GetAbsOrigin();
				if ( SpawnNPC( spawnPos ) )
				{
					ExplosionEffect();
				}
			}
		}
		break;

		case PREDATOR_AE_WHIP_SND:
		{
			EmitSound( "NPC_Bullsquid.TailWhip" );
			break;
		}

		case PREDATOR_AE_TAILWHIP:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, Vector(-16,-16,-16), Vector(16,16,16), GetWhipDamage(), DMG_SLASH | DMG_ALWAYSGIB );
			if ( pHurt ) 
			{
				Vector right, up;
				AngleVectors( GetAbsAngles(), NULL, &right, &up );

				if ( pHurt->GetFlags() & ( FL_NPC | FL_CLIENT ) )
					 pHurt->ViewPunch( QAngle( 20, 0, -20 ) );
			
				pHurt->ApplyAbsVelocityImpulse( 100 * (up+2*right) * GetModelScale() );
			}
		}
		break;

		case PREDATOR_AE_BLINK:
		{
			// Close eye. 
			// Even skins are eyes open, odd are closed
			m_nSkin = ( (m_nSkin / 2) * 2) + 1;
		}
		break;

		case PREDATOR_AE_HOP:
		{
			float flGravity = GetCurrentGravity();

			// throw the squid up into the air on this frame.
			if ( GetFlags() & FL_ONGROUND )
			{
				SetGroundEntity( NULL );
			}

			// jump 40 inches into the air
			Vector vecVel = GetAbsVelocity();
			vecVel.z += sqrt( flGravity * 2.0 * 40 );
			SetAbsVelocity( vecVel );
		}
		break;

		case PREDATOR_AE_THROW:
			{
				// squid throws its prey IF the prey is a client. 
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, Vector(-16,-16,-16), Vector(16,16,16), 0, 0 );


				if ( pHurt && !m_bIsBaby )
				{
					pHurt->ViewPunch( QAngle(20,0,-20) );
							
					// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
					UTIL_ScreenShake( pHurt->GetAbsOrigin(), 25.0, 1.5, 0.7, 2, SHAKE_START );

					// If the player, throw him around
					if ( pHurt->IsPlayer())
					{
						Vector forward, up;
						AngleVectors( GetLocalAngles(), &forward, NULL, &up );
						pHurt->ApplyAbsVelocityImpulse( forward * 300 + up * 300 );
					}
					// If not the player see if has bullsquid throw interatcion
					else
					{
						CBaseCombatCharacter *pVictim = ToBaseCombatCharacter( pHurt );
						if (pVictim)
						{
							if ( pVictim->DispatchInteraction( g_interactionBullsquidThrow, NULL, this ) )
							{
								Vector forward, up;
								AngleVectors( GetLocalAngles(), &forward, NULL, &up );
								pVictim->ApplyAbsVelocityImpulse( forward * 300 + up * 250 );
							}
						}
					}
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
float CNPC_Bullsquid::GetMaxSpitWaitTime( void )
{
	return sk_bullsquid_spit_max_wait.GetFloat();
}

float CNPC_Bullsquid::GetMinSpitWaitTime( void )
{
	return sk_bullsquid_spit_max_wait.GetFloat();
}

//=========================================================
// Damage for bullsquid whip attack
//=========================================================
float CNPC_Bullsquid::GetWhipDamage( void )
{
	// Multiply the damage value by the scale of the model so that baby squids do less damage
	return sk_bullsquid_dmg_whip.GetFloat() * GetModelScale();
}

//=========================================================
// Damage for bullsquid whip attack
//=========================================================
float CNPC_Bullsquid::GetBiteDamage( void )
{
	// Multiply the damage value by the scale of the model so that baby squids do less damage
	return sk_bullsquid_dmg_bite.GetFloat() * GetModelScale();
}


//=========================================================
// Bullsquids have a complicated relationship check
// in order to determine when to seek out a potential mate.
//=========================================================
bool CNPC_Bullsquid::ShouldInfight( CBaseEntity * pTarget )
{
	if ( IsSameSpecies( pTarget ) )
	{
		// Bullsquids that are ready to spawn don't attack other squids, neither do baby squids
		if ( m_bReadyToSpawn || m_bIsBaby )
		{
			return false;
		}

		CNPC_Bullsquid * pOther = static_cast<CNPC_Bullsquid*>(pTarget->MyNPCPointer());
		if ( pOther )
		{
			if ( pOther->m_bReadyToSpawn || pOther->m_bIsBaby )
			{
				// Don't baby squids or squids that are ready to spawn
				return false;
			}

			// Should I try to mate?
			if ( ShouldFindMate() )
			{
				if (CanMateWithTarget( pOther, false ) )
				{
					// This squid is now marked as my mate
					m_hMate = pOther;
					m_iszMate = NULL_STRING;

					// If this mate is my squadmate, break off into a new squad for now
					if ( this->GetSquad() != NULL && pOther->GetSquad() == this->GetSquad() )
					{
						g_AI_SquadManager.CreateSquad( NULL_STRING )->AddToSquad( this );
					}

					// Attack that squid
					return true;
				}
			}

			// Don't attack squadmates
			if ( ( this->m_pSquad != NULL && this->m_pSquad == pOther->GetSquad() ) )
			{
				PredMsg( "Bullsquid '%s' wants to infight but can't fight squadmates. \n" );
				return false;
			}

			// bullsquids should attack other bullsquids who are eating! (or bullsquids they are already targeting)
			if ( sk_bullsquid_monster_infighting.GetBool() && ( HasCondition( COND_PREDATOR_SMELL_FOOD ) || pOther == GetEnemy() ) )
			{
				// Don't aggro your mate over food
				if (pOther == m_hMate)
				{
					PredMsg( "Bullsquid '%s' wants to infight but can't fight mate. \n" );
					return false;
				}

				// if this bullsquid is not my squadmate and they smell the food, I hate them
				// if ( pOther->HasCondition( COND_PREDATOR_SMELL_FOOD ) )
				{
					PredMsg( "Bullsquid '%s' fighting a rival over food. \n" );
					return true;
				}
			}
		}
	}

	return false;
}

//=========================================================
// TakeDamage - overridden for bullsquid so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CNPC_Bullsquid::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
#if 0 //Fix later.

	float flDist;
	Vector vecApex, vOffset;

	// if the squid is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if (GetEnemy() != NULL && IsMoving() && pevAttacker == GetEnemy() && gpGlobals->curtime - m_flLastHurtTime > 3)
	{
		flDist = (GetAbsOrigin() - GetEnemy()->GetAbsOrigin()).Length2D();

		if (flDist > SQUID_SPRINT_DIST)
		{
			AI_Waypoint_t*	pRoute = GetNavigator()->GetPath()->Route();

			if (pRoute)
			{
				flDist = (GetAbsOrigin() - pRoute[pRoute->iNodeID].vecLocation).Length2D();// reusing flDist. 

				if (GetNavigator()->GetPath()->BuildTriangulationRoute( GetAbsOrigin(), pRoute[pRoute->iNodeID].vecLocation, flDist * 0.5, GetEnemy(), &vecApex, &vOffset, NAV_GROUND ))
				{
					GetNavigator()->PrependWaypoint( vecApex, bits_WP_TO_DETOUR | bits_WP_DONT_SIMPLIFY );
				}
			}
		}
	}
#endif

	if( IsSameSpecies( inputInfo.GetAttacker() ) )
	{
		// Infant squids shouldn't take damage from adults
		// There were too many accidents in testing
		if ( m_bIsBaby )
		{
			return 0;
		}

		// if attacked by another squid and capable of spawning, there is a chance to spawn offspring
		if ( m_bSpawningEnabled && ShouldFindMate() )
		{
			CNPC_Bullsquid * pMate = static_cast<CNPC_Bullsquid*>(inputInfo.GetAttacker());
			if ( CanMateWithTarget( pMate, true ) )
			{
				// Bullsquid can now spawn offspring
				m_bReadyToSpawn = true;

				// Set the next spawn time based on the gestation period of the squid
				m_flNextSpawnTime = gpGlobals->curtime + sk_bullsquid_gestation.GetFloat();

				// Set other bullsquid to be the mate
				m_hMate = pMate;
				m_iszMate = NULL_STRING;

				// The other bullsquid is now part of my squad
				if ( this->GetSquad() == NULL )
				{
					g_AI_SquadManager.CreateSquad( NULL_STRING )->AddToSquad( this );
				}
				this->GetSquad()->AddToSquad( pMate );

				// Don't take damage
				return 0;
			}
		}
	}

	return BaseClass::OnTakeDamage_Alive( inputInfo );
}

//========================================================
// RunAI - overridden for bullsquid because there are things
// that need to be checked every think.
//========================================================
void CNPC_Bullsquid::RunAI( void )
{
	// first, do base class stuff
	BaseClass::RunAI();

	if ( m_nSkin % 2 != 0 )
	{
		// Close eye if it was open.
		// Even skins are open eyes, odd are closed
		m_nSkin--; 
	}

	if ( random->RandomInt( 0,39 ) == 0 )
	{
		// Open eye
		// Even skins are open eyes, odd are closed
		m_nSkin++;
	}
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule. 
//
// Overridden for bullsquids to play specific activities
//=========================================================
void CNPC_Bullsquid::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_PREDATOR_PLAY_EXCITE_ACT:
	{
		SetIdealActivity( (Activity)ACT_SQUID_EXCITED );
		break;
	}
	case TASK_PREDATOR_PLAY_EAT_ACT:
		EatSound();
		SetIdealActivity( (Activity)ACT_SQUID_EAT );
		break;
	case TASK_PREDATOR_PLAY_SNIFF_ACT:
		SetIdealActivity( (Activity)ACT_SQUID_DETECT_SCENT );
		break;
	case TASK_PREDATOR_PLAY_INSPECT_ACT:
		SetIdealActivity( (Activity)ACT_SQUID_INSPECT_FLOOR );
		break;
	case TASK_SQUID_GROW:
		SetModelScale( 0.5 + ( m_iTimesFed * 0.25f ) );

		// TODO Remember to apply Xensquid health modifier!
		m_iMaxHealth = GetModelScale() * sk_bullsquid_health.GetFloat();
		m_iHealth = m_iMaxHealth;

		ExplosionEffect();

		if ( GetModelScale() >= 1.0f )
		{
			m_bIsBaby = false;
			CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 );
			m_iTimesFed = 0;

			// Split off into a new squad if there are already max squids
			if ( this->GetSquad() != NULL && this->GetSquad()->NumMembers() > sk_max_squad_squids.GetInt() )
			{
				g_AI_SquadManager.CreateSquad( NULL_STRING )->AddToSquad( this );
			}
		}
		// Task should end after scale is applied
		TaskComplete();
		break;
	case TASK_PREDATOR_SPAWN:
		m_flNextSpawnTime = gpGlobals->curtime + sk_bullsquid_spawn_time.GetFloat();
		SetIdealActivity( (Activity) ACT_MELEE_ATTACK2 );
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
void CNPC_Bullsquid::RunTask ( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_PREDATOR_SPAWN:
	case TASK_SQUID_GROW:
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

//=========================================================
// GetSchedule 
//=========================================================
int CNPC_Bullsquid::SelectSchedule( void )
{
	if ( m_bIsBaby )
	{
		if ( 0.5f + ( m_iTimesFed * 0.25f ) > GetModelScale() )
		{
			return SCHED_BULLSQUID_GROW;
		}
	}
	return BaseClass::SelectSchedule();
}

int CNPC_Bullsquid::TranslateSchedule( int scheduleType )
{
	// For some reason, bullsquids chasing one another get frozen.
	if ( scheduleType == SCHED_CHASE_ENEMY && IsSameSpecies( GetEnemy() ) )
	{
		return SCHED_ESTABLISH_LINE_OF_FIRE;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Bullsquid::ShouldGib( const CTakeDamageInfo &info )
{
	return m_bIsBaby || IsBoss() || BaseClass::ShouldGib( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Bullsquid::CorpseGib( const CTakeDamageInfo &info )
{
	ExplosionEffect();

	// TODO bullsquids might need unique gibs - especially for babies and the bullsquid king, since they are off scale
	return BaseClass::CorpseGib( info );
}

void CNPC_Bullsquid::ExplosionEffect( void )
{
	DispatchParticleEffect( "bullsquid_explode", WorldSpaceCenter(), GetAbsAngles() );
	EmitSound( "NPC_Bullsquid.Explode" );
}

//-----------------------------------------------------------------------------
// Purpose: Create a new baby bullsquid
// Output : True if the new bullsquid is created
//-----------------------------------------------------------------------------
bool CNPC_Bullsquid::SpawnNPC( const Vector position )
{
	// Try to create entity
	CNPC_Bullsquid *pChild = dynamic_cast< CNPC_Bullsquid * >(CreateEntityByName( "npc_bullsquid" ));
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
			DevMsg( "Can't create baby squid. Bad Position!\n" );
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

AI_BEGIN_CUSTOM_NPC( npc_bullsquid, CNPC_Bullsquid )

	DECLARE_ACTIVITY( ACT_SQUID_EXCITED )
	DECLARE_ACTIVITY( ACT_SQUID_EAT )
	DECLARE_ACTIVITY( ACT_SQUID_DETECT_SCENT )
	DECLARE_ACTIVITY( ACT_SQUID_INSPECT_FLOOR )

	DECLARE_TASK( TASK_SQUID_GROW )

	DECLARE_INTERACTION( g_interactionBullsquidThrow )
	DECLARE_INTERACTION( g_interactionBullsquidMonch )

	//=========================================================
	// > SCHED_SQUID_SNIFF_AND_EAT
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_BULLSQUID_GROW,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_PREDATOR_EAT					10"
		"		TASK_PREDATOR_PLAY_SNIFF_ACT		0"
		"		TASK_SQUID_GROW						0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							1"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
	)

AI_END_CUSTOM_NPC()
