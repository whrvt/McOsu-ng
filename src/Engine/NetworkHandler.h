//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		handles network connections, multiplayer, chat etc.
//
// $NoKeywords: $nw
//===============================================================================//

#pragma once
#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H

#include "cbase.h"

#ifdef MCENGINE_FEATURE_NETWORKING

#include <enet/enet.h>

#endif

class NetworkHandler
{
public:
	struct EXTENSION_PACKET
	{
		uint32_t size;
		std::shared_ptr<void> data;
	};

public:
	NetworkHandler();
	~NetworkHandler();

	// curl stuff
	UString httpGet(const UString& url, long timeout = 5, long connectTimeout = 5);
	std::string httpDownload(const UString& url, long timeout = 60, long connectTimeout = 5);

	// client/server stuff
	using NetworkLocalServerStartedListener = SA::delegate<void()>;					// called when we successfully start the server
	using NetworkLocalServerStoppedListener = SA::delegate<void()>;					// called when the host stops

	using NetworkClientDisconnectedFromServerListener = SA::delegate<void()>;			// called when the connection is stopped, (e.g. if the host is stopped, or you disconnect)
	using NetworkClientConnectedToServerListener = SA::delegate<void()>;				// called when a connection has been established (if the client has successfully connected)

	using NetworkClientSendInfoListener = SA::delegate<EXTENSION_PACKET()>;		// allows clients to send custom information to the server, independent of the NetworkHandler
	using NetworkServerSendInfoListener = SA::delegate<EXTENSION_PACKET()>;		// allows the server to send custom information to the client, independent of the NetworkHandler

	// returning false on either one of these will terminate the connection:
	using NetworkClientReceiveServerInfoListener = SA::delegate<void *(bool)>;	// called with the data of the EXTENSION_PACKET (if used) upon receiving the server info packet
	using NetworkServerReceiveClientInfoListener = SA::delegate<void *(bool)>;	// called with the data of the EXTENSION_PACKET (if used) upon receiving a client info packet

	using NetworkServerReceiveClientPacketListener = SA::delegate<bool(uint32_t, void *, uint32_t)>;	// packets sent through this class' functions will be received here
	using NetworkClientReceiveServerPacketListener = SA::delegate<bool(uint32_t, void *, uint32_t)>;	// packets sent through this class' functions will be received here
	using NetworkServerClientChangeListener = SA::delegate<void(uint32_t, UString, bool)>;					// called when a client's connection state (or another state of it, like its name or something) changes

	void update();

	void host();
	void hostStop();
	void status();

	void connect(const UString& address);
	void disconnect();

	void say(const UString& message);
	void kick(const UString& username);

	// main send functions
	void broadcast(void *data, uint32_t size, bool reliable = false);				// two functions depending on the callee: [server -> all clients] or [client -> server -> all clients]; if both server and client, server wins (first case)
	void servercast(void *data, uint32_t size, bool reliable = false);				// [client -> server]; only valid as client
	void clientcast(void *data, uint32_t size, uint32_t id, bool reliable = false);	// [server -> client]; only valid as server

	// callbacks
	void setOnClientReceiveServerPacketListener(const NetworkClientReceiveServerPacketListener& listener) {m_clientReceiveServerPacketListener = listener;}
	void setOnServerReceiveClientPacketListener(const NetworkServerReceiveClientPacketListener& listener) {m_serverReceiveClientPacketListener = listener;}
	void setOnServerClientChangeListener(const NetworkServerClientChangeListener& listener) {m_serverClientChangeListener = listener;}

	void setOnClientSendInfoListener(const NetworkClientSendInfoListener& listener) {m_clientSendInfoListener = listener;}
	void setOnServerSendInfoListener(const NetworkServerSendInfoListener& listener) {m_serverSendInfoListener = listener;}

	void setOnClientReceiveServerInfoListener(const NetworkClientReceiveServerInfoListener& listener) {m_clientReceiveServerInfoListener = listener;}
	void setOnServerReceiveClientInfoListener(const NetworkServerReceiveClientInfoListener& listener) {m_serverReceiveClientInfoListener = listener;}

	void setOnClientConnectedToServerListener(const NetworkClientConnectedToServerListener& listener) {m_clientConnectedToServerListener = listener;}
	void setOnClientDisconnectedFromServerListener(const NetworkClientDisconnectedFromServerListener& listener) {m_clientDisconnectedFromServerListener = listener;}

	void setOnLocalServerStartedListener(const NetworkLocalServerStartedListener& listener) {m_localServerStartedListener = listener;}
	void setOnLocalServerStoppedListener(const NetworkLocalServerStoppedListener& listener) {m_localServerStoppedListener = listener;}

	// getters
	[[nodiscard]] int getPing() const;

	[[nodiscard]] inline uint32_t getLocalClientID() const {return m_iLocalClientID;}
	[[nodiscard]] inline UString getServerAddress() const {return m_sServerAddress;}

	[[nodiscard]] bool isClient() const;
	[[nodiscard]] bool isServer() const;

private:

#ifdef MCENGINE_FEATURE_NETWORKING

