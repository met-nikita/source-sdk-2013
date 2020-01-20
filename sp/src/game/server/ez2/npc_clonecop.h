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

public:
	CNPC_CloneCop();

	void		Spawn( void );
	void		Precache( void );

	void		DeathSound( const CTakeDamageInfo &info );

	void		ClearAttackConditions( void );
	int			SelectSchedule( void );
	int			TranslateSchedule( int scheduleType );

	float		GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info );
	void		HandleAnimEvent( animevent_t *pEvent );

	void		Event_Killed( const CTakeDamageInfo &info );

	Activity	GetFlinchActivity( bool bHeavyDamage, bool bGesture );
	bool		IsHeavyDamage( const CTakeDamageInfo &info );
	Activity	Weapon_TranslateActivity( Activity eNewActivity );

	WeaponProficiency_t CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );

	bool		IsJumpLegal( const Vector & startPos, const Vector & apex, const Vector & endPos ) const;

	bool		AllowedToIgnite( void ) { return false; }

	// For special base Combine behaviors
	bool		IsMajorCharacter() { return true; }

};

#endif
