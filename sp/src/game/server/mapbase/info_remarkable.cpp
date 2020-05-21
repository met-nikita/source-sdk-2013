#include "cbase.h"
#include "info_remarkable.h"

BEGIN_DATADESC( CInfoRemarkable )
    DEFINE_KEYFIELD( m_iszContextSubject, FIELD_STRING, "contextsubject" ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	DEFINE_FIELD( m_iTimesRemarked, FIELD_INTEGER ),

	// Inputs	
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()

// info remarkables, similar to info targets, are like point entities except you can force them to spawn on the client
void CInfoRemarkable::Spawn( void )
{
	BaseClass::Spawn();

	AddFlag( FL_OBJECT );
}

//------------------------------------------------------------------------------
// Purpose: Create criteria set for this remarkable
//------------------------------------------------------------------------------
AI_CriteriaSet& CInfoRemarkable::GetModifiers( CBaseEntity * pEntity )
{
	Assert( pEntity != NULL );

	AI_CriteriaSet * modifiers = new AI_CriteriaSet();
	float flDist = ( this->GetAbsOrigin() - pEntity->GetAbsOrigin() ).Length();

	modifiers->AppendCriteria( "subject", STRING( this->GetContextSubject() ) );
	modifiers->AppendCriteria( "distance", UTIL_VarArgs( "%f", flDist ) );
	modifiers->AppendCriteria( "timesremarked", UTIL_VarArgs( "%i", m_iTimesRemarked ) );
	return *modifiers;
}

//------------------------------------------------------------------------------
// Purpose: Input handler to turn on this trigger.
//------------------------------------------------------------------------------
void CInfoRemarkable::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}


//------------------------------------------------------------------------------
// Purpose: Input handler to turn off this trigger.
//------------------------------------------------------------------------------
void CInfoRemarkable::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

LINK_ENTITY_TO_CLASS( info_remarkable, CInfoRemarkable );