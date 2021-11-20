#pragma once

#include <EHSN.h>

namespace Carl
{
	enum PACKET_TYPES : EHSN::net::PacketType
	{
		PT_ECHO_REQUEST = EHSN::net::SPT_FIRST_FREE_PACKET_TYPE,
		PT_ECHO_REPLY,
		PT_RENDER_FRAME,
		PT_CAPTURE_FRAME,
		PT_WAVEFORMATEX,
	};
}