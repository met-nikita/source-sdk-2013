//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef NPC_BULLSQUID_H
#define NPC_BULLSQUID_H

#include "ai_basenpc.h"
#include "npc_basepredator.h"

class CNPC_Bullsquid : public CNPC_BasePredator
{
	DECLARE_CLASS( CNPC_Bullsquid, CNPC_BasePredator );
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
	int MeleeAttack1Conditions( float flDot, float flDist );

	void HandleAnimEvent( animevent_t *pEvent );

	float GetMaxSpitWaitTime( void );
	float GetMinSpitWaitTime( void );

	float GetWhipDamage( void );
	float GetBiteDamage( void );

	int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );

	bool IsPrey( CBaseEntity* pTarget ) { return pTarget->Classify() == CLASS_HEADCRAB; }
	virtual bool ShouldInfight( CBaseEntity * pTarget ); // Could this target npc be a rival I need to kill?

	void RunAI ( void );

	void StartTask ( const Task_t *pTask );
	void RunTask( const Task_t * pTask );

	int				SelectSchedule( void );
	int 			TranslateSchedule( int scheduleType );

	bool		ShouldGib( const CTakeDamageInfo &info );
	bool		CorpseGib( const CTakeDamageInfo &info );
	void		ExplosionEffect( void );

	bool SpawnNPC( const Vector position );

	DEFINE_CUSTOM_AI;

private:	
	int   m_nSquidSpitSprite;

	float m_nextSquidSoundTime;
};
#endif // NPC_BULLSQUID_H