	static size_t curlStringWriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
	static size_t curlStringStreamWriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

	void onClientEvent(ENetEvent e);
	void onServerEvent(ENetEvent e);

#endif

	//********************//
	//	  Packet Types	  //
	//********************//

	enum PACKET_TYPE : uint8_t
	{
		CLIENT_INFO_PACKET_TYPE,
		SERVER_INFO_PACKET_TYPE,
		CHAT_PACKET_TYPE,
		CLIENT_PACKET_TYPE,				// client -> server
		CLIENT_BROADCAST_PACKET_TYPE,	// client -> server -> all other clients (or, if we are the server: server -> all other clients)
		//CLIENT_SINGLECAST_PACKET_TYPE	// client -> server -> client
		CLIENT_CONNECTION_PACKET_TYPE
	};

	struct CHAT_PACKET;
	struct CLIENT_INFO_PACKET;
	struct CLIENT_PEER;

	void sendClientInfo();				// client -> server (on a successful connection)
	void clientDisconnect();

#ifdef MCENGINE_FEATURE_NETWORKING

	void sendServerInfo(uint32_t assignedID, ENetHost *host, ENetPeer *destination);	// server -> client (on a successful connection)

	void singlecastChatMessage(CHAT_PACKET *cp, ENetHost *host, ENetPeer *destination);
	void singlecastChatMessage(UString username, UString message, ENetHost *host, ENetPeer *destination);
	void broadcastChatMessage(CHAT_PACKET *cp, ENetHost *host, ENetPeer *origin);
	void broadcastChatMessage(UString username, UString message, ENetHost *host, ENetPeer *origin);

#endif

	void chatLog(UString username, const UString& message);

#ifdef MCENGINE_FEATURE_NETWORKING

	CLIENT_PEER *getClientPeerByPeer(ENetPeer *peer);
	CLIENT_PEER *getClientPeerById(uint32_t id);

#endif

	bool m_bReady;
	float m_fDebugNetworkTime;
	std::string curlReadBuffer;

	// client
	uint32_t m_iLocalClientID;
	UString m_sServerAddress;

#ifdef MCENGINE_FEATURE_NETWORKING

	ENetHost *m_client;
	ENetPeer *m_clientPeer;

#endif

	bool m_bClientConnectPending;
	float m_fClientConnectPendingTime;
	bool m_bClientConnectPendingAfterDisconnect;

	// server

#ifdef MCENGINE_FEATURE_NETWORKING

	ENetHost *m_server;
	std::vector<CLIENT_PEER> m_vConnectedClients;

#endif

	bool m_bClientDisconnectPending;
	float m_fClientDisconnectPendingTime;

	// events
	NetworkLocalServerStartedListener m_localServerStartedListener;
	NetworkLocalServerStoppedListener m_localServerStoppedListener;

	NetworkClientConnectedToServerListener m_clientConnectedToServerListener;
	NetworkClientDisconnectedFromServerListener m_clientDisconnectedFromServerListener;

	NetworkClientSendInfoListener m_clientSendInfoListener;
	NetworkServerSendInfoListener m_serverSendInfoListener;

	NetworkClientReceiveServerInfoListener m_clientReceiveServerInfoListener;
	NetworkServerReceiveClientInfoListener m_serverReceiveClientInfoListener;

	NetworkClientReceiveServerPacketListener m_clientReceiveServerPacketListener;
	NetworkServerReceiveClientPacketListener m_serverReceiveClientPacketListener;
	NetworkServerClientChangeListener m_serverClientChangeListener;



	// information saved for every peer

#ifdef MCENGINE_FEATURE_NETWORKING

	struct CLIENT_PEER
	{
		ENetPeer *peer;
		UString name;
		float kickTime; // if the client has been kicked, this serves as a delay to allow the kick chat message to arrive properly
		float kickKillTime; // if the client doesn't want to freely disconnect after the kick delay, kill him
		uint32_t id;
	};

#endif

	unsigned int m_iIDCounter;

	//***************//
	//	  Packets	 //
	//***************//

#pragma pack(1)

	struct CLIENT_BROADCAST_WRAPPER
	{
		PACKET_TYPE ntype = CLIENT_BROADCAST_PACKET_TYPE;
		uint32_t id; // unused; more values can be added here if needed
	};

	struct SERVER_INFO_PACKET
	{
		PACKET_TYPE type = SERVER_INFO_PACKET_TYPE;
		uint32_t id;
		bool extension; // if there is a custom packet of the current app, which may be appended to this one by the onServerSendInfo() API
	};

	struct CLIENT_INFO_PACKET
	{
		PACKET_TYPE type = CLIENT_INFO_PACKET_TYPE;
		uint32_t version;
		uint32_t size;
		wchar_t username[255];
		bool extension; // if there is a custom packet of the current app, which may be appended to this one by the onClientSendInfo() API
	};

	struct CHAT_PACKET
	{
		PACKET_TYPE type = CHAT_PACKET_TYPE;
		uint32_t usize;
		uint32_t msize;
		wchar_t username[255];
		wchar_t message[255];
	};

#pragma pack()
};

#endif
