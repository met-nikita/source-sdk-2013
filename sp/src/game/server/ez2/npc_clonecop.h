//=============================================================================//
//
// Purpose:		Clone Cop, a former man bent on killing anyone who stands in his way.
//				He was trapped under Arbeit 1 for a long time (from his perspective),
//				but now he's back and he's bigger, badder, and possibly even more deranged than ever before.
//				I mean, you could just see the brains of this guy.
//
// Author:		Blixibon
//
//=============================================================================//

#ifndef NPC_CLONECOP_H
#define NPC_CLONECOP_H
#ifdef _WIN32
#pragma once
#endif

#include "npc_combine.h"

class CNPC_CloneCop : public CNPC_Combine
{
	DECLARE_CLASS( CNPC_CloneCop, CNPC_Combine );
	DECLARE_DATADESC();

public:
	CNPC_CloneCop();

	void		Spawn( void );
	void		Precache( void );
	void		Activate( void );

	Class_T		Classify( void ) { return CLASS_COMBINE_NEMESIS; }

	void		DeathSound( const CTakeDamageInfo &info );

	void		ClearAttackConditions( void );
	void		PrescheduleThink();
	int			SelectSchedule( void );
	int			TranslateSchedule( int scheduleType );

	int			OnTakeDamage( const CTakeDamageInfo &info );
	float		GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info );
	Vector		GetShootEnemyDir( const Vector &shootOrigin, bool bNoisy = true );
	Vector		GetActualShootPosition( const Vector &shootOrigin );
	void		HandleAnimEvent( animevent_t *pEvent );

	void		BleedThink();
	void		StartBleeding();
	void		StopBleeding();
	inline bool	IsBleeding() { return m_bIsBleeding; }

	void		HandleManhackSpawn( CAI_BaseNPC *pNPC );

	void		Event_Killed( const CTakeDamageInfo &info );
	void		Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );

	Activity	GetFlinchActivity( bool bHeavyDamage, bool bGesture );
	bool		IsHeavyDamage( const CTakeDamageInfo &info );
	Activity	Weapon_TranslateActivity( Activity eNewActivity );

	WeaponProficiency_t CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );

	bool		DoHolster( void );
	bool		DoUnholster( void );
	bool		Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
	void		PlayDeploySound( CBaseCombatWeapon *pWeapon );

	bool		IsJumpLegal( const Vector & startPos, const Vector & apex, const Vector & endPos ) const;

	bool		GetGameTextSpeechParams( hudtextparms_t &params );

	bool		AllowedToIgnite( void ) { return false; }

	// For special base Combine behaviors
	bool		IsMajorCharacter() { return true; }

	int			GetArmorValue() { return m_ArmorValue; }

	enum
	{
		// Sorted by range
		WEAPONSWITCH_SHOTGUN,
		WEAPONSWITCH_AR2,
		WEAPONSWITCH_CROSSBOW,

		WEAPONSWITCH_COUNT,
	};

protected:
	//=========================================================
	// Clone Cop schedules
	//=========================================================
	enum
	{
		SCHED_COMBINE_FLANK_LINE_OF_FIRE = BaseClass::NEXT_SCHEDULE,
		SCHED_COMBINE_MERCILESS_RANGE_ATTACK1,
		SCHED_COMBINE_MERCILESS_SUPPRESS,
		NEXT_SCHEDULE,
	};

	DEFINE_CUSTOM_AI;

private:

	static int gm_nBloodAttachment;
	static float gm_flBodyRadius;

	int		m_ArmorValue;
	bool	m_bIsBleeding;

	bool	m_bThrowXenGrenades;
};

#endif
