//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Achievements for Entropy : Zero 2.
// The implementation is heavily based on the achievements from Entropy : Zero
//
//=============================================================================

#include "cbase.h"

#ifdef CLIENT_DLL

#include "achievementmgr.h"
#include "baseachievement.h"
#include "point_bonusmaps_accessor.h"
#include "hl2_shareddefs.h"

//=============================================================================
// 
// THIS CODE IS DEACTIVATED!
// (and these are not real achievements)
// 
// This is meant to demonstrate how new challenge map achievements could work
// within E:Z2 and have most of the work needed for them immediately available.
// 
//=============================================================================
#if 0

#define NUM_CHALLENGE_MAPS 8
#define NUM_CHALLENGES 99 // TODO

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
class CBaseAchievementEZ2ChallengeFullMedals : public CBaseAchievement
{
protected:

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "challenge_update" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "challenge_update" ) )
		{
			int iChallenge = event->GetInt( "challenge", 0 );
			if (iChallenge != BonusChallenge())
				return;

			int numMedals = event->GetInt( Medal(), 0 );
			SetCount( numMedals );
		}
	}

	// Show progress for this achievement
	virtual bool ShouldShowProgressNotification() { return true; }

	virtual int BonusChallenge() { return EZ_CHALLENGE_NONE; }
	virtual const char *Medal() { return "numgold"; }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get gold medals on all challenges of certain types
////////////////////////////////////////////////////////////////////////////////////////////////////
class CAchievementEZ2ChallengeDamageAllGold : public CBaseAchievementEZ2ChallengeFullMedals
{
protected:

	void Init() 
	{
		SetGameDirFilter( "EntropyZero2" );
		SetGoal( NUM_CHALLENGE_MAPS );
	}

	virtual int BonusChallenge() { return EZ_CHALLENGE_DAMAGE; }
	virtual const char *Medal() { return "numgold"; }
};
DECLARE_ACHIEVEMENT( CAchievementEZ2ChallengeDamageAllGold, ACHIEVEMENT_EZ2_CHALLENGE_DAMAGE_ALL_GOLD, "ACH_EZ2_CHALLENGE_DAMAGE_ALL_GOLD", 5 );

class CAchievementEZ2ChallengeBulletsAllGold : public CBaseAchievementEZ2ChallengeFullMedals
{
protected:

	void Init()
	{
		SetGameDirFilter( "EntropyZero2" );
		SetGoal( NUM_CHALLENGE_MAPS );
	}

	virtual int BonusChallenge() { return EZ_CHALLENGE_BULLETS; }
	virtual const char *Medal() { return "numgold"; }
};
DECLARE_ACHIEVEMENT( CAchievementEZ2ChallengeBulletsAllGold, ACHIEVEMENT_EZ2_CHALLENGE_BULLETS_ALL_GOLD, "ACH_EZ2_CHALLENGE_BULLETS_ALL_GOLD", 5 );

class CAchievementEZ2ChallengeTimeAllGold : public CBaseAchievementEZ2ChallengeFullMedals
{
protected:

	void Init()
	{
		SetGameDirFilter( "EntropyZero2" );
		SetGoal( NUM_CHALLENGE_MAPS );
	}

	virtual int BonusChallenge() { return EZ_CHALLENGE_TIME; }
	virtual const char *Medal() { return "numgold"; }
};
DECLARE_ACHIEVEMENT( CAchievementEZ2ChallengeTimeAllGold, ACHIEVEMENT_EZ2_CHALLENGE_TIME_ALL_GOLD, "ACH_EZ2_CHALLENGE_TIME_ALL_GOLD", 5 );

class CAchievementEZ2ChallengeKillsAllGold : public CBaseAchievementEZ2ChallengeFullMedals
{
protected:

	void Init()
	{
		SetGameDirFilter( "EntropyZero2" );
		SetGoal( NUM_CHALLENGE_MAPS );
	}

	virtual int BonusChallenge() { return EZ_CHALLENGE_KILLS; }
	virtual const char *Medal() { return "numgold"; }
};
DECLARE_ACHIEVEMENT( CAchievementEZ2ChallengeKillsAllGold, ACHIEVEMENT_EZ2_CHALLENGE_KILLS_ALL_GOLD, "ACH_EZ2_CHALLENGE_KILLS_ALL_GOLD", 5 );

class CAchievementEZ2ChallengeMassAllGold : public CBaseAchievementEZ2ChallengeFullMedals
{
protected:

	void Init()
	{
		SetGameDirFilter( "EntropyZero2" );
		SetGoal( NUM_CHALLENGE_MAPS );
	}

	virtual int BonusChallenge() { return EZ_CHALLENGE_MASS; }
	virtual const char *Medal() { return "numgold"; }
};
DECLARE_ACHIEVEMENT( CAchievementEZ2ChallengeMassAllGold, ACHIEVEMENT_EZ2_CHALLENGE_MASS_ALL_GOLD, "ACH_EZ2_CHALLENGE_MASS_ALL_GOLD", 5 );

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get bronze medals on all challenges
////////////////////////////////////////////////////////////////////////////////////////////////////
class CBaseAchievementEZ2ChallengesAllBronze : public CBaseAchievement
{
protected:

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGameDirFilter( "EntropyZero2" );
		SetGoal( NUM_CHALLENGES );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "challenge_map_complete" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "challenge_map_complete" ) )
		{
			int numMedals = event->GetInt( "numbronze", 0 );
			SetCount( numMedals );
		}
	}

	// Show progress for this achievement
	virtual bool ShouldShowProgressNotification() { return true; }
};
DECLARE_ACHIEVEMENT( CBaseAchievementEZ2ChallengesAllBronze, ACHIEVEMENT_EZ2_CHALLENGES_ALL_BRONZE, "ACH_EZ2_CHALLENGES_ALL_BRONZE", 5 );

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get silver medals on all challenges
////////////////////////////////////////////////////////////////////////////////////////////////////
class CBaseAchievementEZ2ChallengesAllSilver : public CBaseAchievement
{
protected:

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGameDirFilter( "EntropyZero2" );
		SetGoal( NUM_CHALLENGES );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "challenge_map_complete" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "challenge_map_complete" ) )
		{
			int numMedals = event->GetInt( "numsilver", 0 );
			SetCount( numMedals );
		}
	}

	// Show progress for this achievement
	virtual bool ShouldShowProgressNotification() { return true; }
};
DECLARE_ACHIEVEMENT( CBaseAchievementEZ2ChallengesAllSilver, ACHIEVEMENT_EZ2_CHALLENGES_ALL_SILVER, "ACH_EZ2_CHALLENGES_ALL_SILVER", 5 );

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get gold medals on all challenges
////////////////////////////////////////////////////////////////////////////////////////////////////
class CBaseAchievementEZ2ChallengesAllGold : public CBaseAchievement
{
protected:

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGameDirFilter( "EntropyZero2" );
		SetGoal( NUM_CHALLENGES );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "challenge_map_complete" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "challenge_map_complete" ) )
		{
			int numMedals = event->GetInt( "numgold", 0 );
			SetCount( numMedals );
		}
	}

	// Show progress for this achievement
	virtual bool ShouldShowProgressNotification() { return true; }
};
DECLARE_ACHIEVEMENT( CBaseAchievementEZ2ChallengesAllGold, ACHIEVEMENT_EZ2_CHALLENGES_ALL_GOLD, "ACH_EZ2_CHALLENGES_ALL_GOLD", 5 );
#endif

#endif // CLIENT_DLL
