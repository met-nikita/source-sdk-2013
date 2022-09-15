//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basehlplayer.h"
#include "playerandobjectenumerator.h"
#include "engine/ivdebugoverlay.h"
#include "c_ai_basenpc.h"
#include "in_buttons.h"
#include "collisionutils.h"
#include "c_basehlcombatweapon.h"
#include "eventlist.h"
#include "gamestringpool.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// How fast to avoid collisions with center of other object, in units per second
#define AVOID_SPEED 2000.0f
extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;
extern ConVar cl_sidespeed;

extern ConVar zoom_sensitivity_ratio;
extern ConVar default_fov;
extern ConVar sensitivity;

ConVar cl_npc_speedmod_intime( "cl_npc_speedmod_intime", "0.25", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
ConVar cl_npc_speedmod_outtime( "cl_npc_speedmod_outtime", "1.5", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
static ConVar cl_playermodel("cl_playermodel", "models/bad_cop.mdl", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Default Player Model");

IMPLEMENT_CLIENTCLASS_DT(C_BaseHLPlayer, DT_HL2_Player, CHL2_Player)
	RecvPropDataTable( RECVINFO_DT(m_HL2Local),0, &REFERENCE_RECV_TABLE(DT_HL2Local) ),
	RecvPropBool( RECVINFO( m_fIsSprinting ) ),
	RecvPropEHandle(RECVINFO(m_hRagdoll)),
#ifdef SP_ANIM_STATE
	RecvPropFloat( RECVINFO( m_flAnimRenderYaw ) ),
#endif
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_BaseHLPlayer )
	DEFINE_PRED_TYPEDESCRIPTION( m_HL2Local, C_HL2PlayerLocalData ),
	DEFINE_PRED_FIELD( m_fIsSprinting, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

// link to the correct class.
#if !defined ( EZ2MP ) && !defined ( PORTAL )
LINK_ENTITY_TO_CLASS( player, C_BaseHLPlayer );
#endif

//-----------------------------------------------------------------------------
// Purpose: Drops player's primary weapon
//-----------------------------------------------------------------------------
void CC_DropPrimary( void )
{
	C_BasePlayer *pPlayer = (C_BasePlayer *) C_BasePlayer::GetLocalPlayer();
	
	if ( pPlayer == NULL )
		return;

	pPlayer->Weapon_DropPrimary();
}

static ConCommand dropprimary("dropprimary", CC_DropPrimary, "dropprimary: Drops the primary weapon of the player.");

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_BaseHLPlayer::C_BaseHLPlayer()
{
	AddVar( &m_Local.m_vecPunchAngle, &m_Local.m_iv_vecPunchAngle, LATCH_SIMULATION_VAR );
	AddVar( &m_Local.m_vecPunchAngleVel, &m_Local.m_iv_vecPunchAngleVel, LATCH_SIMULATION_VAR );

	m_flZoomStart		= 0.0f;
	m_flZoomEnd			= 0.0f;
	m_flZoomRate		= 0.0f;
	m_flZoomStartTime	= 0.0f;
	m_flSpeedMod		= cl_forwardspeed.GetFloat();
	m_iIDEntIndex = 0;

#ifdef MAPBASE
	ConVarRef scissor("r_flashlightscissor");
	scissor.SetValue("0");
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_BaseHLPlayer::OnDataChanged( DataUpdateType_t updateType )
{
	// Make sure we're thinking
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

#ifdef SP_ANIM_STATE
	if (m_flAnimRenderYaw != FLT_MAX)
	{
		m_angAnimRender = QAngle( 0, m_flAnimRenderYaw, 0 );
	}
#endif

	BaseClass::OnDataChanged( updateType );
}

void C_BaseHLPlayer::ClientThink()
{
	BaseClass::ClientThink();

	UpdateIDTarget();
}

int C_BaseHLPlayer::GetIDTarget() const
{
	return m_iIDEntIndex;
}

void C_BaseHLPlayer::UpdateIDTarget()
{
	if (!IsLocalPlayer())
		return;

	// Clear old target and find a new one
	m_iIDEntIndex = 0;

	// don't show IDs in chase spec mode
	if (GetObserverMode() == OBS_MODE_CHASE ||
		GetObserverMode() == OBS_MODE_DEATHCAM)
		return;

	trace_t tr;
	Vector vecStart, vecEnd;
	VectorMA(MainViewOrigin(), 1500, MainViewForward(), vecEnd);
	VectorMA(MainViewOrigin(), 10, MainViewForward(), vecStart);
	UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);

	if (!tr.startsolid && tr.DidHitNonWorldEntity())
	{
		C_BaseEntity *pEntity = tr.m_pEnt;

		if (pEntity && (pEntity != this))
		{
			m_iIDEntIndex = pEntity->entindex();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseHLPlayer::Weapon_DropPrimary( void )
{
	engine->ServerCmd( "DropPrimary" );
}

float C_BaseHLPlayer::GetFOV()
{
	//Find our FOV with offset zoom value
	float flFOVOffset = BaseClass::GetFOV() + GetZoom();

	// Clamp FOV in MP
	//don't
	int min_fov = 5;
	
	// Don't let it go too low
	flFOVOffset = MAX( min_fov, flFOVOffset );

	return flFOVOffset;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float C_BaseHLPlayer::GetZoom( void )
{
	float fFOV = m_flZoomEnd;

	//See if we need to lerp the values
	if ( ( m_flZoomStart != m_flZoomEnd ) && ( m_flZoomRate > 0.0f ) )
	{
		float deltaTime = (float)( gpGlobals->curtime - m_flZoomStartTime ) / m_flZoomRate;

		if ( deltaTime >= 1.0f )
		{
			//If we're past the zoom time, just take the new value and stop lerping
			fFOV = m_flZoomStart = m_flZoomEnd;
		}
		else
		{
			fFOV = SimpleSplineRemapVal( deltaTime, 0.0f, 1.0f, m_flZoomStart, m_flZoomEnd );
		}
	}

	return fFOV;
}


Vector C_BaseHLPlayer::GetAttackSpread(CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget)
{
	if (pWeapon)
		return pWeapon->GetBulletSpread(WEAPON_PROFICIENCY_PERFECT);

	return VECTOR_CONE_15DEGREES;
}
ConVar sv_player_kick_default_modelname("sv_player_kick_default_modelname", "models/weapons/ez2/v_kick.mdl", FCVAR_REPLICATED, "Default filename of model to use for kick animation - can be overridden in map");
void C_BaseHLPlayer::Precache(void)
{
	BaseClass::Precache();

	if (m_LegModelName == NULL_STRING)
	{
		m_LegModelName = AllocPooledString(sv_player_kick_default_modelname.GetString());
	}
}
ConVar sv_player_kick_attack_enabled("sv_player_kick_attack_enabled", "1", FCVAR_REPLICATED);
void C_BaseHLPlayer::HandleKickAttack()
{
	if (!sv_player_kick_attack_enabled.GetBool() && !IsInAVehicle())
		return;

	// Door kick!
	if (gpGlobals->curtime >= m_flNextAttack && gpGlobals->curtime >= m_flNextKickAttack && !IsInAVehicle() && m_nButtons & IN_ATTACK3)
	{
		// Viewpunch
		QAngle punchAng;
		punchAng.x = random->RandomFloat(2.0f, 3.0f);
		punchAng.y = random->RandomFloat(-3.0f, -2.0f);
		punchAng.z = 0.0f;
		ViewPunch(punchAng);

		// Prevent all attacks for 1 second
		m_flNextKickAttack = gpGlobals->curtime + 1.0f;
		CBaseCombatWeapon * pWeapon = GetActiveWeapon();
		if (pWeapon)
		{
			pWeapon->m_flNextPrimaryAttack = MAX(GetActiveWeapon()->m_flNextPrimaryAttack.Get(), m_flNextKickAttack);
			pWeapon->m_flNextSecondaryAttack = MAX(GetActiveWeapon()->m_flNextSecondaryAttack.Get(), m_flNextKickAttack);

			CBaseHLCombatWeapon * pHLWeapon = dynamic_cast<CBaseHLCombatWeapon *>(pWeapon);

			// Lower the weapon if possible
			if (pHLWeapon)
			{
				if (pHLWeapon->CanLower() && !pHLWeapon->WeaponShouldBeLowered())
				{
					pHLWeapon->Lower();
					m_bKickWeaponLowered = true;
				}
			}
		}

		EmitSound("EZ2Player.KickSwing");

		StartKickAnimation();
	}
	else if (m_bKickWeaponLowered && gpGlobals->curtime <= m_flNextKickAttack)
	{
		CBaseHLCombatWeapon * pHLWeapon = dynamic_cast<CBaseHLCombatWeapon *>(GetActiveWeapon());

		// Lower the weapon if possible
		if (pHLWeapon && pHLWeapon->m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			pHLWeapon->Ready();
			m_bKickWeaponLowered = false;
		}
	}
}

// Should these be separate from g_bludgeonMins and g_bludgeonMaxs in basebludgeonweapon?
#define KICK_HULL_DIM		16
static const Vector g_kickMins(-KICK_HULL_DIM, -KICK_HULL_DIM, -KICK_HULL_DIM);
static const Vector g_kickMaxs(KICK_HULL_DIM, KICK_HULL_DIM, KICK_HULL_DIM);

void C_BaseHLPlayer::TraceKickAttack(CBaseEntity* pKickedEntity)
{
	Vector vecSrc = GetFlags() & FL_DUCKING ? EyePosition() : EyePosition() - Vector(0, 0, 32);
	Vector vecAim = BaseClass::GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
	Vector vecEnd = EyePosition() + (vecAim * 80);

	trace_t tr;

	UTIL_TraceLine(vecSrc, vecEnd, MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr);

	// If the ray trace didn't hit anything, use a hull trace based on bludgeon weapons
	if (tr.fraction == 1.0)
	{
		float bludgeonHullRadius = 1.732f * KICK_HULL_DIM;  // hull is +/- 16, so use cuberoot of 2 to determine how big the hull is from center to the corner point
		// Back off by hull "radius"
		vecEnd -= vecAim * bludgeonHullRadius;

		UTIL_TraceHull(vecSrc, vecEnd, g_kickMins, g_kickMaxs, MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr);
		if (tr.fraction < 1.0 && tr.m_pEnt)
		{
			Vector vecToTarget = tr.m_pEnt->GetAbsOrigin() - vecSrc;
			VectorNormalize(vecToTarget);

			float dot = vecToTarget.Dot(vecAim);

			// YWB:  Make sure they are sort of facing the guy at least...
			if (dot < 0.70721f)
			{
				// Force amiss
				tr.fraction = 1.0f;
			}
		}
	}

	if (pKickedEntity == NULL)
	{
		pKickedEntity = tr.m_pEnt;
	}

	if (pKickedEntity != NULL)
	{
		EmitSound("EZ2Player.KickHit");
	}
	else
	{
		EmitSound("EZ2Player.KickMiss");
	}
}

void C_BaseHLPlayer::StartKickAnimation(void)
{
	/*
	MDLCACHE_CRITICAL_SECTION();
	CBaseViewModel *vm = GetViewModel(2);
	if (vm == NULL)
	{
		//CreateViewModel(2);
		vm = GetViewModel(2);
	}

	if (vm)
	{
		vm->SetWeaponModel(STRING(m_LegModelName), NULL);

		int	idealSequence = vm->SelectWeightedSequence(ACT_VM_PRIMARYATTACK);

		if (idealSequence >= 0)
		{
			vm->SetSequence(idealSequence);
			vm->SetPlaybackRate(1.0f);
		}
	}
	SetAnimation(PLAYER_KICK);
	*/
}

void C_BaseHLPlayer::HandleKickAnimation(void)
{
	/*
	CBaseViewModel *pVM = GetViewModel(2);

	if (pVM)
	{
		pVM->SetPlaybackRate(1.0f);
		pVM->StudioFrameAdvance();
		//pVM->DispatchAnimEvents(this);
		if (pVM->IsSequenceFinished())
		{
			pVM->SetPlaybackRate(0.0f);

			// Destroy the kick viewmodel. 
			// This should prevent the kick animation from firing off after level transitions.
			//pVM->SUB_Remove();
		}
	}
	*/
}

void C_BaseHLPlayer::ItemPostFrame()
{
	/*
#ifdef EZ2
	HandleKickAttack();
#endif
	*/

	BaseClass::ItemPostFrame();

}

void C_BaseHLPlayer::HandleAnimEvent(animevent_t *pEvent)
{
	/*
	if (pEvent->event == AE_KICKATTACK)
	{
		TraceKickAttack();
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : FOVOffset - 
//			time - 
//-----------------------------------------------------------------------------
void C_BaseHLPlayer::Zoom( float FOVOffset, float time )
{
	m_flZoomStart		= GetZoom();
	m_flZoomEnd			= FOVOffset;
	m_flZoomRate		= time;
	m_flZoomStartTime	= gpGlobals->curtime;
}


//-----------------------------------------------------------------------------
// Purpose: Hack to zero out player's pitch, use value from poseparameter instead
// Input  : flags - 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseHLPlayer::DrawModel( int flags )
{
	// Not pitch for player
	QAngle saveAngles = GetLocalAngles();

	QAngle useAngles = saveAngles;
	useAngles[ PITCH ] = 0.0f;

	SetLocalAngles( useAngles );

	int iret = BaseClass::DrawModel( flags );

	SetLocalAngles( saveAngles );

	return iret;
}

bool C_BaseHLPlayer::ShouldDraw(void)
{
	// If we're dead, our ragdoll will be drawn for us instead.
	if (!IsAlive())
		return false;

	//	if( GetTeamNumber() == TEAM_SPECTATOR )
	//		return false;

	if (IsLocalPlayer() && IsRagdoll())
		return true;

	if (IsRagdoll())
		return false;

	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: Helper to remove from ladder
//-----------------------------------------------------------------------------
void C_BaseHLPlayer::ExitLadder()
{
	if ( MOVETYPE_LADDER != GetMoveType() )
		return;
	
	SetMoveType( MOVETYPE_WALK );
	SetMoveCollide( MOVECOLLIDE_DEFAULT );
	// Remove from ladder
	m_HL2Local.m_hLadder = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Determines if a player can be safely moved towards a point
// Input:   pos - position to test move to, fVertDist - how far to trace downwards to see if the player would fall,
//			radius - how close the player can be to the object, objPos - position of the object to avoid,
//			objDir - direction the object is travelling
//-----------------------------------------------------------------------------
bool C_BaseHLPlayer::TestMove( const Vector &pos, float fVertDist, float radius, const Vector &objPos, const Vector &objDir )
{
	trace_t trUp;
	trace_t trOver;
	trace_t trDown;
	float flHit1, flHit2;
	
	UTIL_TraceHull( GetAbsOrigin(), pos, GetPlayerMins(), GetPlayerMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trOver );
	if ( trOver.fraction < 1.0f )
	{
		// check if the endpos intersects with the direction the object is travelling.  if it doesn't, this is a good direction to move.
		if ( objDir.IsZero() ||
			( IntersectInfiniteRayWithSphere( objPos, objDir, trOver.endpos, radius, &flHit1, &flHit2 ) && 
			( ( flHit1 >= 0.0f ) || ( flHit2 >= 0.0f ) ) )
			)
		{
			// our first trace failed, so see if we can go farther if we step up.

			// trace up to see if we have enough room.
			UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, m_Local.m_flStepSize ), 
				GetPlayerMins(), GetPlayerMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trUp );

			// do a trace from the stepped up height
			UTIL_TraceHull( trUp.endpos, pos + Vector( 0, 0, trUp.endpos.z - trUp.startpos.z ), 
				GetPlayerMins(), GetPlayerMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trOver );

			if ( trOver.fraction < 1.0f )
			{
				// check if the endpos intersects with the direction the object is travelling.  if it doesn't, this is a good direction to move.
				if ( objDir.IsZero() ||
					( IntersectInfiniteRayWithSphere( objPos, objDir, trOver.endpos, radius, &flHit1, &flHit2 ) && ( ( flHit1 >= 0.0f ) || ( flHit2 >= 0.0f ) ) ) )
				{
					return false;
				}
			}
		}
	}

	// trace down to see if this position is on the ground
	UTIL_TraceLine( trOver.endpos, trOver.endpos - Vector( 0, 0, fVertDist ), 
		MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &trDown );

	if ( trDown.fraction == 1.0f ) 
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Client-side obstacle avoidance
//-----------------------------------------------------------------------------
void C_BaseHLPlayer::PerformClientSideObstacleAvoidance( float flFrameTime, CUserCmd *pCmd )
{
	// Don't avoid if noclipping or in movetype none
	switch ( GetMoveType() )
	{
	case MOVETYPE_NOCLIP:
	case MOVETYPE_NONE:
	case MOVETYPE_OBSERVER:
		return;
	default:
		break;
	}

	// Try to steer away from any objects/players we might interpenetrate
	Vector size = WorldAlignSize();

	float radius = 0.7f * sqrt( size.x * size.x + size.y * size.y );
	float curspeed = GetLocalVelocity().Length2D();

	//int slot = 1;
	//engine->Con_NPrintf( slot++, "speed %f\n", curspeed );
	//engine->Con_NPrintf( slot++, "radius %f\n", radius );

	// If running, use a larger radius
	float factor = 1.0f;

	if ( curspeed > 150.0f )
	{
		curspeed = MIN( 2048.0f, curspeed );
		factor = ( 1.0f + ( curspeed - 150.0f ) / 150.0f );

		//engine->Con_NPrintf( slot++, "scaleup (%f) to radius %f\n", factor, radius * factor );

		radius = radius * factor;
	}

	Vector currentdir;
	Vector rightdir;

	QAngle vAngles = pCmd->viewangles;
	vAngles.x = 0;

	AngleVectors( vAngles, &currentdir, &rightdir, NULL );
		
	bool istryingtomove = false;
	bool ismovingforward = false;
	if ( fabs( pCmd->forwardmove ) > 0.0f || 
		fabs( pCmd->sidemove ) > 0.0f )
	{
		istryingtomove = true;
		if ( pCmd->forwardmove > 1.0f )
		{
			ismovingforward = true;
		}
	}

	if ( istryingtomove == true )
		 radius *= 1.3f;

	CPlayerAndObjectEnumerator avoid( radius );
	partition->EnumerateElementsInSphere( PARTITION_CLIENT_SOLID_EDICTS, GetAbsOrigin(), radius, false, &avoid );

	// Okay, decide how to avoid if there's anything close by
	int c = avoid.GetObjectCount();
	if ( c <= 0 )
		return;

	//engine->Con_NPrintf( slot++, "moving %s forward %s\n", istryingtomove ? "true" : "false", ismovingforward ? "true" : "false"  );

	float adjustforwardmove = 0.0f;
	float adjustsidemove	= 0.0f;

	for ( int i = 0; i < c; i++ )
	{
		C_AI_BaseNPC *obj = dynamic_cast< C_AI_BaseNPC *>(avoid.GetObject( i ));

		if( !obj )
			continue;

		Vector vecToObject = obj->GetAbsOrigin() - GetAbsOrigin();

		float flDist = vecToObject.Length2D();
		
		// Figure out a 2D radius for the object
		Vector vecWorldMins, vecWorldMaxs;
		obj->CollisionProp()->WorldSpaceAABB( &vecWorldMins, &vecWorldMaxs );
		Vector objSize = vecWorldMaxs - vecWorldMins;

		float objectradius = 0.5f * sqrt( objSize.x * objSize.x + objSize.y * objSize.y );

		//Don't run this code if the NPC is not moving UNLESS we are in stuck inside of them.
		if ( !obj->IsMoving() && flDist > objectradius )
			  continue;

		if ( flDist > objectradius && obj->IsEffectActive( EF_NODRAW ) )
		{
			obj->RemoveEffects( EF_NODRAW );
		}

		Vector vecNPCVelocity;
		obj->EstimateAbsVelocity( vecNPCVelocity );
		float flNPCSpeed = VectorNormalize( vecNPCVelocity );

		Vector vPlayerVel = GetAbsVelocity();
		VectorNormalize( vPlayerVel );

		float flHit1, flHit2;
		Vector vRayDir = vecToObject;
		VectorNormalize( vRayDir );

		float flVelProduct = DotProduct( vecNPCVelocity, vPlayerVel );
		float flDirProduct = DotProduct( vRayDir, vPlayerVel );

		if ( !IntersectInfiniteRayWithSphere(
				GetAbsOrigin(),
				vRayDir,
				obj->GetAbsOrigin(),
				radius,
				&flHit1,
				&flHit2 ) )
			continue;

        Vector dirToObject = -vecToObject;
		VectorNormalize( dirToObject );

		float fwd = 0;
		float rt = 0;

		float sidescale = 2.0f;
		float forwardscale = 1.0f;
		bool foundResult = false;

		Vector vMoveDir = vecNPCVelocity;
		if ( flNPCSpeed > 0.001f )
		{
			// This NPC is moving. First try deflecting the player left or right relative to the NPC's velocity.
			// Start with whatever side they're on relative to the NPC's velocity.
			Vector vecNPCTrajectoryRight = CrossProduct( vecNPCVelocity, Vector( 0, 0, 1) );
			int iDirection = ( vecNPCTrajectoryRight.Dot( dirToObject ) > 0 ) ? 1 : -1;
			for ( int nTries = 0; nTries < 2; nTries++ )
			{
				Vector vecTryMove = vecNPCTrajectoryRight * iDirection;
				VectorNormalize( vecTryMove );
				
				Vector vTestPosition = GetAbsOrigin() + vecTryMove * radius * 2;

				if ( TestMove( vTestPosition, size.z * 2, radius * 2, obj->GetAbsOrigin(), vMoveDir ) )
				{
					fwd = currentdir.Dot( vecTryMove );
					rt = rightdir.Dot( vecTryMove );
					
					//Msg( "PUSH DEFLECT fwd=%f, rt=%f\n", fwd, rt );
					
					foundResult = true;
					break;
				}
				else
				{
					// Try the other direction.
					iDirection *= -1;
				}
			}
		}
		else
		{
			// the object isn't moving, so try moving opposite the way it's facing
			Vector vecNPCForward;
			obj->GetVectors( &vecNPCForward, NULL, NULL );
			
			Vector vTestPosition = GetAbsOrigin() - vecNPCForward * radius * 2;
			if ( TestMove( vTestPosition, size.z * 2, radius * 2, obj->GetAbsOrigin(), vMoveDir ) )
			{
				fwd = currentdir.Dot( -vecNPCForward );
				rt = rightdir.Dot( -vecNPCForward );

				if ( flDist < objectradius )
				{
					obj->AddEffects( EF_NODRAW );
				}

				//Msg( "PUSH AWAY FACE fwd=%f, rt=%f\n", fwd, rt );

				foundResult = true;
			}
		}

		if ( !foundResult )
		{
			// test if we can move in the direction the object is moving
			Vector vTestPosition = GetAbsOrigin() + vMoveDir * radius * 2;
			if ( TestMove( vTestPosition, size.z * 2, radius * 2, obj->GetAbsOrigin(), vMoveDir ) )
			{
				fwd = currentdir.Dot( vMoveDir );
				rt = rightdir.Dot( vMoveDir );

				if ( flDist < objectradius )
				{
					obj->AddEffects( EF_NODRAW );
				}

				//Msg( "PUSH ALONG fwd=%f, rt=%f\n", fwd, rt );

				foundResult = true;
			}
			else
			{
				// try moving directly away from the object
				Vector vTestPosition = GetAbsOrigin() - dirToObject * radius * 2;
				if ( TestMove( vTestPosition, size.z * 2, radius * 2, obj->GetAbsOrigin(), vMoveDir ) )
				{
					fwd = currentdir.Dot( -dirToObject );
					rt = rightdir.Dot( -dirToObject );
					foundResult = true;

					//Msg( "PUSH AWAY fwd=%f, rt=%f\n", fwd, rt );
				}
			}
		}

		if ( !foundResult )
		{
			// test if we can move through the object
			Vector vTestPosition = GetAbsOrigin() - vMoveDir * radius * 2;
			fwd = currentdir.Dot( -vMoveDir );
			rt = rightdir.Dot( -vMoveDir );

			if ( flDist < objectradius )
			{
				obj->AddEffects( EF_NODRAW );
			}

			//Msg( "PUSH THROUGH fwd=%f, rt=%f\n", fwd, rt );

			foundResult = true;
		}

		// If running, then do a lot more sideways veer since we're not going to do anything to
		//  forward velocity
		if ( istryingtomove )
		{
			sidescale = 6.0f;
		}

		if ( flVelProduct > 0.0f && flDirProduct > 0.0f )
		{
			sidescale = 0.1f;
		}

		float force = 1.0f;
		float forward = forwardscale * fwd * force * AVOID_SPEED;
		float side = sidescale * rt * force * AVOID_SPEED;

		adjustforwardmove	+= forward;
		adjustsidemove		+= side;
	}

	pCmd->forwardmove	+= adjustforwardmove;
	pCmd->sidemove		+= adjustsidemove;
	
	// Clamp the move to within legal limits, preserving direction. This is a little
	// complicated because we have different limits for forward, back, and side

	//Msg( "PRECLAMP: forwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );

	float flForwardScale = 1.0f;
	if ( pCmd->forwardmove > fabs( cl_forwardspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_forwardspeed.GetFloat() ) / pCmd->forwardmove;
	}
	else if ( pCmd->forwardmove < -fabs( cl_backspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_backspeed.GetFloat() ) / fabs( pCmd->forwardmove );
	}
	
	float flSideScale = 1.0f;
	if ( fabs( pCmd->sidemove ) > fabs( cl_sidespeed.GetFloat() ) )
	{
		flSideScale = fabs( cl_sidespeed.GetFloat() ) / fabs( pCmd->sidemove );
	}
	
	float flScale = MIN( flForwardScale, flSideScale );
	pCmd->forwardmove *= flScale;
	pCmd->sidemove *= flScale;

	//Msg( "POSTCLAMP: forwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );
}


void C_BaseHLPlayer::PerformClientSideNPCSpeedModifiers( float flFrameTime, CUserCmd *pCmd )
{
	if ( m_hClosestNPC == NULL )
	{
		if ( m_flSpeedMod != cl_forwardspeed.GetFloat() )
		{
			float flDeltaTime = (m_flSpeedModTime - gpGlobals->curtime);
			m_flSpeedMod = RemapValClamped( flDeltaTime, cl_npc_speedmod_outtime.GetFloat(), 0, m_flExitSpeedMod, cl_forwardspeed.GetFloat() );
		}
	}
	else
	{
		C_AI_BaseNPC *pNPC = dynamic_cast< C_AI_BaseNPC *>( m_hClosestNPC.Get() );

		if ( pNPC )
		{
			float flDist = (GetAbsOrigin() - pNPC->GetAbsOrigin()).LengthSqr();
			bool bShouldModSpeed = false; 

			// Within range?
			if ( flDist < pNPC->GetSpeedModifyRadius() )
			{
				// Now, only slowdown if we're facing & running parallel to the target's movement
				// Facing check first (in 2D)
				Vector vecTargetOrigin = pNPC->GetAbsOrigin();
				Vector los = ( vecTargetOrigin - EyePosition() );
				los.z = 0;
				VectorNormalize( los );
				Vector facingDir;
				AngleVectors( GetAbsAngles(), &facingDir );
				float flDot = DotProduct( los, facingDir );
				if ( flDot > 0.8 )
				{
					/*
					// Velocity check (abort if the target isn't moving)
					Vector vecTargetVelocity;
					pNPC->EstimateAbsVelocity( vecTargetVelocity );
					float flSpeed = VectorNormalize(vecTargetVelocity);
					Vector vecMyVelocity = GetAbsVelocity();
					VectorNormalize(vecMyVelocity);
					if ( flSpeed > 1.0 )
					{
						// Velocity roughly parallel?
						if ( DotProduct(vecTargetVelocity,vecMyVelocity) > 0.4  )
						{
							bShouldModSpeed = true;
						}
					} 
					else
					{
						// NPC's not moving, slow down if we're moving at him
						//Msg("Dot: %.2f\n", DotProduct( los, vecMyVelocity ) );
						if ( DotProduct( los, vecMyVelocity ) > 0.8 )
						{
							bShouldModSpeed = true;
						} 
					}
					*/

					bShouldModSpeed = true;
				}
			}

			if ( !bShouldModSpeed )
			{
				m_hClosestNPC = NULL;
				m_flSpeedModTime = gpGlobals->curtime + cl_npc_speedmod_outtime.GetFloat();
				m_flExitSpeedMod = m_flSpeedMod;
				return;
			}
			else 
			{
				if ( m_flSpeedMod != pNPC->GetSpeedModifySpeed() )
				{
					float flDeltaTime = (m_flSpeedModTime - gpGlobals->curtime);
					m_flSpeedMod = RemapValClamped( flDeltaTime, cl_npc_speedmod_intime.GetFloat(), 0, cl_forwardspeed.GetFloat(), pNPC->GetSpeedModifySpeed() );
				}
			}
		}
	}

	if ( pCmd->forwardmove > 0.0f )
	{
		pCmd->forwardmove = clamp( pCmd->forwardmove, -m_flSpeedMod, m_flSpeedMod );
	}
	else
	{
		pCmd->forwardmove = clamp( pCmd->forwardmove, -m_flSpeedMod, m_flSpeedMod );
	}
	pCmd->sidemove = clamp( pCmd->sidemove, -m_flSpeedMod, m_flSpeedMod );
   
	//Msg( "fwd %f right %f\n", pCmd->forwardmove, pCmd->sidemove );
}

//-----------------------------------------------------------------------------
// Purpose: Input handling
//-----------------------------------------------------------------------------
bool C_BaseHLPlayer::CreateMove( float flInputSampleTime, CUserCmd *pCmd )
{
	bool bResult = BaseClass::CreateMove( flInputSampleTime, pCmd );

	if ( !IsInAVehicle() )
	{
		PerformClientSideObstacleAvoidance( TICK_INTERVAL, pCmd );
		PerformClientSideNPCSpeedModifiers( TICK_INTERVAL, pCmd );
	}

	return bResult;
}


//-----------------------------------------------------------------------------
// Purpose: Input handling
//-----------------------------------------------------------------------------
void C_BaseHLPlayer::BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	BaseClass::BuildTransformations( hdr, pos, q, cameraTransform, boneMask, boneComputed );
	BuildFirstPersonMeathookTransformations( hdr, pos, q, cameraTransform, boneMask, boneComputed, "ValveBiped.Bip01_Head1" );
}


#ifdef SP_ANIM_STATE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const QAngle& C_BaseHLPlayer::GetRenderAngles( void )
{
	if ( m_flAnimRenderYaw != FLT_MAX )
	{
		return m_angAnimRender;
	}
	else
	{
		return BaseClass::GetRenderAngles();	
	}
}
#endif

C_BaseAnimating *C_BaseHLPlayer::BecomeRagdollOnClient()
{
	// Let the C_CSRagdoll entity do this.
	// m_builtRagdoll = true;
	return NULL;
}

IRagdoll* C_BaseHLPlayer::GetRepresentativeRagdoll() const
{
	if (m_hRagdoll.Get())
	{
		C_EZ2MPRagdoll *pRagdoll = (C_EZ2MPRagdoll*)m_hRagdoll.Get();

		return pRagdoll->GetIRagdoll();
	}
	else
	{
		return NULL;
	}
}

IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_EZ2MPRagdoll, DT_EZ2MPRagdoll, CEZ2MPRagdoll)
RecvPropVector(RECVINFO(m_vecRagdollOrigin)),
RecvPropEHandle(RECVINFO(m_hPlayer)),
RecvPropInt(RECVINFO(m_nModelIndex)),
RecvPropInt(RECVINFO(m_nForceBone)),
RecvPropVector(RECVINFO(m_vecForce)),
RecvPropVector(RECVINFO(m_vecRagdollVelocity))
END_RECV_TABLE()



C_EZ2MPRagdoll::C_EZ2MPRagdoll()
{

}

C_EZ2MPRagdoll::~C_EZ2MPRagdoll()
{
	PhysCleanupFrictionSounds(this);

	if (m_hPlayer)
	{
		m_hPlayer->CreateModelInstance();
	}
}

void C_EZ2MPRagdoll::Interp_Copy(C_BaseAnimatingOverlay *pSourceEntity)
{
	if (!pSourceEntity)
		return;

	VarMapping_t *pSrc = pSourceEntity->GetVarMapping();
	VarMapping_t *pDest = GetVarMapping();

	// Find all the VarMapEntry_t's that represent the same variable.
	for (int i = 0; i < pDest->m_Entries.Count(); i++)
	{
		VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
		const char *pszName = pDestEntry->watcher->GetDebugName();
		for (int j = 0; j < pSrc->m_Entries.Count(); j++)
		{
			VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
			if (!Q_strcmp(pSrcEntry->watcher->GetDebugName(), pszName))
			{
				pDestEntry->watcher->Copy(pSrcEntry->watcher);
				break;
			}
		}
	}
}

void C_EZ2MPRagdoll::ImpactTrace(trace_t *pTrace, int iDamageType, const char *pCustomImpactName)
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if (!pPhysicsObject)
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if (iDamageType == DMG_BLAST)
	{
		dir *= 4000;  // adjust impact strenght

		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter(dir);
	}
	else
	{
		Vector hitpos;

		VectorMA(pTrace->startpos, pTrace->fraction, dir, hitpos);
		VectorNormalize(dir);

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset(dir, hitpos);

		// Blood spray!
		//		FX_CS_BloodSpray( hitpos, dir, 10 );
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}

void C_BaseHLPlayer::CalcView(Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov)
{
	if (m_lifeState != LIFE_ALIVE && !IsObserver())
	{
		Vector origin = EyePosition();

		IRagdoll *pRagdoll = GetRepresentativeRagdoll();

		if (pRagdoll)
		{
			origin = pRagdoll->GetRagdollOrigin();
			origin.z += VEC_DEAD_VIEWHEIGHT_SCALED(this).z; // look over ragdoll, not through
		}

		BaseClass::CalcView(eyeOrigin, eyeAngles, zNear, zFar, fov);

		eyeOrigin = origin;

		Vector vForward;
		AngleVectors(eyeAngles, &vForward);

		VectorNormalize(vForward);
		VectorMA(origin, -CHASE_CAM_DISTANCE_MAX, vForward, eyeOrigin);

		Vector WALL_MIN(-WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET);
		Vector WALL_MAX(WALL_OFFSET, WALL_OFFSET, WALL_OFFSET);

		trace_t trace; // clip against world
		C_BaseEntity::PushEnableAbsRecomputations(false); // HACK don't recompute positions while doing RayTrace
		UTIL_TraceHull(origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace);
		C_BaseEntity::PopEnableAbsRecomputations();

		if (trace.fraction < 1.0)
		{
			eyeOrigin = trace.endpos;
		}

		return;
	}

	BaseClass::CalcView(eyeOrigin, eyeAngles, zNear, zFar, fov);
}


void C_EZ2MPRagdoll::CreateEZ2MPRagdoll(void)
{
	// First, initialize all our data. If we have the player's entity on our client,
	// then we can make ourselves start out exactly where the player is.
	C_BaseHLPlayer *pPlayer = dynamic_cast< C_BaseHLPlayer* >(m_hPlayer.Get());

	if (pPlayer && !pPlayer->IsDormant())
	{
		// move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance(this);

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.
		bool bRemotePlayer = (pPlayer != C_BasePlayer::GetLocalPlayer());
		if (bRemotePlayer)
		{
			Interp_Copy(pPlayer);

			SetAbsAngles(pPlayer->GetRenderAngles());
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence(pPlayer->GetSequence());
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin(m_vecRagdollOrigin);

			SetAbsAngles(pPlayer->GetRenderAngles());

			SetAbsVelocity(m_vecRagdollVelocity);

			int iSeq = pPlayer->GetSequence();
			if (iSeq == -1)
			{
				Assert(false);	// missing walk_lower?
				iSeq = 0;
			}

			SetSequence(iSeq);	// walk_lower, basic pose
			SetCycle(0.0);

			Interp_Reset(varMap);
		}
	}
	else
	{
		// overwrite network origin so later interpolation will
		// use this position
		SetNetworkOrigin(m_vecRagdollOrigin);

		SetAbsOrigin(m_vecRagdollOrigin);
		SetAbsVelocity(m_vecRagdollVelocity);

		Interp_Reset(GetVarMapping());

	}

	SetModelIndex(m_nModelIndex);

	// Make us a ragdoll..
	m_nRenderFX = kRenderFxRagdoll;

	matrix3x4_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4_t currentBones[MAXSTUDIOBONES];
	const float boneDt = 0.05f;

	if (pPlayer && !pPlayer->IsDormant())
	{
		pPlayer->GetRagdollInitBoneArrays(boneDelta0, boneDelta1, currentBones, boneDt);
	}
	else
	{
		GetRagdollInitBoneArrays(boneDelta0, boneDelta1, currentBones, boneDt);
	}

	InitAsClientRagdoll(boneDelta0, boneDelta1, currentBones, boneDt);
}


void C_EZ2MPRagdoll::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	if (type == DATA_UPDATE_CREATED)
	{
		CreateEZ2MPRagdoll();
	}
}

IRagdoll* C_EZ2MPRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

void C_EZ2MPRagdoll::UpdateOnRemove(void)
{
	VPhysicsSetObject(NULL);

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
void C_EZ2MPRagdoll::SetupWeights(const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights)
{
	BaseClass::SetupWeights(pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights);

	static float destweight[128];
	static bool bIsInited = false;

	CStudioHdr *hdr = GetModelPtr();
	if (!hdr)
		return;

	int nFlexDescCount = hdr->numflexdesc();
	if (nFlexDescCount)
	{
		Assert(!pFlexDelayedWeights);
		memset(pFlexWeights, 0, nFlexWeightCount * sizeof(float));
	}

	if (m_iEyeAttachment > 0)
	{
		matrix3x4_t attToWorld;
		if (GetAttachment(m_iEyeAttachment, attToWorld))
		{
			Vector local, tmp;
			local.Init(1000.0f, 0.0f, 0.0f);
			VectorTransform(local, attToWorld, tmp);
			modelrender->SetViewTarget(GetModelPtr(), GetBody(), tmp);
		}
	}
}