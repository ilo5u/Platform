﻿#include "pch.h"
#include "Kernel.h"

using namespace Platform;
namespace Kernel
{
	std::string WStringToString(const wchar_t* wstr)
	{
		int iLen = WideCharToMultiByte(CP_ACP, 0, wstr,
			(int)std::wcslen(wstr), NULL, 0, NULL, NULL);

		if (iLen <= 0)
			return { "" };

		char *szDst = new char[iLen + 1];
		if (szDst == nullptr)
			return { "" };

		WideCharToMultiByte(CP_ACP, 0, wstr,
			(int)std::wcslen(wstr), szDst, iLen, NULL, NULL);
		szDst[iLen] = '\0';

		std::string str{ szDst };
		delete[] szDst;

		return str;
	}

	std::wstring StringToWString(const char* str)
	{
		int iLen = MultiByteToWideChar(CP_ACP, 0, str,
			(int)std::strlen(str), NULL, 0);

		if (iLen <= 0)
			return { L"" };

		wchar_t *wszDst = new wchar_t[iLen + 1];
		if (wszDst == nullptr)
			return { L"" };

		MultiByteToWideChar(CP_ACP, 0, str,
			(int)std::strlen(str), wszDst, iLen);
		wszDst[iLen] = L'\0';

		if (wszDst[0] == 0xFEFF)
			for (int i = 0; i < iLen; ++i)
				wszDst[i] = wszDst[i + 1];

		std::wstring wstr{ wszDst };
		delete wszDst;
		wszDst = nullptr;

		return wstr;
	}

	Pokemen::Pokemen(int type, int level) :
		instance(static_cast<::Pokemen::PokemenType>(type), level)
	{
	}

	Pokemen::Pokemen(int level) :
		instance(::Pokemen::PokemenType::DEFAULT, level)
	{
	}

	Pokemen::~Pokemen()
	{
	}

	static ::Pokemen::Pokemen aiPlayer(::Pokemen::PokemenType::DEFAULT, 1);
	void Pokemen::SetAIPlayer()
	{
		aiPlayer = instance;
	}

	Property Pokemen::GetProperty()
	{
		return Property{
			instance.GetId(),
			(int)instance.GetType(),
			ref new Platform::String(
				StringToWString(instance.GetName().c_str()).c_str()
			),

			instance.GetHpoints(),
			instance.GetAttack(),
			instance.GetDefense(),
			instance.GetAgility(),

			instance.GetInterval(),
			instance.GetCritical(),
			instance.GetHitratio(),
			instance.GetParryratio(),

			instance.GetCareer(),
			instance.GetExp(),
			instance.GetLevel(),

			instance.GetPrimarySkill(),
			1 - instance.GetPrimarySkill()
		};
	}

	void Pokemen::SetProperty(int id, int type,
		int hpoint, int attack, int defense, int agility,
		int interval, int critical, int hitratio, int parryratio, 
		int career, int exp, int level)
	{
		::Pokemen::Property prop;
		prop.m_id = id;
		prop.m_type = static_cast<::Pokemen::PokemenType>(type);
		prop.m_name = ::Pokemen::String();
		prop.m_hpoints = hpoint;
		prop.m_attack = attack;
		prop.m_defense = defense;
		prop.m_agility = agility;
		prop.m_interval = interval;
		prop.m_critical = critical;
		prop.m_hitratio = hitratio;
		prop.m_parryratio = parryratio;
		prop.m_exp = exp;
		prop.m_level = level;

		instance.RenewProperty(prop, career);
	}

	Core::Core() :
		netDriver(), 
		stage(), battletype(false),
		pokemens()
	{
	}

	Core::~Core()
	{
	}

