//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for EZ2.
//
//=============================================================================//

#ifndef EZ2_PLAYER_H
#define EZ2_PLAYER_H
#pragma once

#include "hl2_player.h"
#include "ai_speech.h"
#include "ai_playerally.h"
#include "ai_sensorydummy.h"

class CAI_PlayerNPCDummy;

//=============================================================================
// >> EZ2_PLAYER
//=============================================================================
class CEZ2_Player : public CAI_ExpresserHost<CHL2_Player>
{
	DECLARE_CLASS(CEZ2_Player, CAI_ExpresserHost<CHL2_Player>);
public:
	void			UpdateOnRemove( void );

	virtual void    ModifyOrAppendCriteria(AI_CriteriaSet& criteriaSet);
	virtual bool	IsAllowedToSpeak(AIConcept_t concept = NULL);
	virtual bool    SpeakIfAllowed(AIConcept_t concept, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL);
	virtual bool    SpeakIfAllowed(AIConcept_t concept, AI_CriteriaSet& modifiers, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL);
	bool			SelectSpeechResponse( AIConcept_t concept, AI_CriteriaSet *modifiers, CBaseEntity *pTarget, AISpeechSelection_t *pSelection );
	virtual void	PostConstructor(const char * szClassname);
	virtual CAI_Expresser * CreateExpresser(void);
	virtual CAI_Expresser * GetExpresser() { return m_pExpresser;  }

	void			ModifyOrAppendDamageCriteria(AI_CriteriaSet & set, const CTakeDamageInfo & info, bool bPlayer = true);
	void			ModifyOrAppendEnemyCriteria(AI_CriteriaSet & set, CBaseEntity * pEnemy);
	void			ModifyOrAppendSquadCriteria(AI_CriteriaSet & set);
	void			ModifyOrAppendWeaponCriteria(AI_CriteriaSet & set, CBaseEntity * pWeapon);

	// "Speech target" is a thing from CAI_PlayerAlly mostly used for things like Q&A.
	// I'm using it here to refer to the player's allies in player dialogue. (shouldn't be used for enemies)
	void			ModifyOrAppendSpeechTargetCriteria(AI_CriteriaSet &set, CBaseEntity *pTarget);

	void			InputAnswerQuestion( inputdata_t &inputdata );

	void			OnPickupWeapon(CBaseCombatWeapon *pNewWeapon);
	virtual int		OnTakeDamage_Alive(const CTakeDamageInfo &info);
	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	void			Event_KilledOther(CBaseEntity * pVictim, const CTakeDamageInfo & info);
	void			Event_Killed( const CTakeDamageInfo &info );
	bool			CommanderExecuteOne(CAI_BaseNPC *pNpc, const commandgoal_t &goal, CAI_BaseNPC **Allies, int numAllies);

	void			Event_NPCKilled( CAI_BaseNPC *pVictim, const CTakeDamageInfo &info );
	void			AllyKilled(CBaseEntity *pVictim, const CTakeDamageInfo &info);

	// Blixibon - StartScripting for gag replacement
	inline bool			IsInAScript( void ) { return m_bInAScript; }
	inline void			SetInAScript( bool bScript ) { m_bInAScript = bScript; }
	void				InputStartScripting( inputdata_t &inputdata );
	void				InputStopScripting( inputdata_t &inputdata );

	// Blixibon - Speech thinking implementation
	void				DoSpeechAI();
	bool				DoIdleSpeech();
	bool				DoCombatSpeech();

	void				MeasureEnemies(int &iVisibleEnemies, int &iCloseEnemies);

	AI_CriteriaSet	GetSoundCriteria( CSound *pSound, float flDist );

	bool				ReactToSound( CSound *pSound, float flDist );

	void				SetSpeechFilter( CAI_SpeechFilter *pFilter )	{ m_hSpeechFilter = pFilter; }
	CAI_SpeechFilter	*GetSpeechFilter( void )						{ return m_hSpeechFilter; }

	CAI_PlayerNPCDummy *GetNPCComponent() { return m_hNPCComponent.Get(); }
	void				CreateNPCComponent();
	void				RemoveNPCComponent();

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

protected:
	virtual	void	PostThink(void);

	// A lot of combat-related concepts modify categorized criteria directly for specific subjects.
	// 
	// Modifiers will always overwrite automatic criteria, so we don't have to worry about this overwriting the desired "enemy",
	// but we do have to worry about doing a bunch of useless measurements in general criteria that are just gonna be overwritten.
	// 
	// As a result, each relevant category marks itself as "used" until the next time the original ModifyOrAppendCriteria() is called.
	enum PlayerCriteria_t
	{
		PLAYERCRIT_DAMAGE,
		PLAYERCRIT_ENEMY,
		PLAYERCRIT_SQUAD,
		PLAYERCRIT_WEAPON,
		PLAYERCRIT_SPEECHTARGET,
	};

	inline void			MarkCriteria(PlayerCriteria_t crit) { m_iCriteriaAppended |= (1 << crit); }
	inline bool			IsCriteriaModified(PlayerCriteria_t crit) { return (m_iCriteriaAppended & (1 << crit)) != 0; }
	inline void			ResetPlayerCriteria() { m_iCriteriaAppended = 0; }

private:
	CAI_Expresser * m_pExpresser;

