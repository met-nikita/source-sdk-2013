//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Gonome. Big scary-ass headcrab zombie, based heavily on the Bullsquid.+
//				+++Repurposed into the Zombie Assassin
//				++Can now Jump and Climb like the fast zombie
//
// 			Originally, the Gonome / Zombie Assassin was created by Sergeant Stacker
// 			and provided to the EZ2 team. 
//			It has been heavily modified for its role as the boss of EZ2 chapter 3. 
//			It now inherits from a base class, CNPC_BasePredator			
// $NoKeywords: $
//=============================================================================//
#ifndef NPC_GONOME_H
#define NPC_GONOME_H

#include "ai_basenpc.h"
#include "npc_basepredator.h"
#include "npc_BaseZombie.h"

class CNPC_Gonome : public CNPC_BasePredator
{
	DECLARE_CLASS( CNPC_Gonome, CNPC_BasePredator );

public:
	void Spawn( void );
	void Precache( void );
	Class_T	Classify( void );
	
	void IdleSound( void );
	void PainSound( const CTakeDamageInfo &info );
	void AlertSound( void );
	void DeathSound( const CTakeDamageInfo &info );
	void AttackSound( void );
	void GrowlSound( void );
	void FoundEnemySound(void);
	void RetreatModeSound(void);
	void BerserkModeSound(void);

	void HandleAnimEvent( animevent_t *pEvent );

	float GetMaxSpitWaitTime( void ) { return 5.0f; };
	float GetMinSpitWaitTime( void ) { return 0.5f; };
	float GetWhipDamage( void );
	
	void Ignite( float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );

	bool IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;
	bool ShouldIgnite( const CTakeDamageInfo &info );

	mutable float	m_flJumpDist;

	void RemoveIgnoredConditions( void );
	int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	virtual void OnFed();

	bool IsPrey( CBaseEntity* pTarget ) { return pTarget->Classify() == CLASS_PLAYER_ALLY; }
	bool SpawnNPC( const Vector position );

	int TranslateSchedule( int scheduleType );

	void StartTask ( const Task_t *pTask );

	virtual bool CanFlinch(void) { return false; } // Gonomes cannot flinch. 

	void PrescheduleThink( void );

	float	m_flBurnDamage;				// Keeps track of how much burn damage we've incurred in the last few seconds.
	float	m_flBurnDamageResetTime;	// Time at which we reset the burn damage.

	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC()

// Boss stuff
public:
	virtual bool	BecomeRagdollOnClient(const Vector &force); // Need to override this to play sound when killed from Hammer - kind of a hackhack
};
#endif // NPC_GONOME_H