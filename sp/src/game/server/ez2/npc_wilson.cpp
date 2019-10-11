//================= Copyright LOLOLOLOL, All rights reserved. ==================//
//
// Purpose: Bad Cop's friend, a pacifist turret who could open doors for some reason.
// 
// Author: Blixibon
//
//=============================================================================//

#include "cbase.h"

#include "npc_wilson.h"
#include "Sprite.h"
#include "ai_senses.h"
#include "sceneentity.h"
#include "props.h"
#include "particle_parse.h"
#include "eventqueue.h"
#include "ez2_player.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

extern int ACT_FLOOR_TURRET_OPEN;
extern int ACT_FLOOR_TURRET_CLOSE;
extern int ACT_FLOOR_TURRET_OPEN_IDLE;
extern int ACT_FLOOR_TURRET_CLOSED_IDLE;

extern ConVar	g_debug_turret;

// Interactions
int	g_interactionConsumedByXenGrenade	= 0;
int g_interactionArbeitScannerStart		= 0;
int g_interactionArbeitScannerEnd		= 0;

// Global pointer to Will-E for fast lookups
CEntityClassList<CNPC_Wilson> g_WillieList;
template <> CNPC_Wilson *CEntityClassList<CNPC_Wilson>::m_pClassList = NULL;
CNPC_Wilson *CNPC_Wilson::GetWilson( void )
{
	return g_WillieList.m_pClassList;
}

#define WILSON_MODEL "models/will_e.mdl"

#define FLOOR_TURRET_GLOW_SPRITE	"sprites/glow1.vmt"

#define MIN_IDLE_SPEECH 7.5f
#define MAX_IDLE_SPEECH 12.5f

#define WILSON_MAX_TIPPED_HEIGHT 2048

BEGIN_DATADESC(CNPC_Wilson)

	DEFINE_FIELD( m_bBlinkState,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bTipped,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bCarriedByPlayer, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bUseCarryAngles, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flPlayerDropTime, FIELD_TIME ),

	DEFINE_FIELD( m_iEyeAttachment,	FIELD_INTEGER ),
	//DEFINE_FIELD( m_iMuzzleAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_iEyeState,		FIELD_INTEGER ),
	DEFINE_FIELD( m_hEyeGlow,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLaser,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_pMotionController,FIELD_EHANDLE),

	DEFINE_FIELD( m_hPhysicsAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLastPhysicsInfluenceTime, FIELD_TIME ),

	DEFINE_FIELD( m_fNextFidgetSpeechTime, FIELD_TIME ),

	DEFINE_KEYFIELD( m_bStatic, FIELD_BOOLEAN, "static" ),

	DEFINE_INPUT( m_bOmniscient, FIELD_BOOLEAN, "SetOmniscient" ),

	DEFINE_USEFUNC( SimpleUse ),

	DEFINE_INPUTFUNC( FIELD_VOID, "SelfDestruct", InputSelfDestruct ),

	DEFINE_INPUTFUNC( FIELD_STRING, "AnswerConcept", InputAnswerConcept ),

	DEFINE_OUTPUT( m_OnTipped, "OnTipped" ),
	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),
	DEFINE_OUTPUT( m_OnPhysGunPickup, "OnPhysGunPickup" ),
	DEFINE_OUTPUT( m_OnPhysGunDrop, "OnPhysGunDrop" ),
	DEFINE_OUTPUT( m_OnDestroyed, "OnDestroyed" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_wilson, CNPC_Wilson );

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CNPC_Wilson::CNPC_Wilson( void ) : 
	m_hEyeGlow( NULL ),
	m_hLaser( NULL ),
	m_bCarriedByPlayer( false ),
	m_flPlayerDropTime( 0.0f ),
	m_bTipped( false ),
	m_pMotionController( NULL )
{
	g_WillieList.Insert(this);
}