	bool			m_bInAScript;

	CHandle<CAI_PlayerNPCDummy> m_hNPCComponent;
	float			m_flNextSpeechTime;
	CHandle<CAI_SpeechFilter>	m_hSpeechFilter;

	int				m_iVisibleEnemies;
	int				m_iCloseEnemies;

	// See PlayerCriteria_t
	int				m_iCriteriaAppended;
};

//-----------------------------------------------------------------------------
// Purpose: Sensory dummy for Bad Cop component
// 
// NPC created by Blixibon.
//-----------------------------------------------------------------------------
class CAI_PlayerNPCDummy : public CAI_SensingDummy<CAI_BaseNPC>
{
	DECLARE_CLASS(CAI_PlayerNPCDummy, CAI_SensingDummy<CAI_BaseNPC>);
	DECLARE_DATADESC();
public:

	// Don't waste CPU doing sensing code.
	// We now need hearing for state changes and stuff, but sight isn't necessary at the moment.
	//int		GetSensingFlags( void ) { return SENSING_FLAGS_DONT_LOOK /*| SENSING_FLAGS_DONT_LISTEN*/; }

	void	Spawn();

	void	ModifyOrAppendOuterCriteria(AI_CriteriaSet & set);

	void	RunAI( void );
	void	GatherEnemyConditions( CBaseEntity *pEnemy );
	int 	TranslateSchedule( int scheduleType );

	// Base class's sound interests include combat and danger, add relevant scents onto it
	int		GetSoundInterests( void ) { return BaseClass::GetSoundInterests() | SOUND_PHYSICS_DANGER | SOUND_CARCASS | SOUND_MEAT; }
	bool	QueryHearSound( CSound *pSound );

	//---------------------------------------------------------------------------------------------
	// Override a bunch of stuff to redirect to our outer.
	//---------------------------------------------------------------------------------------------
	Vector				EyePosition() { return GetOuter()->EyePosition(); }
	const QAngle		&EyeAngles() { return GetOuter()->EyeAngles(); }
	void				EyePositionAndVectors( Vector *pPosition, Vector *pForward, Vector *pRight, Vector *pUp ) { GetOuter()->EyePositionAndVectors(pPosition, pForward, pRight, pUp); }
	const QAngle		&LocalEyeAngles() { return GetOuter()->LocalEyeAngles(); }
	void				EyeVectors( Vector *pForward, Vector *pRight = NULL, Vector *pUp = NULL ) { GetOuter()->EyeVectors(pForward, pRight, pUp); }

	Vector				HeadDirection2D( void ) { return GetOuter()->HeadDirection2D(); }
	Vector				HeadDirection3D( void ) { return GetOuter()->HeadDirection3D(); }
	Vector				EyeDirection2D( void ) { return GetOuter()->EyeDirection2D(); }
	Vector				EyeDirection3D( void ) { return GetOuter()->EyeDirection3D(); }

	Disposition_t		IRelationType( CBaseEntity *pTarget )		{ return GetOuter()->IRelationType(pTarget); }
	int					IRelationPriority( CBaseEntity *pTarget );	//{ return GetOuter()->IRelationPriority(pTarget); }
	// NPCs seem to be able to see the player inappropriately with these overrides to FVisible()
	//bool				FVisible ( CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL ) { return GetOuter()->FVisible(pEntity, traceMask, ppBlocker); }
	//bool				FVisible( const Vector &vecTarget, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL )	{ return GetOuter()->FVisible( vecTarget, traceMask, ppBlocker ); }
	bool				FInViewCone( CBaseEntity *pEntity ) { return GetOuter()->FInViewCone(pEntity); }
	bool				FInViewCone( const Vector &vecSpot ) { return GetOuter()->FInViewCone(vecSpot); }

	//---------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------
	bool		IsPlayerAlly(CBasePlayer *pPlayer = NULL) { return false; }
	bool		IsSilentSquadMember() const 	{ return true; }
	bool		CanBeAnEnemyOf( CBaseEntity *pEnemy ) { return false; } // A sensing dummy is NEVER a valid enemy
	bool		CanBeSeenBy( CAI_BaseNPC *pNPC ) { return false; } // A sensing dummy is NEVER visible

	Class_T	Classify( void ) { return CLASS_NONE; }

	CEZ2_Player        *GetOuter() { return m_hOuter.Get(); }
	void            SetOuter(CEZ2_Player *pOuter) { m_hOuter.Set(pOuter); }

protected:
	//-----------------------------------------------------
	// Conditions, Schedules, Tasks
	//-----------------------------------------------------
	enum
	{
		//COND_EXAMPLE = BaseClass::NEXT_CONDITION,
		//NEXT_CONDITION,

		SCHED_PLAYERDUMMY_ALERT_STAND = BaseClass::NEXT_SCHEDULE,
		NEXT_SCHEDULE,

		//TASK_EXAMPLE = BaseClass::NEXT_TASK,
		//NEXT_TASK,

		//AE_EXAMPLE = LAST_SHARED_ANIMEVENT

	};

	CHandle<CEZ2_Player> m_hOuter;

	DEFINE_CUSTOM_AI;
};

#endif	//EZ2_PLAYER_H