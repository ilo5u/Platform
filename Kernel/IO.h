#pragma once

constexpr int BUFLEN = 2048;

struct Packet
{
	enum class Type
	{
		INVALID = 0x0000,

		LOGIN_REQUEST = 0x0001,
		LOGIN_SUCCESS = 0x0002,
		LOGIN_FAILED = 0x0003,
		LOGON_REQUEST = 0x0004,
		LOGON_SUCCESS = 0x0005,
		LOGON_FAILED = 0x0006,
		LOGOUT = 0x0007,

		PVE_RESULT = 0x0010,
		PVP_REQUEST = 0x0020,
		PVP_FAILED = 0x0030,
		PVP_BATTLE = 0x0040,
		PVP_RESULT = 0x0050,
		PVE_MESSAGE = 0x0060,
		PVP_MESSAGE = 0x0070,
		PVP_UPDATE = 0x0080,
		PVP_ACCEPT = 0x0090,
		PVP_CANCEL = 0x00A0,
		PVP_BUSY = 0x00C0,

		SET_ONLINE_USERS = 0x0100,
		UPDATE_ONLINE_USERS = 0x0200,
		UPDATE_POKEMENS = 0x0300,
		UPDATE_RANKLIST = 0x0400,
		GET_ONLINE_USERS = 0x0500,
		PROMOTE_POKEMEN = 0x0600,
		ADD_POKEMEN = 0x0700,
		SUB_POKEMEN = 0x0800,
		GET_POKEMENS_BY_USER = 0x0900,
		SET_POKEMENS_BY_USER = 0x0A00,
		SET_POKEMENS_OVER = 0x0B00,
		RENEW_RANKLIST = 0x0C00,
		SET_RANKLIST = 0x0D00
	};

	typedef char Data[BUFLEN];

	Type type;
	Data data;

	Packet();
	Packet(const Packet& other);
	Packet(Packet&& other);
	Packet& operator=(const Packet& other);
	Packet& operator=(Packet&& other);
};