	void Core::SendMessage(Message msg)
	{
		if (!netDriver.IsConnected())
			return;

		Packet sendPacket;
		switch (msg.type)
		{
		case MsgType::LOGIN_REQUEST:
			sendPacket.type = PacketType::LOGIN_REQUEST;
			break;

		case MsgType::LOGON_REQUEST:
			sendPacket.type = PacketType::LOGON_REQUEST;
			break;

		case MsgType::LOGOUT:
			sendPacket.type = PacketType::LOGOUT;
			pokemens.clear();
			break;

		case MsgType::PROMOTE_POKEMEN:
			sendPacket.type = PacketType::PROMOTE_POKEMEN;
			break;

		case MsgType::GET_ONLINE_USERS:
			sendPacket.type = PacketType::GET_ONLINE_USERS;
			break;

		case MsgType::ADD_POKEMEN:
			sendPacket.type = PacketType::ADD_POKEMEN;
			break;

		case MsgType::SUB_POKEMEN:
			{
				sendPacket.type = PacketType::SUB_POKEMEN;
				int removeId = 0;
				sscanf(WStringToString(msg.data->Data()).c_str(),
					"%d", &removeId);
				if (removeId > 0)
				{
					pokemens.remove_if([&removeId](const HPokemen& pokemen) {
						return pokemen->GetId() == removeId;
					});
				}
			}
			break;

		case MsgType::UPDATE_POKEMENS:
			sendPacket.type = PacketType::UPDATE_POKEMENS;
			break;

		case MsgType::UPDATE_RANKLIST:
			sendPacket.type = PacketType::UPDATE_RANKLIST;
			break;

		case MsgType::GET_POKEMENS_BY_USER:
			sendPacket.type = PacketType::GET_POKEMENS_BY_USER;
			break;

		case MsgType::PVP_REQUEST:
			sendPacket.type = PacketType::PVP_REQUEST;
			break;

		case MsgType::PVP_CANCEL:
			sendPacket.type = PacketType::PVP_CANCEL;
			break;

		case MsgType::PVP_ACCEPT:
			sendPacket.type = PacketType::PVP_ACCEPT;
			break;

		case MsgType::PVP_BUSY:
			sendPacket.type = PacketType::PVP_BUSY;
			break;

		case MsgType::PVP_MESSAGE:
			sendPacket.type = PacketType::PVP_MESSAGE;
			break;

		case MsgType::PVP_RESULT:
			sendPacket.type = PacketType::PVP_RESULT;
			break;

		case MsgType::PVP_BATTLE:
			sendPacket.type = PacketType::PVP_BATTLE;
			break;

		default:
			break;
		}
		std::memcpy(
			(LPCH)sendPacket.data,
			WStringToString(msg.data->Data()).c_str(),
			msg.data->Length()
		);
		netDriver.SendPacket(sendPacket);

		if (sendPacket.type == PacketType::LOGOUT)
			netDriver.Disconnect();
	}

	Message Core::ReadOfflineMessage()
	{
		if (stage.IsRunning()
			|| stage.ReadyForRead())
		{
			::Pokemen::BattleMessage msg = stage.ReadMessage();
			switch (msg.type)
			{
			case BattleMessage::Type::DISPLAY:
				if (!battletype)
				{
					return Message{
						MsgType::PVE_MESSAGE,
						ref new Platform::String(StringToWString(msg.options.c_str()).c_str())
					};
				}
				else
				{
					Packet packet;
					packet.type = PacketType::PVP_MESSAGE;
					sprintf(packet.data, "%s\n", msg.options.c_str());
					netDriver.SendPacket(packet);

					return Message{
						MsgType::PVP_MESSAGE,
						ref new Platform::String(StringToWString(msg.options.c_str()).c_str())
					};
				}
				
			case BattleMessage::Type::RESULT:
			{
				/* 向服务器回传战斗结果 */
				Packet packet;
				sprintf(packet.data, "%s", msg.options.c_str());

				if (!battletype)
				{
					/* 升级对应小精灵 */
					int pokemenId;
					int raiseExp = 0;
					if (msg.options[0] == 'F')
					{
						sscanf(msg.options.c_str(), "F\n%d\n%d\n", &pokemenId, &raiseExp);
						raiseExp += raiseExp;
					}
					else if (msg.options[0] == 'S')
						sscanf(msg.options.c_str(), "S\n%d\n%d\n", &pokemenId, &raiseExp);
					Pokemens::iterator it = std::find_if(
						pokemens.begin(), pokemens.end(), [&pokemenId](const HPokemen& pokemen) {
						return pokemen->GetId() == pokemenId;
					}
					);
					if (it != pokemens.end())
					{
						/* 向UI回传UPDATE信息 */
						char prop[BUFSIZ];
						sprintf(prop,
							"%d,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
							(*it)->GetId(), (int)(*it)->GetType(), (*it)->GetName().c_str(),
							(*it)->GetHpoints(), (*it)->GetAttack(), (*it)->GetDefense(), (*it)->GetAgility(),
							(*it)->GetInterval(), (*it)->GetCritical(), (*it)->GetHitratio(), (*it)->GetParryratio(),
							(*it)->GetCareer(), (*it)->GetExp(), (*it)->GetLevel()
						);
						msg.options.append(prop);

						/* 升级 */
						(*it)->Upgrade(raiseExp);

						sprintf(prop,
							"%d,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
							(*it)->GetId(), (int)(*it)->GetType(), (*it)->GetName().c_str(),
							(*it)->GetHpoints(), (*it)->GetAttack(), (*it)->GetDefense(), (*it)->GetAgility(),
							(*it)->GetInterval(), (*it)->GetCritical(), (*it)->GetHitratio(), (*it)->GetParryratio(),
							(*it)->GetCareer(), (*it)->GetExp(), (*it)->GetLevel()
						);
						msg.options.append(prop);

						sprintf(packet.data + std::strlen(packet.data),
							"%d\n%d\n%s\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
							(*it)->GetId(), (int)(*it)->GetType(), (*it)->GetName().c_str(),
							(*it)->GetHpoints(), (*it)->GetAttack(), (*it)->GetDefense(), (*it)->GetAgility(),
							(*it)->GetInterval(), (*it)->GetCritical(), (*it)->GetHitratio(), (*it)->GetParryratio(),
							(*it)->GetCareer(), (*it)->GetExp(), (*it)->GetLevel()
						);
						/* 向服务器回传UPDATE信息 */
						packet.type = PacketType::PVE_RESULT;
						netDriver.SendPacket(packet);
					}
					return Message{
						MsgType::PVE_RESULT,
						ref new Platform::String(StringToWString(msg.options.c_str()).c_str())
					};
				}
				else
				{
					packet.type = PacketType::PVP_RESULT;
					netDriver.SendPacket(packet);
					return Message{
						MsgType::PVP_RESULT,
						ref new Platform::String(StringToWString(msg.options.c_str()).c_str())
					};
				}
			}

			default:
				break;
			}
		}
		return { };
	}

