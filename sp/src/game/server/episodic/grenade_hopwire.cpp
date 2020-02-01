//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Gravity well device
//
//=====================================================================================//

#include "cbase.h"
#include "grenade_hopwire.h"
#include "rope.h"
#include "rope_shared.h"
#include "beam_shared.h"
#include "physics.h"
#include "physics_saverestore.h"
#include "explode.h"
#include "physics_prop_ragdoll.h"
#include "movevars_shared.h"
#ifdef EZ2
#include "ai_basenpc.h"
#include "npc_barnacle.h"
#include "ez2/npc_basepredator.h"
#include "hl2_player.h"
#include "particle_parse.h"
#include "npc_crow.h"

#include "ai_hint.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_link.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar hopwire_vortex( "hopwire_vortex", "1" );

ConVar hopwire_strider_kill_dist_h( "hopwire_strider_kill_dist_h", "300" );
ConVar hopwire_strider_kill_dist_v( "hopwire_strider_kill_dist_v", "256" );
ConVar hopwire_strider_hits( "hopwire_strider_hits", "1" );
ConVar hopwire_trap("hopwire_trap", "0");
ConVar hopwire_hopheight( "hopwire_hopheight", "400" );

ConVar hopwire_minheight("hopwire_minheight", "100");
ConVar hopwire_radius("hopwire_radius", "300");
#ifndef EZ2
ConVar hopwire_strength( "hopwire_strength", "150" );
#endif
ConVar hopwire_duration("hopwire_duration", "3.0");
ConVar hopwire_pull_player("hopwire_pull_player", "1");
#ifdef EZ2
ConVar hopwire_strength( "hopwire_strength", "256" );
ConVar hopwire_ragdoll_radius( "hopwire_ragdoll_radius", "96" );
ConVar hopwire_spawn_life("hopwire_spawn_life", "1");
ConVar hopwire_guard_mass("hopwire_guard_mass", "1500");
ConVar hopwire_zombine_mass( "hopwire_zombine_mass", "800" );
ConVar hopwire_zombigaunt_mass( "hopwire_zombigaunt_mass", "1000" );
ConVar hopwire_bullsquid_mass("hopwire_bullsquid_mass", "500");
ConVar hopwire_stukabat_mass( "hopwire_stukabat_mass", "425" );
ConVar hopwire_antlion_mass("hopwire_antlion_mass", "325");
ConVar hopwire_zombie_mass( "hopwire_zombie_mass", "250" ); // TODO Zombies should require a rebel as an ingredient
ConVar hopwire_babysquid_mass( "hopwire_babysquid_mass", "200" );
ConVar hopwire_headcrab_mass("hopwire_headcrab_mass", "100");
ConVar hopwire_boid_mass( "hopwire_boid_mass", "50" );
#endif

ConVar g_debug_hopwire( "g_debug_hopwire", "0" );

#define	DENSE_BALL_MODEL	"models/props_junk/metal_paintcan001b.mdl"

#define	MAX_HOP_HEIGHT		(hopwire_hopheight.GetFloat())		// Maximum amount the grenade will "hop" upwards when detonated
#define	MIN_HOP_HEIGHT		(hopwire_minheight.GetFloat())		// Minimum amount the grenade will "hop" upwards when detonated

#ifdef EZ2
// These are defined in npc_wilson.
// I don't know if there's a way to make it base-level since Xen grenades aren't NPCs.
extern int g_interactionXenGrenadeConsume;
extern int g_interactionXenGrenadeRelease;
#endif

//-----------------------------------------------------------------------------
// Purpose: Returns the amount of mass consumed by the vortex
//-----------------------------------------------------------------------------
float CGravityVortexController::GetConsumedMass( void ) const 
{
	return m_flMass;
}

//-----------------------------------------------------------------------------
// Purpose: Adds the entity's mass to the aggregate mass consumed
//-----------------------------------------------------------------------------
void CGravityVortexController::ConsumeEntity( CBaseEntity *pEnt )
{
	// Don't try to consume the player! What would even happen!?
	if (pEnt->IsPlayer())
		return;

#ifdef EZ
	// Don't consume objects that are protected
	if ( pEnt->IsDisplacementImpossible() )
		return;


	// Don't consume barnacle parts! (Weird edge case)
	CBarnacleTongueTip *pBarnacleTongueTip = dynamic_cast< CBarnacleTongueTip* >(pEnt);
	if (pBarnacleTongueTip != NULL)
	{
		return;
	}
#endif

	// Get our base physics object
	IPhysicsObject *pPhysObject = pEnt->VPhysicsGetObject();
	if ( pPhysObject == NULL )
		return;

	// Ragdolls need to report the sum of all their parts
	CRagdollProp *pRagdoll = dynamic_cast< CRagdollProp* >( pEnt );
	if ( pRagdoll != NULL )
	{		
		// Find the aggregate mass of the whole ragdoll
		ragdoll_t *pRagdollPhys = pRagdoll->GetRagdoll();
		for ( int j = 0; j < pRagdollPhys->listCount; ++j )
		{
			m_flMass += pRagdollPhys->list[j].pObject->GetMass();
		}
	}
	else
	{
#ifdef EZ2
		if (pEnt->IsCombatCharacter() && pEnt->MyCombatCharacterPointer()->DispatchInteraction( g_interactionXenGrenadeConsume, this, NULL ))
		{
			// Do not remove
			m_hReleaseEntity = pEnt->MyCombatCharacterPointer();
			return;
		}
		else
#endif
		// Otherwise we just take the normal mass
		m_flMass += pPhysObject->GetMass();
	}

	// Destroy the entity
	UTIL_Remove( pEnt );
}

