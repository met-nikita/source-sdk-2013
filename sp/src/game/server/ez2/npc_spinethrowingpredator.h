//=============================================================================//
//
// Purpose: Predator that fires spiky projectiles at enemies.
//			Could be used to replicate the Pit Drone from Opposing Force
// Author: 1upD
//
//=============================================================================//

#ifndef NPC_SPINETHROWINGPREDATOR_H
#define NPC_SPINETHROWINGPREDATOR_H

#include "ai_basenpc.h"
#include "npc_basepredator.h"

class CNPC_SpineThrowingPredator : public CNPC_BasePredator
{
	DECLARE_CLASS( CNPC_SpineThrowingPredator, CNPC_BasePredator );
	DECLARE_DATADESC();

public:
	void Spawn( void );
	void Precache( void );
	Class_T	Classify( void );
	
	void IdleSound( void );
	void PainSound( const CTakeDamageInfo &info );
	void AlertSound( void );
	void DeathSound( const CTakeDamageInfo &info );
	void FoundEnemySound( void );
	void AttackSound( void );
	void GrowlSound( void );
	void BiteSound( void );
	void EatSound( void );

	float MaxYawSpeed ( void );

	int RangeAttack1Conditions( float flDot, float flDist );

	void HandleAnimEvent( animevent_t *pEvent );

	// Projectile methods
	virtual float GetProjectileGravity();
	virtual float GetProjectileArc();
	virtual float GetProjectileDamge();
	virtual float GetBiteDamage( void );
	virtual float GetWhipDamage( void );

	// Pit drones in opposing force hated headcrabs - this was an oversight, but let's keep it
	bool IsPrey( CBaseEntity* pTarget ) { return pTarget->Classify() == CLASS_HEADCRAB; }

	DEFINE_CUSTOM_AI;

private:	
	float m_nextSoundTime;
};
#endif // NPC_SPINETHROWINGPREDATOR_H