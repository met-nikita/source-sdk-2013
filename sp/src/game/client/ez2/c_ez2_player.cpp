#include "cbase.h"
#include "c_EZ2_player.h"

#if defined( CEZ2Player )
	#undef CEZ2Player
#endif


IMPLEMENT_CLIENTCLASS_DT( C_EZ2_Player, DT_EZ2_Player, CEZ2_Player )
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_EZ2_Player )
END_PREDICTION_DATA()

C_EZ2_Player::C_EZ2_Player()
{
}


C_EZ2_Player::~C_EZ2_Player()
{
}