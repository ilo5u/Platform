/*  ------------------------------------------
* 描述：
*       客户端与服务器通信模块
* ----------------------------------------- */

#include "pch.h"
#include <thread>

constexpr int PORT = 27893;

#pragma comment (lib, "WS2_32.lib")

static std::string serverIp = "10.201.6.248";

namespace NetIO
{
	NetIO::NetIO() :
		m_connectSocket(NULL), m_serverAddr(),
		m_recvAvailable(nullptr), m_recvLocker(), m_recvPackets(),
		m_sendAvailable(nullptr), m_sendLocker(), m_sendPackets(),
		m_isConnected(false),
		m_recvDriver(), m_sendDriver()
	{
		// 创建控制IO缓冲队列的信号量
		m_sendAvailable = CreateSemaphore(NULL, NULL, 0xFF, NULL);
		m_recvAvailable = CreateSemaphore(NULL, NULL, 0xFF, NULL);
	}

	NetIO::~NetIO()
	{
		Disconnect();

		CloseHandle(m_sendAvailable);
		CloseHandle(m_recvAvailable);
	}

	bool NetIO::IsConnected() const
	{
		return m_isConnected;
	}

	/// <summary>
	/// 连接请求
	/// </summary>
	/// <returns></returns>
	bool NetIO::Connect()
	{
		try
		{
			if (!_InitNetwork_())
				throw std::exception{ "Failed to set the network environment." };

			m_connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (m_connectSocket == INVALID_SOCKET)
			{
				WSACleanup();
				return false;
			}

			m_serverAddr.sin_family = AF_INET;
			m_serverAddr.sin_addr.S_un.S_addr = inet_addr(serverIp.c_str());
			m_serverAddr.sin_port = htons(PORT);
			if (connect(m_connectSocket, (LPSOCKADDR)&m_serverAddr, sizeof(SOCKADDR))
				== SOCKET_ERROR)
			{
				WSACleanup();
				closesocket(m_connectSocket);
				return false;
			}
			m_isConnected = true;

			m_recvDriver 
				= std::move(Thread{ std::bind(&NetIO::_RecvThread_, this) });
			m_sendDriver 
				= std::move(Thread{ std::bind(&NetIO::_SendThread_, this) });
		}
		catch (const std::exception&)
		{
			WSACleanup();
			return false;
		}
		return true;
	}

	bool NetIO::Disconnect()
	{
		// 销毁IO通信线程
		if (m_recvDriver.joinable())
			m_recvDriver.join();

		if (m_sendDriver.joinable())
			m_sendDriver.join();

		// 关闭通信
		closesocket(m_connectSocket);
		WSACleanup();

		// 清空IO缓冲队列
		m_recvLocker.lock();
		while (!m_recvPackets.empty())
			m_recvPackets.pop();
		m_recvLocker.unlock();

		m_sendLocker.lock();
		while (!m_sendPackets.empty())
			m_sendPackets.pop();
		m_sendLocker.unlock();

		return true;
	}

	std::string NetIO::GetIP() const
	{
		return serverIp;
	}

	void NetIO::SetIP(const std::string& newIP)
	{
		if (m_isConnected)
		{
			m_isConnected = false;
			Disconnect();
		}
		serverIp = newIP;
	}

	bool NetIO::SendPacket(const Packet& packet)
	{
		m_sendLocker.lock();

		m_sendPackets.push(packet);
		ReleaseSemaphore(m_sendAvailable, 1, NULL);

		m_sendLocker.unlock();
		return false;
	}

	Packet NetIO::ReadPacket()
	{
		Packet recvPacket;
		WaitForSingleObject(m_recvAvailable, 1000);

		m_recvLocker.lock();
		if (!m_recvPackets.empty())
		{
			recvPacket = m_recvPackets.front();
			m_recvPackets.pop();
		}
		m_recvLocker.unlock();
		return recvPacket;
	}

	bool NetIO::_InitNetwork_()
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NULL)
			return false;
		return true;
	}

	/// <summary>
	/// 接收线程
	/// </summary>
	void NetIO::_RecvThread_()
	{
		int iRecvBytes = 0;
		Packet recvPacket;
		while (m_isConnected)
		{
			std::memset((LPCH)(&recvPacket), 0x0, sizeof(recvPacket));
			iRecvBytes =
				recv(m_connectSocket, (LPCH)(&recvPacket), sizeof(recvPacket), 0);
			if (iRecvBytes > 0)
			{
				m_recvLocker.lock();
				m_recvPackets.push(recvPacket);
				ReleaseSemaphore(m_recvAvailable, 1, NULL);
				m_recvLocker.unlock();
			}
			else
			{
				m_isConnected = false;
				return;
			}
		}
	}

	/// <summary>
	/// 发送线程
	/// </summary>
	void NetIO::_SendThread_()
	{
		Packet sendPacket;
		int iSendBytes = 0;
		while (m_isConnected)
		{
			WaitForSingleObject(m_sendAvailable, 1000);

			bool sendValid = false;
			m_sendLocker.lock();
			if (!m_sendPackets.empty())
			{
				sendValid = true;
				sendPacket = m_sendPackets.front();
				m_sendPackets.pop();
			}

			if (sendPacket.type == Packet::Type::LOGOUT)
				m_isConnected = false;

			m_sendLocker.unlock();
			if (sendValid && sendPacket.type != Packet::Type::INVALID)
			{
				iSendBytes = send(m_connectSocket,
					(LPCH)(&sendPacket),
					sizeof(sendPacket),
					0);

				if (iSendBytes == SOCKET_ERROR)
				{
					m_isConnected = false;
					return;
				}
			}
		}
	}
}

