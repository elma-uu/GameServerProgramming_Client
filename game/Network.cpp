#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Network.h"
#include "Game.h"
#include "protocol_2026.h"
#include <string>

Network::Network()
{
	mSocket = INVALID_SOCKET;
	ZeroMemory(&mServerAddr, sizeof(mServerAddr));

	// WSAStartup �ʱ�ȭ
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

Network::~Network()
{
	if (mSocket != INVALID_SOCKET)
	{
		closesocket(mSocket);
	}
	WSACleanup();
}

bool Network::Connect(const char* ip, int port)
{
	mServerAddr.sin_family = AF_INET;
	mServerAddr.sin_port = htons(port);
	mServerAddr.sin_addr.s_addr = inet_addr(ip);
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
	if (mSocket == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		printf("Socket creation failed: %d\n", error);
		return false;
	}
	int result = WSAConnect(mSocket, (SOCKADDR*)&mServerAddr, sizeof(mServerAddr), NULL, NULL, NULL, NULL);
	if (result == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		printf("Connection failed: %d\n", error);
		return false;
	}

	// ������ Non-blocking ���� ����
	u_long mode = 1; // Non-blocking
	if (ioctlsocket(mSocket, FIONBIO, &mode) == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		printf("Failed to set non-blocking mode: %d\n", error);
		return false;
	}

	printf("Connected to server successfully\n");
	return true;
}

void Network::Send(void* packet)
{
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	unsigned char packetSize = *p;
	DWORD sent = 0;
	WSABUF wsaBuf;
	wsaBuf.buf = (char*)p;
	wsaBuf.len = packetSize;
	WSASend(mSocket, &wsaBuf, 1, &sent, 0, NULL, NULL);
}

int Network::Receive(char* buffer, int bufferSize)
{
	DWORD received = 0;
	DWORD flags = 0;
	WSABUF wsaBuf;
	wsaBuf.buf = buffer;
	wsaBuf.len = bufferSize;
	int result = WSARecv(mSocket, &wsaBuf, 1, &received, &flags, NULL, NULL);
	return (result == SOCKET_ERROR) ? -1 : static_cast<int>(received);
}

void Network::ProcessPacket(char* recv_packet)
{
	switch (static_cast<PACKET_TYPE>(recv_packet[1]))
	{
	case S2C_LOGIN_RESULT:
	{
		S2C_LoginResult* packet = reinterpret_cast<S2C_LoginResult*>(recv_packet);
		if (packet->result == LOGIN_NEW_USER)
		{
			// First-ever login → show character selection
			GAME.OnLoginSuccess();
		}
		else if (packet->result == LOGIN_SUCCESS)
		{
			// Returning user with saved character → enter game directly
			GAME.OnLoginDirect();
		}
		else
		{
			GAME.SetLoginMessage(std::string(packet->message));
		}
	}
	break;
	case S2C_AVATAR_INFO:
	{
		S2C_AvatarInfo* packet = reinterpret_cast<S2C_AvatarInfo*>(recv_packet);
		int pixelX = packet->x * 50 + 25;
		int pixelY = packet->y * 50 + 25;
		if (!GAME.GetAvatar()->IsAvatarPositionSet()) {
			// First AVATAR_INFO: set spawn position, visual, and all stats
			GAME.GetAvatar()->SetPlayerInfo(packet->playerId, pixelX, pixelY,
				packet->hp, packet->max_hp, packet->exp, packet->level);
			GAME.GetAvatar()->SetMyVisualId(packet->visualId);
			GAME.GetAvatar()->MarkAvatarPositionSet();
		} else {
			// Subsequent (kill reward, level-up): update stats + visual, never snap position
			GAME.GetAvatar()->SetMyVisualId(packet->visualId);
			GAME.GetAvatar()->UpdateAvatarStats(packet->hp, packet->max_hp,
				packet->exp, packet->level);
		}
		printf("Avatar info received - Tile: (%d, %d) -> Pixel: (%d, %d), visual=%d\n",
			packet->x, packet->y, pixelX, pixelY, packet->visualId);
	}
	break;
	case S2C_ADD_OBJECT:
	{
		S2C_AddObject* packet = reinterpret_cast<S2C_AddObject*>(recv_packet);
		// �������� ���� Ÿ�� ��ǥ(x, y)�� �ȼ� ��ǥ�� ��ȯ (Ÿ�� ũ��: 50)
		// Ÿ�� ����: Ÿ�� �߽� = Ÿ�Ϲ�ȣ * 50 + 25
		int pixelX = packet->x * 50 + 25;
		int pixelY = packet->y * 50 + 25;
		GAME.GetAvatar()->AddObject(packet->object_id, std::string(packet->obj_name), 
			packet->visual_id, pixelX, pixelY, packet->hp, packet->max_hp, 
			packet->exp, packet->level);
		printf("Object added - ID: %d, Tile: (%d, %d) -> Pixel: (%d, %d)\n", 
			packet->object_id, packet->x, packet->y, pixelX, pixelY);
	}
	break;
	case S2C_REMOVE_OBJECT:
	{
		S2C_RemoveObject* packet = reinterpret_cast<S2C_RemoveObject*>(recv_packet);
		GAME.GetAvatar()->RemoveObject(packet->object_id);
	}
	break;
	case S2C_MOVE_OBJECT:
	{
		S2C_MoveObject* packet = reinterpret_cast<S2C_MoveObject*>(recv_packet);
		// coords are tile coordinates; convert to pixel center for rendering
		int pixelX = packet->x * 50 + 25;
		int pixelY = packet->y * 50 + 25;

		if (packet->object_id != GAME.GetAvatar()->GetPlayerID())
		{
			// Other objects: update interpolation target
			GAME.GetAvatar()->UpdateObjectPosition(packet->object_id, pixelX, pixelY);
		}
		// Own player: local client is authoritative — ignore server echo
	}
	break;
	case S2C_CHAT_MESSAGE:
	{
		S2C_ChatMessage* packet = reinterpret_cast<S2C_ChatMessage*>(recv_packet);

		std::string senderName;
		int myId = GAME.GetAvatar()->GetPlayerID();
		if (packet->object_id == myId)
		{
			senderName = "Me";
		}
		else
		{
			senderName = GAME.GetAvatar()->GetObjectName(packet->object_id);
			if (senderName.empty())
				senderName = "Player" + std::to_string(packet->object_id);
		}

		GAME.GetChatSystem()->AddMessage(senderName, std::string(packet->message));
	}
	break;
	case S2C_STATUS_CHANGE:
	{
		S2C_StatusChange* packet = reinterpret_cast<S2C_StatusChange*>(recv_packet);
		GAME.GetAvatar()->UpdateObjectStatus(packet->object_id, packet->hp, packet->max_hp,
			packet->exp, packet->level);
	}
	break;
	case S2C_STAT_INFO:
	{
		S2C_StatInfo* packet = reinterpret_cast<S2C_StatInfo*>(recv_packet);
		if (packet->object_id == GAME.GetAvatar()->GetPlayerID())
		{
			GAME.GetAvatar()->SetStatInfo(
				packet->str, packet->intl, packet->dex, packet->luk, packet->stat_points);
		}
	}
	break;

	case S2C_PARTY_UPDATE:
	{
		S2C_PartyUpdate* pkt = reinterpret_cast<S2C_PartyUpdate*>(recv_packet);
		GAME.GetAvatar()->OnPartyUpdate(pkt->party_id, (int)pkt->member_count, pkt->members);
	}
	break;
	case S2C_PARTY_LIST:
	{
		S2C_PartyList* pkt = reinterpret_cast<S2C_PartyList*>(recv_packet);
		GAME.GetAvatar()->OnPartyList((int)pkt->party_count, pkt->entries);
	}
	break;
	case S2C_DAMAGE_NUMBER:
	{
		S2C_DamageNumber* pkt = reinterpret_cast<S2C_DamageNumber*>(recv_packet);
		GAME.GetAvatar()->AddDamageNumber(pkt->attacker_id, pkt->object_id, pkt->damage, pkt->is_crit != 0);
	}
	break;
	case S2C_GOLD_UPDATE:
	{
		S2C_GoldUpdate* pkt = reinterpret_cast<S2C_GoldUpdate*>(recv_packet);
		GAME.GetAvatar()->SetGold(pkt->gold);
	}
	break;
	case S2C_BUY_RESULT:
	{
		S2C_BuyResult* pkt = reinterpret_cast<S2C_BuyResult*>(recv_packet);
		GAME.GetAvatar()->OnBuyResult(pkt->success, pkt->item_type, pkt->gold,
			pkt->new_hp, pkt->new_x, pkt->new_y);
	}
	break;
	default:
		// Unknown Packet Type
		break;
	}
}

void SendBuyItemToServer(ITEM_TYPE t)
{
	C2S_BuyItem pkt;
	pkt.size      = sizeof(C2S_BuyItem);
	pkt.type      = C2S_BUY_ITEM;
	pkt.item_type = t;
	GAME.GetNetwork()->Send(reinterpret_cast<void*>(&pkt));
}

void Network::ReceiveAndProcessPackets()
{
	int receivedBytes = Receive(mBuf, MAX_BUF_SIZE);

	// Non-blocking ��忡�� WSAEWOULDBLOCK�� ���� (������ ����)
	if (receivedBytes == -1)
	{
		int error = WSAGetLastError();
		if (error == WSAEWOULDBLOCK)
		{
			// ���� �����Ͱ� ���� - ���� ����
			return;
		}
		else
		{
			printf("WSARecv error: %d\n", error);
			return;
		}
	}

	if (receivedBytes > 0)
	{
		// ���� �����͸� ��Ŷ ������ ó��
		// ù ��° ����Ʈ�� ��Ŷ ũ��
		int offset = 0;
		while (offset < receivedBytes)
		{
			if (offset + 1 > receivedBytes)
				break;

			int packetSize = static_cast<unsigned char>(mBuf[offset]);

			if (packetSize <= 0 || offset + packetSize > receivedBytes)
				break;

			ProcessPacket(&mBuf[offset]);
			offset += packetSize;
		}
	}
}