CNPC_Wilson::~CNPC_Wilson( void )
{
	g_WillieList.Remove(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::Precache()
{
	if (GetModelName() == NULL_STRING)
		SetModelName(AllocPooledString(WILSON_MODEL));

	PrecacheModel(STRING(GetModelName()));
	PrecacheModel( FLOOR_TURRET_GLOW_SPRITE );

	// Hope that the original floor turret allows us to use them
	//ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_OPEN );
	//ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_CLOSE );
	//ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_CLOSED_IDLE );
	//ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_OPEN_IDLE );
	//ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_FIRE );

	//PrecacheScriptSound( "NPC_FloorTurret.Retire" );
	//PrecacheScriptSound( "NPC_FloorTurret.Deploy" );
	//PrecacheScriptSound( "NPC_FloorTurret.Move" );
	//PrecacheScriptSound( "NPC_FloorTurret.Activate" );
	//PrecacheScriptSound( "NPC_FloorTurret.Alert" );
	//PrecacheScriptSound( "NPC_FloorTurret.Die" );
	//PrecacheScriptSound( "NPC_FloorTurret.Retract");
	//PrecacheScriptSound( "NPC_FloorTurret.Alarm");
	//PrecacheScriptSound( "NPC_FloorTurret.Ping");
	//PrecacheScriptSound( "NPC_FloorTurret.DryFire");
	PrecacheScriptSound( "NPC_FloorTurret.Destruct" );

	if (!m_bStatic)
	{
		PrecacheParticleSystem( "explosion_turret_break" );
	}

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::Spawn()
{
	Precache();

	SetModel(STRING(GetModelName()));

	BaseClass::Spawn();

	// Wilson has to like Bad Cop for his AI to work.
	AddClassRelationship( CLASS_PLAYER, D_LI, 0 );

	AddEFlags( EFL_NO_DISSOLVE );

	// Will-E has no "head turning", but he does have gestures, which counts as an animated face.
	CapabilitiesAdd( /*bits_CAP_TURN_HEAD |*/ bits_CAP_ANIMATEDFACE );

	//NPCInit();

	SetBlocksLOS( false );

	m_HackedGunPos	= Vector( 0, 0, 12.75 );
	SetViewOffset( EyeOffset( ACT_IDLE ) );
	m_flFieldOfView	= 0.4f; // 60 degrees
	m_takedamage	= DAMAGE_EVENTS_ONLY;
	m_iHealth		= 100;
	m_iMaxHealth	= 100;

	SetPoseParameter( m_poseAim_Yaw, 0 );
	SetPoseParameter( m_poseAim_Pitch, 0 );

	//m_iMuzzleAttachment = LookupAttachment( "eyes" );
	m_iEyeAttachment = LookupAttachment( "light" );

	SetEyeState( TURRET_EYE_DORMANT );

	SetUse( &CNPC_Wilson::SimpleUse );

	// Don't allow us to skip animation setup because our attachments are critical to us!
	SetBoneCacheFlags( BCF_NO_ANIMATION_SKIP );

	//SetState(NPC_STATE_IDLE);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::Activate( void )
{
	BaseClass::Activate();

	// Force the eye state to the current state so that our glows are recreated after transitions
	SetEyeState( m_iEyeState );

	if ( !m_pMotionController && !m_bStatic )
	{
		// Create the motion controller
		m_pMotionController = CTurretTipController::CreateTipController( this );

		// Enable the controller
		if ( m_pMotionController != NULL )
		{
			m_pMotionController->Enable();
		}
	}
}

//-----------------------------------------------------------------------------

bool CNPC_Wilson::CreateVPhysics( void )
{
	//Spawn our physics hull
	IPhysicsObject *pPhys = VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
	if ( pPhys == NULL )
	{
		DevMsg( "%s unable to spawn physics object!\n", GetDebugName() );
		return false;
	}

	if ( m_bStatic )
	{
		pPhys->EnableMotion( false );
		PhysSetGameFlags(pPhys, FVPHYSICS_NO_PLAYER_PICKUP);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::UpdateOnRemove( void )
{
	if ( m_pMotionController != NULL )
	{
		UTIL_Remove( m_pMotionController );
		m_pMotionController = NULL;
	}

	if ( m_hEyeGlow != NULL )
	{
		UTIL_Remove( m_hEyeGlow );
		m_hEyeGlow = NULL;
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::Touch( CBaseEntity *pOther )
{
	// Do this dodgy trick to avoid TestPlayerPushing in CAI_PlayerAlly.
	CAI_BaseActor::Touch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::SimpleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_OnPlayerUse.FireOutput(pActivator, this);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
int CNPC_Wilson::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo	newInfo = info;

	if ( info.GetDamageType() & (DMG_SLASH|DMG_CLUB) )
	{
		// Take extra force from melee hits
		newInfo.ScaleDamageForce( 2.0f );
		
		// Disable our upright controller for some time
		if ( m_pMotionController != NULL )
		{
			m_pMotionController->Suspend( 2.0f );
		}
	}
	else if ( info.GetDamageType() & DMG_BLAST )
	{
		newInfo.ScaleDamageForce( 2.0f );
	}
	else if ( (info.GetDamageType() & DMG_BULLET) && !(info.GetDamageType() & DMG_BUCKSHOT) )
	{
		// Bullets, but not buckshot, do extra push
		newInfo.ScaleDamageForce( 2.5f );
	}

	// Manually apply vphysics because AI_BaseNPC takedamage doesn't call back to CBaseEntity OnTakeDamage
	VPhysicsTakeDamage( newInfo );

	// Respond to damage
	bool bTriggerHurt = info.GetInflictor() && info.GetInflictor()->ClassMatches("trigger_hurt");
	if ( !(info.GetDamageType() & DMG_RADIATION) || bTriggerHurt )
	{
		AI_CriteriaSet modifiers;
		ModifyOrAppendDamageCriteria(modifiers, info);
		SpeakIfAllowed( TLK_WOUND, modifiers );
	}

	if (bTriggerHurt || (m_hDamageFilter && static_cast<CBaseFilter*>(m_hDamageFilter.Get())->PassesDamageFilter(info)))
	{
		// Slowly take damage from trigger_hurts! (or anyone who passes our damage filter if we have one)
		m_iHealth -= info.GetDamage();

		// See this method's declaration comment for more info
		SetLastDamageTime(gpGlobals->curtime);

		if (m_iHealth <= 0)
		{
			Event_Killed( info );
			return 0;
		}
	}

	return BaseClass::OnTakeDamage(info);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
//-----------------------------------------------------------------------------
void CNPC_Wilson::Event_Killed( const CTakeDamageInfo &info )
{
	Vector vecUp;
	GetVectors( NULL, NULL, &vecUp );
	Vector vecOrigin = WorldSpaceCenter() + ( vecUp * 12.0f );

	// Our effect
	DispatchParticleEffect( "explosion_turret_break", vecOrigin, GetAbsAngles() );

	// Ka-boom!
	RadiusDamage( CTakeDamageInfo( this, info.GetAttacker(), 25.0f, DMG_BLAST ), vecOrigin, (10*12), CLASS_NONE, this );

	EmitSound( "NPC_Wilson.Destruct" );

	breakablepropparams_t params( GetAbsOrigin(), GetAbsAngles(), vec3_origin, RandomAngularImpulse( -800.0f, 800.0f ) );
	params.impactEnergyScale = 1.0f;
	params.defCollisionGroup = COLLISION_GROUP_INTERACTIVE;

	// no damage/damage force? set a burst of 100 for some movement
	params.defBurstScale = 100;
	PropBreakableCreateAll( GetModelIndex(), VPhysicsGetObject(), params, this, -1, true );

	// Throw out some small chunks too obscure the explosion even more
	CPVSFilter filter( vecOrigin );
	for ( int i = 0; i < 4; i++ )
	{
		Vector gibVelocity = RandomVector(-100,100);
		int iModelIndex = modelinfo->GetModelIndex( g_PropDataSystem.GetRandomChunkModel( "MetalChunks" ) );	
		te->BreakModel( filter, 0.0, vecOrigin, GetAbsAngles(), Vector(40,40,40), gibVelocity, iModelIndex, 150, 4, 2.5, BREAK_METAL );
	}

	// WILSOOOOON!!!
	m_OnDestroyed.FireOutput( info.GetAttacker(), this );

	if (info.GetAttacker())
	{
		info.GetAttacker()->Event_KilledOther(this, info);
	}

	if ( CBasePlayer *pPlayer = UTIL_GetLocalPlayer() )
	{
		pPlayer->Event_NPCKilled(this, info);
	}

	// We're done!
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Will-E got involved in a bloody accident!
//-----------------------------------------------------------------------------
void CNPC_Wilson::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	if (!IsAlive())
		return;

	AI_CriteriaSet modifiers;

	SetSpeechTarget( FindSpeechTarget( AIST_PLAYERS ) );

	ModifyOrAppendDamageCriteria(modifiers, info);
	ModifyOrAppendEnemyCriteria(modifiers, pVictim);

	SpeakIfAllowed(TLK_ENEMY_DEAD, modifiers);
}

//-----------------------------------------------------------------------------
// Purpose: Whether this should return carry angles
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Wilson::HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer )
{
	return m_bUseCarryAngles;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const QAngle
//-----------------------------------------------------------------------------
QAngle CNPC_Wilson::PreferredCarryAngles( void )
{
	// FIXME: Embed this into the class
	static QAngle g_prefAngles;

	Vector vecUserForward;
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	pPlayer->EyeVectors( &vecUserForward );

	g_prefAngles.Init();

	// Always look at the player as if you're saying "HELOOOOOO"
	g_prefAngles.y = 180.0f;
	
	return g_prefAngles;
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether the turret is upright enough to function
// Output : Returns true if the turret is tipped over
//-----------------------------------------------------------------------------
inline bool CNPC_Wilson::OnSide( void )
{
	Vector	up;
	GetVectors( NULL, NULL, &up );

	return ( DotProduct( up, Vector(0,0,1) ) < 0.40f );
}

//------------------------------------------------------------------------------
// Do we have a physics attacker?
//------------------------------------------------------------------------------
CBasePlayer *CNPC_Wilson::HasPhysicsAttacker( float dt )
{
	// If the player is holding me now, or I've been recently thrown
	// then return a pointer to that player
	if ( IsHeldByPhyscannon( ) || (gpGlobals->curtime - dt <= m_flLastPhysicsInfluenceTime) )
	{
		return m_hPhysicsAttacker;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::NPCThink()
{
	BaseClass::NPCThink();

	if (HasCondition(COND_IN_PVS))
	{
		// Needed for choreography reasons
		AimGun();
	}

	// See if we've tipped, but only do this if we're not being carried.
	// This is based on CNPC_FloorTurret::PreThink()'s tip detection functionality.
	if ( !IsBeingCarriedByPlayer() && !m_bStatic )
	{
		if ( OnSide() == false )
		{
			// TODO: NPC knock-over interaction handling?

			if (m_bTipped)
			{
				// Enable the tip controller
				m_pMotionController->Enable( true );

				m_bTipped = false;
			}
		}
		else
		{
			// Checking m_bTipped is how we figure out whether this is our first think since we've been tipped
			if (m_bTipped == false)
			{
				AI_CriteriaSet modifiers;

				// Measure our distance between ourselves and the ground.
				trace_t tr;
				AI_TraceLine(WorldSpaceCenter(), WorldSpaceCenter() - Vector(0, 0, WILSON_MAX_TIPPED_HEIGHT), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr);

				modifiers.AppendCriteria( "distancetoground", UTIL_VarArgs("%f", (tr.fraction * WILSON_MAX_TIPPED_HEIGHT)) );

				SpeakIfAllowed( TLK_TIPPED, modifiers );

				// Might want to get a real activator... (e.g. a last physics attacker)
				m_OnTipped.FireOutput( AI_GetSinglePlayer(), this );

				// Disable the tip controller
				m_pMotionController->Enable( false );

				m_bTipped = true;
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Wilson::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	// Simple health regeneration.
	// The one in CAI_PlayerAlly::PrescheduleThink() woudn't work because Wilson
	// doesn't take normal damage and therefore wouldn't be able to use TakeHealth().
	if( GetHealth() < GetMaxHealth() && gpGlobals->curtime - GetLastDamageTime() > 2.0f )
	{
		switch (g_pGameRules->GetSkillLevel())
		{
		case SKILL_EASY:		m_iHealth += 3; break;
		default:				m_iHealth += 2; break;
		case SKILL_HARD:		m_iHealth += 1; break;
		}

		if (m_iHealth > m_iMaxHealth)
			m_iHealth = m_iMaxHealth;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::GatherConditions( void )
{
	// Will-E doesn't use sensing code, but COND_SEE_PLAYER is important for CAI_PlayerAlly stuff.
	CBasePlayer *pLocalPlayer = AI_GetSinglePlayer();
	if (pLocalPlayer && FInViewCone(pLocalPlayer->EyePosition()) && FVisible(pLocalPlayer))
	{
		Assert( GetSenses()->HasSensingFlags(SENSING_FLAGS_DONT_LOOK) );

		SetCondition( COND_SEE_PLAYER );
	}

	BaseClass::GatherConditions();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	BaseClass::GatherEnemyConditions( pEnemy );

	if ( GetLastEnemyTime() == 0 || gpGlobals->curtime - GetLastEnemyTime() > 30 )
	{
		if ( HasCondition( COND_SEE_ENEMY ) && pEnemy->Classify() != CLASS_BULLSEYE )
		{
			SpeakIfAllowed(TLK_STARTCOMBAT);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Wilson::DoCustomSpeechAI( AISpeechSelection_t *pSelection, int iState )
{
	// From SelectAlertSpeech()
	CBasePlayer *pTarget = assert_cast<CBasePlayer *>(FindSpeechTarget( AIST_PLAYERS | AIST_FACING_TARGET ));
	if ( pTarget )
	{
		SetSpeechTarget(pTarget);

		// IsValidSpeechTarget() already verified the player is alive
		float flHealthPerc = ((float)pTarget->m_iHealth / (float)pTarget->m_iMaxHealth);
		if ( flHealthPerc < 1.0 )
		{
			if ( SelectSpeechResponse( TLK_PLHURT, NULL, pTarget, pSelection ) )
				return true;
		}

		// Citizens, etc. only told the player to reload when they themselves were reloading.
		// That can't apply to Will-E and we don't want him to say this in the middle of combat,
		// so we only have Will-E comment on the player's ammo when he's idle.
		if (iState == NPC_STATE_IDLE)
		{
			CBaseCombatWeapon *pWeapon = pTarget->GetActiveWeapon();
			if( pWeapon && pWeapon->UsesClipsForAmmo1() && 
				pWeapon->Clip1() < ( pWeapon->GetMaxClip1() * .5 ) &&
				pTarget->GetAmmoCount( pWeapon->GetPrimaryAmmoType() ) )
			{
				if ( SelectSpeechResponse( TLK_PLRELOAD, NULL, pTarget, pSelection ) )
					return true;
			}
		}
	}

	if ( HasCondition(COND_TALKER_PLAYER_DEAD) && HasCondition(COND_SEE_PLAYER) && !GetExpresser()->SpokeConcept(TLK_PLDEAD) )
	{
		if ( SelectSpeechResponse( TLK_PLDEAD, NULL, pTarget, pSelection ) )
			return true;
	}

	// Fidget handling
	if (m_fNextFidgetSpeechTime < gpGlobals->curtime)
	{
		// Try again later (set it before we return)
		m_fNextFidgetSpeechTime = gpGlobals->curtime + random->RandomFloat(MIN_IDLE_SPEECH, MAX_IDLE_SPEECH);

		//SpeakIfAllowed(TLK_WILLE_FIDGET);
		if ( SelectSpeechResponse( TLK_FIDGET, NULL, NULL, pSelection ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Make us explode
//-----------------------------------------------------------------------------
void CNPC_Wilson::InputSelfDestruct( inputdata_t &inputdata )
{
	const CTakeDamageInfo info(this, inputdata.pActivator, NULL, DMG_GENERIC);
	Event_Killed(info);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	if ( reason != PUNTED_BY_CANNON )
	{
		m_bCarriedByPlayer = true;
		m_OnPhysGunPickup.FireOutput( pPhysGunUser, this );

		// We want to use preferred carry angles if we're not nicely upright
		Vector vecToTurret = pPhysGunUser->GetAbsOrigin() - GetAbsOrigin();
		vecToTurret.z = 0;
		VectorNormalize( vecToTurret );

		// We want to use preferred carry angles if we're not nicely upright
		Vector	forward, up;
		GetVectors( &forward, NULL, &up );

		bool bUpright = DotProduct( up, Vector(0,0,1) ) > 0.9f;
		bool bInFront = DotProduct( vecToTurret, forward ) > 0.10f;

		// Correct our angles only if we're not upright or we're mostly in front of the turret
		m_bUseCarryAngles = ( bUpright == false || bInFront );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t reason )
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	m_bCarriedByPlayer = false;
	m_bUseCarryAngles = false;
	m_OnPhysGunDrop.FireOutput( pPhysGunUser, this );

	m_flPlayerDropTime = gpGlobals->curtime + 2.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Wilson::OnAttemptPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	return true;
}

extern int g_interactionCombineBash;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Wilson::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter *sourceEnt)
{
	// Change eye while being scanned
	if ( interactionType == g_interactionArbeitScannerStart )
	{
		SetEyeState(TURRET_EYE_SEEKING_TARGET);
		return true;
	}
	if ( interactionType == g_interactionArbeitScannerEnd )
	{
		SetEyeState(TURRET_EYE_DORMANT);
		return true;
	}


	// TODO: grenade_hopwire doesn't use this interaction yet, awaiting Xen Grenade being turned into a unique entity
	if ( interactionType == g_interactionConsumedByXenGrenade )
	{
		if (true)
		{
			// "Don't be sucked up" case, which is default

			// Return true to indicate we shouldn't be sucked up
			return true;
		}
		else
		{
			// "Let suck" case, which could be random or something

			// Maybe have the grenade-thrower as an activator, passed through *data
			// Also, WILSOOOOON!!!
			m_OnDestroyed.FireOutput(this, this);

			// Return false to indicate we should still be sucked up
			// (assuming the Xen Grenade has a thing where it doesn't consume if DispatchInteraction returns true)
			return false;
		}
	}

	if ( interactionType == g_interactionCombineBash )
	{
		// Get knocked away
		Vector forward, up;
		AngleVectors( sourceEnt->GetLocalAngles(), &forward, NULL, &up );
		ApplyAbsVelocityImpulse( forward * 100 + up * 50 );
		CTakeDamageInfo info( sourceEnt, sourceEnt, 30, DMG_CLUB );
		CalculateMeleeDamageForce( &info, forward, GetAbsOrigin() );
		TakeDamage( info );

		EmitSound( "NPC_Combine.WeaponBash" );
		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}

#define WILSON_EYE_GLOW_DEFAULT_BRIGHT 255

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::SetEyeState( eyeState_t state )
{
	// Must have a valid eye to affect
	if ( !m_hEyeGlow )
	{
		// Create our eye sprite
		m_hEyeGlow = CSprite::SpriteCreate( FLOOR_TURRET_GLOW_SPRITE, GetLocalOrigin(), false );
		if ( !m_hEyeGlow )
			return;

		m_hEyeGlow->SetTransparency( kRenderWorldGlow, 255, 0, 0, WILSON_EYE_GLOW_DEFAULT_BRIGHT, kRenderFxNoDissipation );
		m_hEyeGlow->SetAttachment( this, m_iEyeAttachment );
	}

	m_iEyeState = state;

	//Set the state
	switch( state )
	{
	default:
	case TURRET_EYE_DORMANT: // Default, based on the env_sprite in Wilson's temporary physbox prefab.
		m_hEyeGlow->SetColor( 255, 0, 0 );
		m_hEyeGlow->SetBrightness( WILSON_EYE_GLOW_DEFAULT_BRIGHT, 0.5f );
		m_hEyeGlow->SetScale( 0.3f, 0.5f );
		break;

	case TURRET_EYE_SEEKING_TARGET: // Now used for when an Arbeit scanner is scanning Wilson.
		m_hEyeGlow->SetColor( 255, 0, 0 );
		m_hEyeGlow->SetBrightness( 192, 0.25f );
		m_hEyeGlow->SetScale( 0.2f, 0.25f );

		// Blink state stuff was removed, see npc_turret_floor for original

		break;

	case TURRET_EYE_DEAD: //Fade out slowly
		m_hEyeGlow->SetColor( 255, 0, 0 );
		m_hEyeGlow->SetScale( 0.1f, 3.0f );
		m_hEyeGlow->SetBrightness( 0, 3.0f );
		break;

	case TURRET_EYE_DISABLED:
		m_hEyeGlow->SetColor( 0, 255, 0 );
		m_hEyeGlow->SetScale( 0.1f, 1.0f );
		m_hEyeGlow->SetBrightness( 0, 1.0f );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::ModifyOrAppendCriteria(AI_CriteriaSet& set)
{
    set.AppendCriteria("being_held", IsBeingCarriedByPlayer() ? "1" : "0");

	if (m_hPhysicsAttacker)
	{
		set.AppendCriteria("physics_attacker", m_hPhysicsAttacker->GetClassname());
		set.AppendCriteria("last_physics_attack", UTIL_VarArgs("%f", gpGlobals->curtime - m_flLastPhysicsInfluenceTime));
	}
	else
	{
		set.AppendCriteria("physics_attacker", "0");
	}

	// Assume that if we're not using carrying angles, Will-E is not looking at the player.
    set.AppendCriteria("facing_player", m_bUseCarryAngles ? "1" : "0");

	set.AppendCriteria("on_side", OnSide() ? "1" : "0");

    BaseClass::ModifyOrAppendCriteria(set);
}

//-----------------------------------------------------------------------------
// Purpose: Appends damage criteria
//-----------------------------------------------------------------------------
void CNPC_Wilson::ModifyOrAppendDamageCriteria(AI_CriteriaSet & set, const CTakeDamageInfo & info)
{
	set.AppendCriteria("damage", UTIL_VarArgs("%i", (int)info.GetDamage()));
	set.AppendCriteria("damage_type", UTIL_VarArgs("%i", info.GetDamageType()));
}

//-----------------------------------------------------------------------------
// Purpose: Check if the given concept can be spoken and then speak it
//-----------------------------------------------------------------------------
bool CNPC_Wilson::SpeakIfAllowed(AIConcept_t concept, AI_CriteriaSet& modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter, bool bCanInterrupt)
{
	if (!bCanInterrupt && IsRunningScriptedSceneAndNotPaused(this, false) /*&& !IsInInterruptableScenes(this)*/)
		return false;

    if (!IsAllowedToSpeak(concept, HasCondition(COND_SEE_PLAYER)))
        return false;

    return Speak(concept, modifiers, pszOutResponseChosen, bufsize, filter);
}

//-----------------------------------------------------------------------------
// Purpose: Alternate method signature for SpeakIfAllowed allowing no criteriaset parameter 
//-----------------------------------------------------------------------------
bool CNPC_Wilson::SpeakIfAllowed(AIConcept_t concept, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter, bool bCanInterrupt)
{
    AI_CriteriaSet set;
	return SpeakIfAllowed(concept, set, pszOutResponseChosen, bufsize, filter, bCanInterrupt);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::PostSpeakDispatchResponse( AIConcept_t concept, AI_Response *response )
{
	// Only respond to speech that targets the player
	if (GetSpeechTarget() && GetSpeechTarget()->IsPlayer() && GetSpeechTarget()->IsAlive()) // IsValidSpeechTarget(0, pPlayer)
	{
		// Notify the player so they could respond
		variant_t variant;
		variant.SetString(AllocPooledString(concept));
		g_EventQueue.AddEvent(GetSpeechTarget(), "AnswerConcept", variant, (GetTimeSpeechComplete() - gpGlobals->curtime) + RandomFloat(0.5f, 1.0f), this, this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::InputAnswerConcept( inputdata_t &inputdata )
{
	// Complex Q&A
	if (inputdata.pActivator)
	{
		AI_CriteriaSet modifiers;

		SetSpeechTarget(inputdata.pActivator);
		modifiers.AppendCriteria("speechtarget_concept", inputdata.value.String());

		// Tip: Speech target contexts are appended automatically. (try applyContext)

		SpeakIfAllowed(TLK_CONCEPT_ANSWER, modifiers);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_Expresser *CNPC_Wilson::CreateExpresser(void)
{
    m_pExpresser = new CAI_Expresser(this);
    if (!m_pExpresser)
        return NULL;

    m_pExpresser->Connect(this);
    return m_pExpresser;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Wilson::PostConstructor(const char *szClassname)
{
    BaseClass::PostConstructor(szClassname);
    CreateExpresser();
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_wilson, CNPC_Wilson )

	DECLARE_INTERACTION( g_interactionConsumedByXenGrenade );
	DECLARE_INTERACTION( g_interactionArbeitScannerStart );
	DECLARE_INTERACTION( g_interactionArbeitScannerEnd );

AI_END_CUSTOM_NPC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

BEGIN_DATADESC(CArbeitScanner)

	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_flCooldown,	FIELD_FLOAT, "Cooldown" ),

	DEFINE_KEYFIELD( m_flOuterRadius,	FIELD_FLOAT, "OuterRadius" ),
	DEFINE_KEYFIELD( m_flInnerRadius,	FIELD_FLOAT, "InnerRadius" ),

	DEFINE_KEYFIELD( m_iszScanSound, FIELD_SOUNDNAME, "ScanSound" ),
	DEFINE_KEYFIELD( m_iszScanDeploySound, FIELD_SOUNDNAME, "ScanDeploySound" ),
	DEFINE_KEYFIELD( m_iszScanDoneSound, FIELD_SOUNDNAME, "ScanDoneSound" ),
	DEFINE_KEYFIELD( m_iszScanRejectSound, FIELD_SOUNDNAME, "ScanRejectSound" ),

	DEFINE_KEYFIELD( m_flScanTime,	FIELD_FLOAT, "ScanTime" ),
	DEFINE_FIELD( m_flScanEndTime, FIELD_TIME ),

	DEFINE_FIELD( m_hScanning, FIELD_EHANDLE ),
	DEFINE_FIELD( m_pSprite, FIELD_CLASSPTR ),

	DEFINE_FIELD( m_iScanAttachment,	FIELD_INTEGER ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_THINKFUNC( IdleThink ),
	DEFINE_THINKFUNC( AwaitScanThink ),
	DEFINE_THINKFUNC( WaitForReturnThink ),
	DEFINE_THINKFUNC( ScanThink ),

	DEFINE_OUTPUT( m_OnScanDone, "OnScanDone" ),
	DEFINE_OUTPUT( m_OnScanReject, "OnScanReject" ),

	DEFINE_OUTPUT( m_OnScanStart, "OnScanStart" ),
	DEFINE_OUTPUT( m_OnScanInterrupt, "OnScanInterrupt" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( prop_wilson_scanner, CArbeitScanner );

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CArbeitScanner::CArbeitScanner( void )
{
	m_flCooldown = -1;

	m_flInnerRadius = 64.0f;
	m_flOuterRadius = 128.0f;

	m_flScanTime = 2.0f;

	m_hScanning = NULL;
	m_pSprite = NULL;
	m_flScanEndTime = 0.0f;
	m_iScanAttachment = 0;
}

CArbeitScanner::~CArbeitScanner( void )
{
	CleanupScan();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArbeitScanner::Precache( void )
{
	if (GetModelName() == NULL_STRING)
		SetModelName( AllocPooledString("models/props_lab/monitor02.mdl") );

	PrecacheModel( STRING(GetModelName()) );

	PrecacheScriptSound( STRING(m_iszScanSound) );
	PrecacheScriptSound( STRING(m_iszScanDeploySound) );
	PrecacheScriptSound( STRING(m_iszScanDoneSound) );
	PrecacheScriptSound( STRING(m_iszScanRejectSound) );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArbeitScanner::Spawn( void )
{
	Precache();

	SetModel( STRING(GetModelName()) );

	BaseClass::Spawn();

	// Our view offset should correspond to our scan attachment
	m_iScanAttachment = LookupAttachment( "eyes" );
	Vector vecOrigin;
	GetAttachment( m_iScanAttachment, vecOrigin );
	SetViewOffset( vecOrigin - GetAbsOrigin() );

	if (!m_bDisabled)
	{
		SetThink( &CArbeitScanner::IdleThink );
		SetNextThink( gpGlobals->curtime );
	}
	else
	{
		SetThink( NULL );
		SetNextThink( TICK_NEVER_THINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArbeitScanner::InputEnable( inputdata_t &inputdata )
{
	// Only reset to IdleThink if we're disabled or "done"
	if (GetNextThink() == TICK_NEVER_THINK || GetScanState() == SCAN_DONE || GetScanState() == SCAN_REJECT)
	{
		SetThink( &CArbeitScanner::IdleThink );
		SetNextThink( gpGlobals->curtime );
	}
}

void CArbeitScanner::InputDisable( inputdata_t &inputdata )
{
	// Clean up any scans
	CleanupScan();
	SetScanState( SCAN_IDLE );

	SetThink( NULL );
	SetNextThink( TICK_NEVER_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_BaseNPC *CArbeitScanner::FindMatchingNPC( float flRadiusSqr )
{
	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();

	Vector vecForward;
	Vector vecOrigin;
	GetAttachment( m_iScanAttachment, vecOrigin, &vecForward );

	for ( int i = 0; i < nAIs; i++ )
	{
		CAI_BaseNPC *pNPC = ppAIs[ i ];

		if( IsScannable(pNPC) && pNPC->IsAlive() )
		{
			Vector vecToNPC = (vecOrigin - pNPC->GetAbsOrigin());
			float flDist = vecToNPC.LengthSqr();

			if( flDist < flRadiusSqr )
			{
				// Now do a visibility test.
				if ( IsInScannablePosition(pNPC, vecToNPC, vecForward) )
				{
					// They passed!
					return pNPC;
				}
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArbeitScanner::IdleThink()
{
	if( !UTIL_FindClientInPVS(edict()) || (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI) )
	{
		// If we're not in the PVS or AI is disabled, sleep!
		SetNextThink( gpGlobals->curtime + 0.5 );
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.25 );
	StudioFrameAdvance();

	// First, look through the NPCs that match our criteria and find out if any of them are in our outer radius.
	CAI_BaseNPC *pNPC = FindMatchingNPC( m_flOuterRadius * m_flOuterRadius );
	if (pNPC)
	{
		// Got one! Wait for them to enter the inner radius.
		SetScanState( SCAN_WAITING );
		EmitSound( STRING(m_iszScanDeploySound) );
		SetThink( &CArbeitScanner::AwaitScanThink );
		SetNextThink( gpGlobals->curtime + 1.0f );
	}
	else if (GetScanState() != SCAN_IDLE)
	{
		SetScanState( SCAN_IDLE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArbeitScanner::AwaitScanThink()
{
	// I know it's kind of weird to look through the NPCs again instead of just storing them or even using FindMatchingNPC, 
	// but I prefer this over having to keep track of every matching entity that gets nearby or doing the same function again.
	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();

	Vector vecForward;
	Vector vecOrigin;
	GetAttachment( m_iScanAttachment, vecOrigin, &vecForward );

	// Makes sure there's still someone in the outer radius
	bool bNPCAvailable = false;

	for ( int i = 0; i < nAIs; i++ )
	{
		CAI_BaseNPC *pNPC = ppAIs[ i ];

		if( IsScannable(pNPC) && pNPC->IsAlive() )
		{
			Vector vecToNPC = (vecOrigin - pNPC->GetAbsOrigin());
			float flDist = vecToNPC.LengthSqr();

			// This time, make sure they're in the inner radius
			if( flDist < (m_flInnerRadius * m_flInnerRadius) )
			{
				// Now do a visibility test.
				if ( IsInScannablePosition(pNPC, vecToNPC, vecForward) )
				{
					// Begin scanning this NPC!
					m_hScanning = pNPC;
					SetThink( &CArbeitScanner::ScanThink );
					SetNextThink( gpGlobals->curtime );
					return;
				}
			}

			// Okay, nobody in the inner radius yet. Make sure there's someone in the outer radius though.
			else if ( flDist < (m_flOuterRadius * m_flOuterRadius) )
			{
				if ( IsInScannablePosition(pNPC, vecToNPC, vecForward) )
				{
					// We still have someone in the outer radius.
					bNPCAvailable = true;
				}
			}
		}
	}

	if (!bNPCAvailable)
	{
		// Nobody's within scanning distance anymore! Wait a few seconds before going back.
		SetThink( &CArbeitScanner::WaitForReturnThink );
		SetNextThink( gpGlobals->curtime + 1.5f );
	}

	SetNextThink( gpGlobals->curtime + 0.25f );
	StudioFrameAdvance();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArbeitScanner::WaitForReturnThink()
{
	// Is anyone there?
	CAI_BaseNPC *pNPC = FindMatchingNPC( m_flOuterRadius * m_flOuterRadius );
	if (pNPC)
	{
		// Ah, they've returned!
		SetScanState( SCAN_WAITING );
		SetThink( &CArbeitScanner::AwaitScanThink );
		SetNextThink( gpGlobals->curtime );
	}
	else
	{
		// Go back to idling I guess.
		SetScanState( SCAN_IDLE );
		SetThink( &CArbeitScanner::IdleThink );
		SetNextThink( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArbeitScanner::ScanThink()
{
	Vector vecOrigin;
	GetAttachment( m_iScanAttachment, vecOrigin );

	if (!m_hScanning || !m_hScanning->IsAlive() || ((vecOrigin - m_hScanning->GetAbsOrigin()).LengthSqr() > (m_flInnerRadius * m_flInnerRadius)))
	{
		// Must've been killed or moved too far away while we were scanning.
		// Use WaitForReturn to test whether anyone else is in our radius.
		m_OnScanInterrupt.FireOutput( m_hScanning, this );

		CleanupScan();
		SetThink( &CArbeitScanner::WaitForReturnThink );
		SetNextThink( gpGlobals->curtime );
		return;
	}

	// Scanning...
	if (!m_pSprite)
	{
		// Okay, start our scanning effect.
		SetScanState( SCAN_SCANNING );
		EmitSound( STRING(m_iszScanSound) );
		m_OnScanStart.FireOutput( m_hScanning, this );

		m_pSprite = CSprite::SpriteCreate("sprites/glow1.vmt", GetLocalOrigin(), false);
		if (m_pSprite)
		{
			m_pSprite->SetTransparency( kRenderWorldGlow, 200, 240, 255, 255, kRenderFxNoDissipation );
			m_pSprite->SetScale(0.2f, 1.5f);
			m_pSprite->SetAttachment( this, m_iScanAttachment );
		}

		m_hScanning->DispatchInteraction(g_interactionArbeitScannerStart, NULL, NULL);

		m_flScanEndTime = gpGlobals->curtime + m_flScanTime;
	}
	else
	{
		// Check progress...
		if (m_flScanEndTime <= gpGlobals->curtime)
		{
			// Scan is finished!
			if (CanPassScan(m_hScanning))
			{
				SetScanState( SCAN_DONE );
				EmitSound( STRING(m_iszScanDoneSound) );
				m_OnScanDone.FireOutput(m_hScanning, this);
			}
			else
			{
				SetScanState( SCAN_REJECT );
				EmitSound( STRING(m_iszScanRejectSound) );
				m_OnScanReject.FireOutput(m_hScanning, this);
			}

			CleanupScan();

			if (m_flCooldown != -1)
			{
				SetThink( &CArbeitScanner::IdleThink );
				SetNextThink( gpGlobals->curtime + m_flCooldown );
			}
			else
			{
				SetThink( NULL );
				SetNextThink( TICK_NEVER_THINK );
			}

			return;
		}
	}

	SetNextThink(gpGlobals->curtime + 0.1f);
	StudioFrameAdvance();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CArbeitScanner::CleanupScan()
{
	if (m_hScanning)
	{
		m_hScanning->DispatchInteraction(g_interactionArbeitScannerEnd, NULL, NULL);
		m_hScanning = NULL;
	}

	if (m_pSprite)
	{
		UTIL_Remove(m_pSprite);
		m_pSprite = NULL;
	}
}
