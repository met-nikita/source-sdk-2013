//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
// 
// $NoKeywords: $
//=============================================================================//
#ifndef AI_BEHAVIOR_BEAST_H
#define AI_BEHAVIOR_BEAST_H

#include "ai_basenpc.h"
#include "ai_behavior.h"
#include "npc_basepredator.h"
#include "GameEventListener.h"

// 
// Beast Behavior
// 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAI_BeastBehavior : public CAI_SimpleBehavior, public CGameEventListener
{
	DECLARE_CLASS( CAI_BeastBehavior, CAI_SimpleBehavior );
public:
	DECLARE_DATADESC();
	CAI_BeastBehavior();

	enum
	{
		// Schedules
		SCHED_BEAST_STAY_HOME = BaseClass::NEXT_SCHEDULE,
		SCHED_BEAST_COME_HOME,
		SCHED_BEAST_TELEPORT_HOME,
		NEXT_SCHEDULE,
		
		// Tasks
		TASK_BEAST_FIND_HOME = BaseClass::NEXT_TASK,
		TASK_BEAST_BE_HOME,
		TASK_TELEPORT_HOME,
		NEXT_TASK,
		
		// Conditions
		COND_BEAST_FORCE_HOME = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION,
	};

	virtual const char *GetName() { return "Beast"; }

	void	GoHome( bool bTeleport = false );

	void	HandleLeaveHome();

	void	EndScheduleSelection( void );
	int		SelectSchedule( void );
	void	GatherConditions( void );
	int		TranslateSchedule( int scheduleType );
	void	BuildScheduleTestBits( void );
	bool	CanSelectSchedule( void );

	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );

	bool	QueryHearSound( CSound *pSound );
	bool	IsInterruptable( void );

	void	FireGameEvent( IGameEvent *event );

	bool	m_bAtHome;
	bool	m_bTeleport;

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;
};

#endif // AI_BEHAVIOR_BEAST_H