//-----------------------------------------------------------------------------
// Purpose: Causes players within the radius to be sucked in
//-----------------------------------------------------------------------------
void CGravityVortexController::PullPlayersInRange( void )
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	
	Vector	vecForce = GetAbsOrigin() - pPlayer->WorldSpaceCenter();
	float	dist = VectorNormalize( vecForce );
	
	// FIXME: Need a more deterministic method here
	if ( dist < 128.0f )
	{
		// Kill the player (with falling death sound and effects)
#ifndef EZ2
		CTakeDamageInfo deathInfo( this, this, GetAbsOrigin(), GetAbsOrigin(), 200, DMG_FALL );
#else
		CTakeDamageInfo deathInfo( this, this, GetAbsOrigin(), GetAbsOrigin(), 5, DMG_FALL );
#endif
		pPlayer->TakeDamage( deathInfo );
		
		if ( pPlayer->IsAlive() == false )
		{
#ifdef EZ2
			color32 green = { 32, 252, 0, 255 };
			UTIL_ScreenFade( pPlayer, green, 0.1f, 0.0f, (FFADE_OUT|FFADE_STAYOUT) );
#else
			color32 black = { 0, 0, 0, 255 };
			UTIL_ScreenFade( pPlayer, black, 0.1f, 0.0f, (FFADE_OUT|FFADE_STAYOUT) );
#endif
			return;
		}
	}

	// Must be within the radius
	if ( dist > m_flRadius )
		return;

	float mass = pPlayer->VPhysicsGetObject()->GetMass();
	float playerForce = m_flStrength * 0.05f;

	// Find the pull force
	// NOTE: We might want to make this non-linear to give more of a "grace distance"
	vecForce *= ( 1.0f - ( dist / m_flRadius ) ) * playerForce * mass;
	vecForce[2] *= 0.025f;
	
	pPlayer->SetBaseVelocity( vecForce );
	pPlayer->AddFlag( FL_BASEVELOCITY );
	
	// Make sure the player moves
	if ( vecForce.z > 0 && ( pPlayer->GetFlags() & FL_ONGROUND) )
	{
		pPlayer->SetGroundEntity( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to kill an NPC if it's within range and other criteria
// Input  : *pVictim - NPC to assess
//			**pPhysObj - pointer to the ragdoll created if the NPC is killed
// Output :	bool - whether or not the NPC was killed and the returned pointer is valid
//-----------------------------------------------------------------------------
bool CGravityVortexController::KillNPCInRange( CBaseEntity *pVictim, IPhysicsObject **pPhysObj )
{
	CBaseCombatCharacter *pBCC = pVictim->MyCombatCharacterPointer();

	// See if we can ragdoll
	if ( pBCC != NULL && pBCC->CanBecomeRagdoll() )
	{
		// Don't bother with striders
		if ( FClassnameIs( pBCC, "npc_strider" ) )
			return false;

		// TODO: Make this an interaction between the NPC and the vortex

		// Become ragdoll
		CTakeDamageInfo info( this, this, 1.0f, DMG_GENERIC );
		CBaseEntity *pRagdoll = CreateServerRagdoll( pBCC, 0, info, COLLISION_GROUP_INTERACTIVE_DEBRIS, true );
		pRagdoll->SetCollisionBounds( pVictim->CollisionProp()->OBBMins(), pVictim->CollisionProp()->OBBMaxs() );

		// Necessary to cause it to do the appropriate death cleanup
		CTakeDamageInfo ragdollInfo( this, this, 10000.0, DMG_GENERIC | DMG_REMOVENORAGDOLL );
		pVictim->TakeDamage( ragdollInfo );

		// Return the pointer to the ragdoll
		*pPhysObj = pRagdoll->VPhysicsGetObject();
		return true;
	}

	// Wasn't able to ragdoll this target
	*pPhysObj = NULL;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Creates a dense ball with a mass equal to the aggregate mass consumed by the vortex
//-----------------------------------------------------------------------------
void CGravityVortexController::CreateDenseBall( void )
{
	CBaseEntity *pBall = CreateEntityByName( "prop_physics" );
	
	pBall->SetModel( DENSE_BALL_MODEL );
	pBall->SetAbsOrigin( GetAbsOrigin() );
	pBall->Spawn();

	IPhysicsObject *pObj = pBall->VPhysicsGetObject();
	if ( pObj != NULL )
	{
		pObj->SetMass( GetConsumedMass() );
	}
}
#ifdef EZ2
//-----------------------------------------------------------------------------
// Purpose: Creates a xen lifeform with a mass equal to the aggregate mass consumed by the vortex
//-----------------------------------------------------------------------------
void CGravityVortexController::CreateXenLife(void)
{
	if ( GetConsumedMass() >= hopwire_guard_mass.GetFloat() )
	{
		if ( TryCreateNPC( "npc_antlionguard" ) )
			return;
	}
	if (GetConsumedMass() >= hopwire_zombigaunt_mass.GetFloat())
	{
		if (TryCreateNPC( "npc_zombigaunt" ) )
			return;
	}
	if ( GetConsumedMass() >= hopwire_zombine_mass.GetFloat() )
	{
		if ( TryCreateNPC( "npc_zombine" ) )
			return;
	}
	if ( GetConsumedMass() >= hopwire_bullsquid_mass.GetFloat() )
	{
		if ( TryCreateNPC( "npc_bullsquid" ) )
			return;
	}
	if (GetConsumedMass() >= hopwire_stukabat_mass.GetFloat())
	{
		if (TryCreateNPC( "npc_stukabat" ))
			return;
	}
	if ( GetConsumedMass() >= hopwire_antlion_mass.GetFloat() )
	{
		if ( TryCreateNPC( "npc_antlion" ) )
			return;
	}
	if ( GetConsumedMass() >= hopwire_zombie_mass.GetFloat() )
	{
		if ( TryCreateNPC( "npc_zombie" ) )
			return;
	}
	if (GetConsumedMass() >= hopwire_babysquid_mass.GetFloat())
	{
		if ( TryCreateBaby( "npc_bullsquid" ) )
			return;
	}
	if ( GetConsumedMass() >= hopwire_headcrab_mass.GetFloat() )
	{
		if ( TryCreateNPC( "npc_headcrab" ) )
			return;
	}
	if ( GetConsumedMass() >= hopwire_boid_mass.GetFloat() )
	{
		if ( TryCreateBird( "npc_boid" ) )
			return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create an NPC if possible.
//	Returns true if the NPC was created, false if no NPC created
//-----------------------------------------------------------------------------
bool CGravityVortexController::TryCreateNPC( const char *className ) {
	return TryCreateComplexNPC( className, false, false );
}

bool CGravityVortexController::TryCreateBaby( const char *className ) {
	return TryCreateComplexNPC( className, true, false );
}

#define SF_CROW_FLYING		16
bool CGravityVortexController::TryCreateBird ( const char *className ) {

	// Look for fly nodes
	CHintCriteria hintCriteria;
	hintCriteria.SetHintType( HINT_CROW_FLYTO_POINT );
	hintCriteria.AddIncludePosition( GetAbsOrigin(), 4500 );
	CAI_Hint *pHint = CAI_HintManager::FindHint( GetAbsOrigin(), hintCriteria );
	// If the vortex controller can't find a bird node, don't spawn any birds!
	if ( pHint == NULL )
	{
		return false;
	}

	return TryCreateComplexNPC( className, false, true );
}

bool CGravityVortexController::TryCreateComplexNPC( const char *className, bool isBaby, bool isBird) {
	CBaseEntity * pEntity = CreateEntityByName( className );
	CAI_BaseNPC * baseNPC = pEntity->MyNPCPointer();
	if (pEntity == NULL || baseNPC == NULL) {
		return false;
	}

	baseNPC->SetAbsOrigin( GetAbsOrigin() );
	baseNPC->SetAbsAngles( GetAbsAngles() );
	if (isBird)
	{
		baseNPC->AddSpawnFlags( SF_CROW_FLYING );
	}
	else
	{
		baseNPC->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
	}
	baseNPC->m_tEzVariant = CAI_BaseNPC::EZ_VARIANT_XEN;
	// Set the squad name of a new Xen NPC to 'xenpc_headcrab', 'xenpc_bullsquid', etc
	char * squadname = UTIL_VarArgs( "xe%s", className );
	DevMsg( "Adding xenpc '%s' to squad '%s' \n", baseNPC->GetDebugName(), squadname );
	baseNPC->SetSquadName( AllocPooledString( squadname ) );

	// If the created NPC is a bullsquid or other predatory alien, make sure it has the appropriate fields set
	CNPC_BasePredator * pPredator = dynamic_cast< CNPC_BasePredator * >(baseNPC);
	if (pPredator != NULL)
	{
		pPredator->setIsBaby( isBaby );
		pPredator->InputSetWanderAlways( inputdata_t() );
		pPredator->InputEnableSpawning( inputdata_t() );
	}

	DispatchSpawn( baseNPC );

	Vector vUpBit = baseNPC->GetAbsOrigin();
	vUpBit.z += 1;
	trace_t tr;
	AI_TraceHull( baseNPC->GetAbsOrigin(), vUpBit, baseNPC->GetHullMins(), baseNPC->GetHullMaxs(),
		MASK_NPCSOLID, baseNPC, COLLISION_GROUP_NONE, &tr );
	if ( tr.startsolid || (tr.fraction < 1.0) )
	{
		DevMsg( "Xen grenade can't create %s.  Bad Position!\n", className );
		baseNPC->SUB_Remove();
		NDebugOverlay::Box( baseNPC->GetAbsOrigin(), baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 255, 0, 0, 0, 0 );
		return false;
	}

	// Now that the XenPC was created successfully, play a sound and display a particle effect
	EmitSound( "WeaponXenGrenade.SpawnXenPC" );
	DispatchParticleEffect( "xenpc_spawn", baseNPC->WorldSpaceCenter(), baseNPC->GetAbsAngles(), baseNPC );

	// XenPC - ACTIVATE! Especially important for antlion glows
	baseNPC->Activate();

	// Notify the NPC of the player's position
	// Sometimes Xen grenade spawns just kind of hang out in one spot. They should always have at least one potential enemy to aggro
	// XenPCs that are 'predators' don't need this because they already wander
	CHL2_Player *pPlayer = (CHL2_Player *)UTIL_GetLocalPlayer();
	if ( pPredator == NULL && pPlayer != NULL )
	{
		DevMsg( "Updating xenpc '%s' enemy memory \n", baseNPC->GetDebugName(), squadname );
		baseNPC->UpdateEnemyMemory( pPlayer, pPlayer->GetAbsOrigin(), this );
	}

	return true;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Pulls physical objects towards the vortex center, killing them if they come too near
//-----------------------------------------------------------------------------
void CGravityVortexController::PullThink( void )
{
	// Pull any players close enough to us
	if(hopwire_pull_player.GetBool())
		PullPlayersInRange();

	Vector mins, maxs;
	mins = GetAbsOrigin() - Vector( m_flRadius, m_flRadius, m_flRadius );
	maxs = GetAbsOrigin() + Vector( m_flRadius, m_flRadius, m_flRadius );

	// Draw debug information
	if ( g_debug_hopwire.GetBool() )
	{
		NDebugOverlay::Box( GetAbsOrigin(), mins - GetAbsOrigin(), maxs - GetAbsOrigin(), 0, 255, 0, 16, 4.0f );
	}

	CBaseEntity *pEnts[128];
	int numEnts = UTIL_EntitiesInBox( pEnts, 128, mins, maxs, 0 );

	for ( int i = 0; i < numEnts; i++ )
	{
		if (pEnts[i]->IsPlayer())
			continue;

		IPhysicsObject *pPhysObject = NULL;

#ifdef EZ2
		// Hackhack - don't suck voltigores
		// TODO Improve this
		if ( FClassnameIs( pEnts[i], "npc_voltigore_projectile" ) || FClassnameIs( pEnts[i], "npc_voltigore" ))
			continue;

		Vector	vecForce = GetAbsOrigin() - pEnts[i]->WorldSpaceCenter();
		Vector	vecForce2D = vecForce;
		vecForce2D[2] = 0.0f;
		float	dist2D = VectorNormalize( vecForce2D );
		float	dist = VectorNormalize( vecForce );
		
		// First, pull npcs outside of the ragdoll radius towards the vortex
		if ( abs( dist ) > hopwire_ragdoll_radius.GetFloat() ) 
		{
			CAI_BaseNPC * pNPC = pEnts[i]->MyNPCPointer();
			if (pEnts[i]->IsNPC() && pNPC != NULL && pNPC->CanBecomeRagdoll())
			{
				// Find the pull force
				vecForce *= (1.0f - (dist2D / m_flRadius)) * m_flStrength;
				pNPC->ApplyAbsVelocityImpulse( vecForce );
				// We already handled this NPC, move on
				continue;
			}
		}
		if ( KillNPCInRange( pEnts[i], &pPhysObject ) )
		{
			DevMsg( "Xen grenade turned NPC '%s' into a ragdoll! \n", pEnts[i]->GetDebugName() );
		}
		else
		{
			// If we didn't have a valid victim, see if we can just get the vphysics object
			pPhysObject = pEnts[i]->VPhysicsGetObject();
			if ( pPhysObject == NULL )
			{
				continue;
			}
		}

		float mass = 0.0f;

		CRagdollProp * pRagdoll = dynamic_cast< CRagdollProp* >( pEnts[i] );
		ragdoll_t * pRagdollPhys = NULL;
		if (pRagdoll != NULL)
		{
			pRagdollPhys = pRagdoll->GetRagdoll();
		}

		if( pRagdollPhys != NULL)
		{			
			// Find the aggregate mass of the whole ragdoll
			for ( int j = 0; j < pRagdollPhys->listCount; ++j )
			{
				mass += pRagdollPhys->list[j].pObject->GetMass();
			}
		}
		else if ( pPhysObject != NULL )
		{
			mass = pPhysObject->GetMass();
		}
#else
		// Attempt to kill and ragdoll any victims in range
		if ( KillNPCInRange( pEnts[i], &pPhysObject ) == false )
		{	
			// If we didn't have a valid victim, see if we can just get the vphysics object
			pPhysObject = pEnts[i]->VPhysicsGetObject();
			if ( pPhysObject == NULL )
				continue;
		}

		float mass;

		CRagdollProp *pRagdoll = dynamic_cast< CRagdollProp* >( pEnts[i] );
		if ( pRagdoll != NULL )
		{
			ragdoll_t *pRagdollPhys = pRagdoll->GetRagdoll();
			mass = 0.0f;
			
			// Find the aggregate mass of the whole ragdoll
			for ( int j = 0; j < pRagdollPhys->listCount; ++j )
			{
				mass += pRagdollPhys->list[j].pObject->GetMass();
			}
		}
		else
		{
			mass = pPhysObject->GetMass();
		}
		Vector	vecForce = GetAbsOrigin() - pEnts[i]->WorldSpaceCenter();
		Vector	vecForce2D = vecForce;
		vecForce2D[2] = 0.0f;
		float	dist2D = VectorNormalize( vecForce2D );
		float	dist = VectorNormalize( vecForce );
#endif
		// FIXME: Need a more deterministic method here
		if ( dist < 48.0f )
		{
			ConsumeEntity( pEnts[i] );
			continue;
		}

		// Must be within the radius
		if ( dist > m_flRadius )
			continue;

		// Find the pull force
		vecForce *= ( 1.0f - ( dist2D / m_flRadius ) ) * m_flStrength * mass;
		
		if ( pEnts[i]->VPhysicsGetObject() )
		{
			// Pull the object in
			pEnts[i]->VPhysicsTakeDamage( CTakeDamageInfo( this, this, vecForce, GetAbsOrigin(), m_flStrength, DMG_BLAST ) );
		}
	}

	// Keep going if need-be
	if ( m_flEndTime > gpGlobals->curtime )
	{
		SetThink( &CGravityVortexController::PullThink );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else
	{
#ifdef EZ2
		if ( m_hReleaseEntity && m_hReleaseEntity->DispatchInteraction( g_interactionXenGrenadeRelease, this, NULL ) )
		{
			// Act as if it's the Xen life we were supposed to spawn
			m_hReleaseEntity = NULL;
			return;
		}

		DevMsg( "Consumed %.2f kilograms\n", m_flMass );
		// Spawn Xen lifeform
		if ( hopwire_spawn_life.GetBool() )
			CreateXenLife();
#else
		//Msg( "Consumed %.2f kilograms\n", m_flMass );
		//CreateDenseBall();
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Starts the vortex working
//-----------------------------------------------------------------------------
void CGravityVortexController::StartPull( const Vector &origin, float radius, float strength, float duration )
{
	SetAbsOrigin( origin );
	m_flEndTime	= gpGlobals->curtime + duration;
	m_flRadius	= radius;
	m_flStrength= strength;

#ifdef EZ2
	// Play a danger sound throughout the duration of the vortex so that NPCs run away
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), radius, duration, this );
#endif

	SetThink( &CGravityVortexController::PullThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: Creation utility
//-----------------------------------------------------------------------------
CGravityVortexController *CGravityVortexController::Create( const Vector &origin, float radius, float strength, float duration )
{
	// Create an instance of the vortex
	CGravityVortexController *pVortex = (CGravityVortexController *) CreateEntityByName( "vortex_controller" );
	if ( pVortex == NULL )
		return NULL;

	// Start the vortex working
	pVortex->StartPull( origin, radius, strength, duration );

	return pVortex;
}

BEGIN_DATADESC( CGravityVortexController )
	DEFINE_FIELD( m_flMass, FIELD_FLOAT ),
	DEFINE_FIELD( m_flEndTime, FIELD_TIME ),
	DEFINE_FIELD( m_flRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_flStrength, FIELD_FLOAT ),

	DEFINE_THINKFUNC( PullThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( vortex_controller, CGravityVortexController );

#ifdef EZ2
#define GRENADE_MODEL_CLOSED	"models/weapons/w_XenGrenade.mdl" // was roller.mdl
#define GRENADE_MODEL_OPEN		"models/weapons/w_XenGrenade.mdl" // was roller_spikes.mdl
#else
#define GRENADE_MODEL_CLOSED	"models/roller.mdl"
#define GRENADE_MODEL_OPEN		"models/roller_spikes.mdl"
#endif

BEGIN_DATADESC( CGrenadeHopwire )
	DEFINE_FIELD( m_hVortexController, FIELD_EHANDLE ),

	DEFINE_THINKFUNC( EndThink ),
	DEFINE_THINKFUNC( CombatThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_grenade_hopwire, CGrenadeHopwire );

IMPLEMENT_SERVERCLASS_ST( CGrenadeHopwire, DT_GrenadeHopwire )
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeHopwire::Spawn( void )
{
	if (szWorldModelClosed == NULL || szWorldModelClosed[0] == '\0') {
		DevMsg("Warning: Missing primary hopwire model, using placeholder models \n");
		SetWorldModelClosed(GRENADE_MODEL_CLOSED);
		SetWorldModelOpen(GRENADE_MODEL_OPEN);
	} else if (szWorldModelOpen == NULL || szWorldModelOpen[0] == '\0') {
		DevMsg("Warning: Missing secondary hopwire model, using primary model as secondary \n");
		SetWorldModelOpen(szWorldModelClosed);
	}

	Precache();

	SetModel( szWorldModelClosed );
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	
	CreateVPhysics();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGrenadeHopwire::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, 0, false );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeHopwire::Precache( void )
{
	// FIXME: Replace
	PrecacheScriptSound("NPC_Strider.Charge");
	PrecacheScriptSound("NPC_Strider.Shoot");
	//PrecacheSound("d3_citadel.weapon_zapper_beam_loop2");

	PrecacheModel( szWorldModelOpen );
	PrecacheModel( szWorldModelClosed );

#ifndef EZ2	
	PrecacheModel( DENSE_BALL_MODEL );
#else
	PrecacheScriptSound( "WeaponXenGrenade.Explode" );
	PrecacheScriptSound( "WeaponXenGrenade.SpawnXenPC" );

	PrecacheParticleSystem( "xenpc_spawn" );

	UTIL_PrecacheXenVariant( "npc_headcrab" );
	UTIL_PrecacheXenVariant( "npc_zombie" );
	UTIL_PrecacheXenVariant( "npc_antlion" );
	UTIL_PrecacheXenVariant( "npc_bullsquid" );
	UTIL_PrecacheXenVariant( "npc_zombine" );
	UTIL_PrecacheXenVariant( "npc_antlionguard" );
#endif

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : timer - 
//-----------------------------------------------------------------------------
void CGrenadeHopwire::SetTimer( float timer )
{
	SetThink( &CBaseGrenade::PreDetonate );
	SetNextThink( gpGlobals->curtime + timer );
}

#define	MAX_STRIDER_KILL_DISTANCE_HORZ	(hopwire_strider_kill_dist_h.GetFloat())		// Distance a Strider will be killed if within
#define	MAX_STRIDER_KILL_DISTANCE_VERT	(hopwire_strider_kill_dist_v.GetFloat())		// Distance a Strider will be killed if within

#define MAX_STRIDER_STUN_DISTANCE_HORZ	(MAX_STRIDER_KILL_DISTANCE_HORZ*2)	// Distance a Strider will be stunned if within
#define MAX_STRIDER_STUN_DISTANCE_VERT	(MAX_STRIDER_KILL_DISTANCE_VERT*2)	// Distance a Strider will be stunned if within

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeHopwire::KillStriders( void )
{
	CBaseEntity *pEnts[128];
	Vector	mins, maxs;

	ClearBounds( mins, maxs );
	AddPointToBounds( -Vector( MAX_STRIDER_STUN_DISTANCE_HORZ, MAX_STRIDER_STUN_DISTANCE_HORZ, MAX_STRIDER_STUN_DISTANCE_HORZ ), mins, maxs );
	AddPointToBounds(  Vector( MAX_STRIDER_STUN_DISTANCE_HORZ, MAX_STRIDER_STUN_DISTANCE_HORZ, MAX_STRIDER_STUN_DISTANCE_HORZ ), mins, maxs );
	AddPointToBounds( -Vector( MAX_STRIDER_STUN_DISTANCE_VERT, MAX_STRIDER_STUN_DISTANCE_VERT, MAX_STRIDER_STUN_DISTANCE_VERT ), mins, maxs );
	AddPointToBounds(  Vector( MAX_STRIDER_STUN_DISTANCE_VERT, MAX_STRIDER_STUN_DISTANCE_VERT, MAX_STRIDER_STUN_DISTANCE_VERT ), mins, maxs );

	// FIXME: It's probably much faster to simply iterate over the striders in the map, rather than any entity in the radius - jdw

	// Find any striders in range of us
	int numTargets = UTIL_EntitiesInBox( pEnts, ARRAYSIZE( pEnts ), GetAbsOrigin()+mins, GetAbsOrigin()+maxs, FL_NPC );
	float targetDistHorz, targetDistVert;

	for ( int i = 0; i < numTargets; i++ )
	{
		// Only affect striders
		if ( FClassnameIs( pEnts[i], "npc_strider" ) == false )
			continue;

		// We categorize our spatial relation to the strider in horizontal and vertical terms, so that we can specify both parameters separately
		targetDistHorz = UTIL_DistApprox2D( pEnts[i]->GetAbsOrigin(), GetAbsOrigin() );
		targetDistVert = fabs( pEnts[i]->GetAbsOrigin()[2] - GetAbsOrigin()[2] );

		if ( targetDistHorz < MAX_STRIDER_KILL_DISTANCE_HORZ && targetDistHorz < MAX_STRIDER_KILL_DISTANCE_VERT )
		{
			// Kill the strider
			float fracDamage = ( pEnts[i]->GetMaxHealth() / hopwire_strider_hits.GetFloat() ) + 1.0f;
			CTakeDamageInfo killInfo( this, this, fracDamage, DMG_GENERIC );
			Vector	killDir = pEnts[i]->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize( killDir );

			killInfo.SetDamageForce( killDir * -1000.0f );
			killInfo.SetDamagePosition( GetAbsOrigin() );

			pEnts[i]->TakeDamage( killInfo );
		}
		else if ( targetDistHorz < MAX_STRIDER_STUN_DISTANCE_HORZ && targetDistHorz < MAX_STRIDER_STUN_DISTANCE_VERT )
		{
			// Stun the strider
			CTakeDamageInfo killInfo( this, this, 200.0f, DMG_GENERIC );
			pEnts[i]->TakeDamage( killInfo );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeHopwire::EndThink( void )
{
	if ( hopwire_vortex.GetBool() )
	{
		EntityMessageBegin( this, true );
			WRITE_BYTE( 1 );
		MessageEnd();
	}

	SetThink( &CBaseEntity::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 1.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeHopwire::CombatThink( void )
{
	// Stop the grenade from moving
	AddEFlags( EF_NODRAW );
	AddFlag( FSOLID_NOT_SOLID );
	VPhysicsDestroyObject();
	SetAbsVelocity( vec3_origin );
	SetMoveType( MOVETYPE_NONE );

#ifndef EZ2 // No special behavior for striders in EZ2 - wouldn't want to Xen Grenade our allies!
	// Do special behaviors if there are any striders in the area
	KillStriders();
#endif

	// FIXME: Replace
#ifndef EZ2
	EmitSound("NPC_Strider.Charge"); // Sound to emit during detonation
#endif
	//EmitSound("d3_citadel.weapon_zapper_beam_loop2");

	// Quick screen flash
	CBasePlayer *pPlayer = ToBasePlayer( GetThrower() );
#ifdef EZ2
	color32 green = { 32,252,0,100 }; // my bits Xen Green - was 255,255,255,255
	UTIL_ScreenFade( pPlayer, green, 0.2f, 0.0f, FFADE_IN );
#else
	color32 white = { 255,255,255,255 };
	UTIL_ScreenFade( pPlayer, white, 0.2f, 0.0f, FFADE_IN );
#endif

	// Create the vortex controller to pull entities towards us
	if ( hopwire_vortex.GetBool() )
	{
		m_hVortexController = CGravityVortexController::Create( GetAbsOrigin(), hopwire_radius.GetFloat(), hopwire_strength.GetFloat(), hopwire_duration.GetFloat() );

		// Start our client-side effect
		EntityMessageBegin( this, true );
			WRITE_BYTE( 0 );
		MessageEnd();
		
		// Begin to stop in two seconds
		SetThink( &CGrenadeHopwire::EndThink );
		SetNextThink( gpGlobals->curtime + 2.0f );
	}
	else
	{
		// Remove us immediately
		SetThink( &CBaseEntity::SUB_Remove );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeHopwire::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	
	if ( pPhysicsObject != NULL )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hop off the ground to start deployment
//-----------------------------------------------------------------------------
void CGrenadeHopwire::Detonate( void )
{
#ifdef EZ2
	EmitSound("WeaponXenGrenade.Explode");
	SetModel( szWorldModelOpen );

	//Find out how tall the ceiling is and always try to hop halfway
	trace_t	tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, MAX_HOP_HEIGHT*2 ), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	// Jump half the height to the found ceiling
	float hopHeight = MIN( MAX_HOP_HEIGHT, (MAX_HOP_HEIGHT*tr.fraction) );
	hopHeight =	MAX(hopHeight, MIN_HOP_HEIGHT);

	// Go through each node and find a link we should hop to.
	Vector vecHopTo = vec3_invalid;
	for ( int node = 0; node < g_pBigAINet->NumNodes(); node++)
	{
		CAI_Node *pNode = g_pBigAINet->GetNode(node);

		if ((GetAbsOrigin() - pNode->GetOrigin()).LengthSqr() > Square(192.0f))
			continue;

		// Only hop towards ground nodes
		if (pNode->GetType() != NODE_GROUND)
			continue;

		// Iterate through these hulls to find the largest possible space we can hop to.
		// Should be sorted largest to smallest.
		static Hull_t iHopHulls[] =
		{
			HULL_LARGE_CENTERED,
			HULL_MEDIUM,
			HULL_HUMAN,
		};

		// The "3" here should be kept up-to-date with iHopHulls
		for (int hull = 0; hull < 3; hull++)
		{
			Hull_t iHull = iHopHulls[hull];
			CAI_Link *pLink = NULL;
			float flNearestSqr = Square(192.0f);
			int iNearestLink = -1;
			for (int i = 0; i < pNode->NumLinks(); i++)
			{
				pLink = pNode->GetLinkByIndex( i );
				if (pLink)
				{
					if (pLink->m_iAcceptedMoveTypes[iHull] & bits_CAP_MOVE_GROUND)
					{
						int iOtherID = node == pLink->m_iSrcID ? pLink->m_iDestID : pLink->m_iSrcID;
						CAI_Node *pOtherNode = g_pBigAINet->GetNode(iOtherID);
						Vector vecBetween = (pNode->GetOrigin() - pOtherNode->GetOrigin()) * 0.5f + (pOtherNode->GetOrigin());
						float flDistSqr = (GetAbsOrigin() - vecBetween).LengthSqr();

						if (flDistSqr > flNearestSqr)
							continue;

						UTIL_TraceLine( GetAbsOrigin() + Vector( 0, 0, hopHeight ), vecBetween, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
						if (tr.fraction == 1.0f)
						{
							vecHopTo = vecBetween;
							flNearestSqr = (GetAbsOrigin() - vecBetween).LengthSqr();
							iNearestLink = i;

							// DEBUG
							//DebugDrawLine(GetAbsOrigin(), vecHopTo, 100 * hull, 0, 255, false, 2.0f);
						}
					}
				}
			}

			// We found a link
			if (iNearestLink != -1)
				break;
		}
	}

	AngularImpulse hopAngle = RandomAngularImpulse( -300, 300 );

	// Hop towards the point we found
	Vector hopVel( 0.0f, 0.0f, hopHeight );
	if (vecHopTo != vec3_invalid)
	{
		// DEBUG
		//DebugDrawLine(GetAbsOrigin(), vecHopTo, 0, 255, 0, false, 3.0f);

		Vector vecDelta = (vecHopTo - GetAbsOrigin());
		float flLength = vecDelta.Length();
		if (flLength != 0.0f)
			hopVel += (vecDelta * (64.0f / flLength));
	}

	SetVelocity( hopVel, hopAngle );

	// Get the time until the apex of the hop
	float apexTime = sqrt( hopHeight / GetCurrentGravity() );
#else
	EmitSound("NPC_Strider.Shoot"); // Sound to emit before detonating
	SetModel( szWorldModelOpen );

	AngularImpulse	hopAngle = RandomAngularImpulse( -300, 300 );

	//Find out how tall the ceiling is and always try to hop halfway
	trace_t	tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, MAX_HOP_HEIGHT*2 ), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	// Jump half the height to the found ceiling
	float hopHeight = MIN( MAX_HOP_HEIGHT, (MAX_HOP_HEIGHT*tr.fraction) );
	hopHeight =	MAX(hopHeight, MIN_HOP_HEIGHT);

	//Add upwards velocity for the "hop"
	Vector hopVel( 0.0f, 0.0f, hopHeight );
	SetVelocity( hopVel, hopAngle );

	// Get the time until the apex of the hop
	float apexTime = sqrt( hopHeight / GetCurrentGravity() );
#endif

	// Explode at the apex
	SetThink( &CGrenadeHopwire::CombatThink );
	SetNextThink( gpGlobals->curtime + apexTime);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseGrenade *HopWire_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, const char * modelClosed, const char * modelOpen)
{
	CGrenadeHopwire *pGrenade = (CGrenadeHopwire *) CBaseEntity::CreateNoSpawn( "npc_grenade_hopwire", position, angles, pOwner ); // Don't spawn the hopwire until models are set!
	pGrenade->SetWorldModelClosed(modelClosed);
	pGrenade->SetWorldModelOpen(modelOpen);

	DispatchSpawn(pGrenade);

	// Only set ourselves to detonate on a timer if we're not a trap hopwire
	if ( hopwire_trap.GetBool() == false )
	{
		pGrenade->SetTimer( timer );
	}

	pGrenade->SetVelocity( velocity, angVelocity );
	pGrenade->SetThrower( ToBaseCombatCharacter( pOwner ) );

	return pGrenade;
}
