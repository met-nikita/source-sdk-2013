//=============================================================================//
//
// Purpose: Flying enemy with similar behaviors to the bullsquid.
//			Fires spiky projectiles at enemies.
// Author: 1upD
//
//=============================================================================//

#ifndef NPC_FLYINGPREDATOR_H
#define NPC_FLYINGPREDATOR_H

#include "ai_basenpc.h"
#include "npc_basepredator.h"

class CNPC_FlyingPredator : public CNPC_BasePredator
{
	DECLARE_CLASS( CNPC_FlyingPredator, CNPC_BasePredator );
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

	void HandleAnimEvent( animevent_t *pEvent );

	float GetMaxSpitWaitTime( void );
	float GetMinSpitWaitTime( void );

	bool IsPrey( CBaseEntity* pTarget ) { return pTarget->Classify() == CLASS_EARTH_FAUNA; }

	virtual Activity NPC_TranslateActivity( Activity eNewActivity );
	virtual int TranslateSchedule( int scheduleType );

	void StartTask ( const Task_t *pTask );
	void RunTask( const Task_t * pTask );

	bool		ShouldGib( const CTakeDamageInfo &info ) { return true; };
	bool		CorpseGib( const CTakeDamageInfo &info );
	void		ExplosionEffect( void );

	bool SpawnNPC( const Vector position );

	DEFINE_CUSTOM_AI;

private:	
	float m_nextSoundTime;
};
#endif // NPC_FLYINGPREDATOR_H