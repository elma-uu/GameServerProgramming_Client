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
	case S2C_RESPAWN:
	{
		S2C_Respawn* pkt = reinterpret_cast<S2C_Respawn*>(recv_packet);
		GAME.GetAvatar()->Respawn(pkt->hp, pkt->max_hp, pkt->x, pkt->y);
	}
	break;
	case S2C_QUEST_UPDATE:
	{
		S2C_QuestUpdate* pkt = reinterpret_cast<S2C_QuestUpdate*>(recv_packet);
		GAME.GetAvatar()->OnQuestUpdate(pkt->quest_id, pkt->quest_state,
		                                pkt->progress, pkt->goal);
	}
	break;
	case S2C_DUNGEON_ENTER:
	{
		S2C_DungeonEnter* pkt = reinterpret_cast<S2C_DungeonEnter*>(recv_packet);
		GAME.GetAvatar()->DungeonEnter(pkt->entered, pkt->instance_id, pkt->x, pkt->y);
	}
	break;
	case S2C_USE_ITEM_RESULT:
	{
		S2C_UseItemResult* pkt = reinterpret_cast<S2C_UseItemResult*>(recv_packet);
		GAME.GetAvatar()->OnUseItemResult(pkt->success, pkt->item_type, pkt->item_count,
		                                  pkt->new_hp, pkt->new_x, pkt->new_y);
	}
	break;
	case S2C_HAND_MOVE_TO:
	{
		S2C_HandMoveTo* pkt = reinterpret_cast<S2C_HandMoveTo*>(recv_packet);
		GAME.GetAvatar()->OnHandMoveTo(pkt->object_id, pkt->target_x, pkt->target_y, pkt->move_ms);
	}
	break;
	case S2C_LASER_FIRE:
	{
		S2C_LaserFire* pkt = reinterpret_cast<S2C_LaserFire*>(recv_packet);
		GAME.GetAvatar()->OnLaserFire(pkt->object_id, pkt->center_y, pkt->duration_ms);
	}
	break;
	case S2C_HAND_ANIM_STATE:
	{
		S2C_HandAnimState* pkt = reinterpret_cast<S2C_HandAnimState*>(recv_packet);
		GAME.GetAvatar()->OnHandAnimState(pkt->object_id, pkt->anim_state);
	}
	break;
	case S2C_SWORD_FALL:
	{
		S2C_SwordFall* pkt = reinterpret_cast<S2C_SwordFall*>(recv_packet);
		GAME.GetAvatar()->OnSwordFall(pkt->fall_duration_ms);
	}
	break;
	case S2C_SWORD_FALL_H:
	{
		S2C_SwordFallH* pkt = reinterpret_cast<S2C_SwordFallH*>(recv_packet);
		GAME.GetAvatar()->OnSwordFallH(pkt->fall_duration_ms);
	}
	break;
	case S2C_PLAYER_DIE:
	{
		S2C_PlayerDie* pkt = reinterpret_cast<S2C_PlayerDie*>(recv_packet);
		GAME.GetAvatar()->OnPlayerDie(pkt->object_id);
	}
	break;
	case S2C_ENHANCE_RESULT:
	{
		S2C_EnhanceResult* pkt = reinterpret_cast<S2C_EnhanceResult*>(recv_packet);
		GAME.GetAvatar()->OnEnhanceResult(pkt->result, pkt->new_level, pkt->gold);
	}
	break;
	default:
		// Unknown Packet Type
		break;
	}
}

void SendQuestInteractToServer()
{
	C2S_QuestInteract pkt;
	pkt.size = sizeof(C2S_QuestInteract);
	pkt.type = C2S_QUEST_INTERACT;
	GAME.GetNetwork()->Send(reinterpret_cast<void*>(&pkt));
}

void SendEnhanceWeaponToServer()
{
	C2S_EnhanceWeapon pkt;
	pkt.size = sizeof(C2S_EnhanceWeapon);
	pkt.type = C2S_ENHANCE_WEAPON;
	GAME.GetNetwork()->Send(reinterpret_cast<void*>(&pkt));
}

void SendDungeonEnterToServer()
{
	C2S_DungeonEnter pkt;
	pkt.size = sizeof(C2S_DungeonEnter);
	pkt.type = C2S_DUNGEON_ENTER;
	GAME.GetNetwork()->Send(reinterpret_cast<void*>(&pkt));
}

void SendUseItemToServer(ITEM_TYPE t)
{
	C2S_UseItem pkt;
	pkt.size      = sizeof(C2S_UseItem);
	pkt.type      = C2S_USE_ITEM;
	pkt.item_type = t;
	GAME.GetNetwork()->Send(reinterpret_cast<void*>(&pkt));
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