	Message Core::ReadOnlineMessage()
	{
		if (!netDriver.IsConnected())
			return { 
				MsgType::DISCONNECT
			};

		try
		{
			Packet recvPacket = netDriver.ReadPacket();
			switch (recvPacket.type)
			{
			case PacketType::LOGIN_SUCCESS:
				return {
					MsgType::LOGIN_SUCCESS,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::LOGIN_FAILED:
				return {
					MsgType::LOGIN_FAILED,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::LOGON_SUCCESS:
				return {
					MsgType::LOGON_SUCCESS,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::LOGON_FAILED:
				return {
					MsgType::LOGON_FAILED,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::UPDATE_ONLINE_USERS:
				return {
					MsgType::UPDATE_ONLINE_USERS,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::UPDATE_POKEMENS:
				{
					::Pokemen::Property prop;
					int carrer;
					char name[BUFSIZ];
					sscanf(recvPacket.data,
						"%d\n%d\n%s\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
						&prop.m_id, (int*)&prop.m_type, name,
						&prop.m_hpoints, &prop.m_attack, &prop.m_defense, &prop.m_agility,
						&prop.m_interval, &prop.m_critical, &prop.m_hitratio, &prop.m_parryratio,
						&carrer, &prop.m_exp, &prop.m_level
					);
					prop.m_name.assign(name);
					Pokemens::iterator it = std::find_if(pokemens.begin(),
						pokemens.end(), [&prop](const HPokemen& pokemen) {
						return pokemen->GetId() == prop.m_id;
					});
					if (it == pokemens.end())
					{
						pokemens.push_back(new ::Pokemen::Pokemen(
							prop, carrer
						));
					}
					else
					{
						(*it)->RenewProperty(prop, carrer);
					}
				}
				return {
					MsgType::UPDATE_POKEMENS,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::SET_ONLINE_USERS:
				return {
					MsgType::SET_ONLINE_USERS,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::ADD_POKEMEN:
				{
					::Pokemen::Property prop;
					int carrer;
					char name[BUFSIZ];
					sscanf(recvPacket.data,
						"%d\n%d\n%s\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
						&prop.m_id, (int*)&prop.m_type, name,
						&prop.m_hpoints, &prop.m_attack, &prop.m_defense, &prop.m_agility,
						&prop.m_interval, &prop.m_critical, &prop.m_hitratio, &prop.m_parryratio,
						&carrer, &prop.m_exp, &prop.m_level
					);
					prop.m_name.assign(name);
					Pokemens::iterator it = std::find_if(pokemens.begin(),
						pokemens.end(), [&prop](const HPokemen& pokemen) {
						return pokemen->GetId() == prop.m_id;
					});
					if (it == pokemens.end())
					{
						pokemens.push_back(new ::Pokemen::Pokemen(
							prop, carrer
						));
					}
					else
					{
						(*it)->RenewProperty(prop, carrer);
					}
				}	
				return {
					MsgType::ADD_POKEMEN,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::SET_POKEMENS_BY_USER:
				return {
					MsgType::SET_POKEMENS_BY_USER,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::SET_POKEMENS_OVER:
				return {
					MsgType::SET_POKEMENS_OVER,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::RENEW_RANKLIST:
				return {
					MsgType::RENEW_RANKLIST,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::SET_RANKLIST:
				return {
					MsgType::SET_RANKLIST,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::PVP_REQUEST:
				return {
					MsgType::PVP_REQUEST,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::PVP_ACCEPT:
				return {
					MsgType::PVP_ACCEPT,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::PVP_BUSY:
				return {
					MsgType::PVP_BUSY,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::PVP_CANCEL:
				return {
					MsgType::PVP_CANCEL,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::PVP_MESSAGE:
				return {
					MsgType::PVP_MESSAGE,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::PVP_RESULT:
				return {
					MsgType::PVP_RESULT,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			case PacketType::PVP_BATTLE:
				return {
					MsgType::PVP_BATTLE,
					ref new Platform::String(StringToWString(recvPacket.data).c_str())
				};

			default:
				return { };
			}
		}
		catch (const std::exception& e)
		{
			OutputDebugStringA(e.what());
			return { };
		}
	}

	bool Core::StartConnection()
	{
		try
		{
			if (netDriver.IsConnected())
				return false;
			return netDriver.Connect();
		}
		catch (const std::exception& e)
		{
			OutputDebugStringA(e.what());
			return false;
		}
	}

	bool Core::CloseConnection()
	{
		return netDriver.Disconnect();
	}

	bool Core::IsConnected()
	{
		return netDriver.IsConnected();
	}

	Platform::String^ Core::GetIP()
	{
		return ref new Platform::String(StringToWString(netDriver.GetIP().c_str()).c_str());
	}

	void Core::SetIP(Platform::String^ newIP)
	{
		netDriver.SetIP(WStringToString(newIP->Data()));
	}

	Property Core::GetPropertyAt(int pokemenId)
	{
		for (Pokemens::const_iterator it = pokemens.begin();
			it != pokemens.end(); ++it)
		{
			if ((*it)->GetId() == pokemenId)
			{
				return {
					(*it)->GetId(),
					(int)(*it)->GetType(),
					ref new Platform::String(
						StringToWString((*it)->GetName().c_str()).c_str()
					),
					(*it)->GetHpoints(),
					(*it)->GetAttack(),
					(*it)->GetDefense(),
					(*it)->GetAgility(),
					(*it)->GetInterval(),
					(*it)->GetCritical(),
					(*it)->GetHitratio(),
					(*it)->GetParryratio(),
					(*it)->GetCareer(),
					(*it)->GetExp(),
					(*it)->GetLevel()
				};
			}
		}
		return { };
	}

	void Core::SetBattlePlayersAndType(int pokemenId, Kernel::Pokemen^ ai, int type, bool battle)
	{
		battletype = battle;
		ai->SetAIPlayer();
		for (Pokemens::const_iterator it = pokemens.begin();
			it != pokemens.end(); ++it)
		{
			if ((*it)->GetId() == pokemenId)
			{
				(*it)->SetPrimarySkill(type);
				stage.SetPlayers(*(*it), aiPlayer);
				break;
			}
		}
	}

	void Core::StartBattle()
	{
		stage.Start();
	}

	void Core::SetBattleOn()
	{
		stage.GoOn();
	}

	void Core::SetBattlePasue()
	{
		stage.Pause();
	}

	void Core::ShutdownBattle()
	{
		stage.Clear();
	}

	bool Core::IsBattleRunning()
	{
		return stage.IsRunning();
	}

}