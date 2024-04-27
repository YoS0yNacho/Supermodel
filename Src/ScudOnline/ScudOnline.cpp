

//model3.h has handlers for ram r/w
//romset.h for patching rom 


#define ENET_IMPLEMENTATION
#include "Network/enet.c"
#include <iostream>
#include <stdio.h>
#include "Model3/Model3.h"
#include "addresses.h"
#include "CPU/Bus.h"
#include "net_constants.h"
#include "OSD/Video.h"


//client crap

//bool connected = false;

// Data about players
typedef struct
{
	// true if the player is active and valid
	bool Active;

    // Player info
	uint32_t base;
	uint8_t carType;
	uint8_t carNumber;
	// the last known location of the player on the field
	float x;
	float y;
    float z;

	float pitch;
	float yaw;

	// the direction they were going
	//Vector3 Direction;

	// the time we got the last update
	double UpdateTime;

	//extrapolated pos
    float px;
	float py;
	float pz;

}RemotePlayer;

bool updatetick = false;
int tickCount; 
bool usedPBase[MAX_PLAYERS] = { false };
const uint32_t pBase[8] = { 0x181200, 0x181500, 0x181800, 0x181B00, 0x181E00, 0x182100, 0x182400, 0x182700 };
const uint8_t CarValues[8] = {0x9, 0xB, 0x8, 0xA, 0xD, 0xF, 0xC, 0xE };


// The list of all possible players
// this is the local simulation that represents the current game state
// it includes the current local player and the last known data from all remote players
// the client checks this every frame to see where everyone is on the field
RemotePlayer Players[MAX_PLAYERS] = { 0 };
int LocalPlayerId = -1;
// the enet address we are connected to
ENetAddress address = { 0 };

// the server object we are connecting to
ENetPeer* server = { 0 };

// the client peer we are using
ENetHost* client = { 0 };


// Model 3 context provides read/write handlers

static class IBus	*Bus = NULL;	// pointer to Model 3 bus object (for access handlers)

void gamemod_attachbus(IBus *BusPtr)
{
	Bus = BusPtr;
}

void SCUD_GamePatches()
{
    Bus->Write32(SelCourseFixAddr, 0x60000000);
	Bus->Write32(DisableRetireAddr, 0x60000000);
	Bus->Write32(DisableRetireAddr2, 0x60000000);
	Bus->Write32(DisableRetireAddr3, 0x60000000);
	Bus->Write32(DisableRetireAddr4, 0x60000000);
}

void SCUD_SecretCars(bool val)
{
    if(!val)
    {
        Bus->Write32(SecretCarsAddr, 0x4082001C);
    } else 
    {
        Bus->Write32(SecretCarsAddr, 0x60000000);
    }
}

// network crap

uint8_t ReadByte(ENetPacket* packet, size_t* offset)
{
	// make sure we have not gone past the end of the data we were sent
	if (*offset > packet->dataLength)
		return 0;

	// cast the data to a byte so we can increment it in 1 byte chunks
	uint8_t* ptr = (uint8_t*)packet->data;

	// get the byte at the current offset
	uint8_t data = ptr[(*offset)];

	// move the offset over 1 byte for the next read
	*offset = *offset + 1;

	return data;
}

float ReadFloat(ENetPacket* packet, size_t* offset)
{
	// make sure we have not gone past the end of the data we were sent
	if (*offset > packet->dataLength)
		return 0;

	// cast the data to a byte at the offset
	uint8_t* data = (uint8_t*)packet->data;
	data += (*offset);

	// move the offset over 2 bytes for the next read
	*offset = (*offset) + 4;

	// cast the data pointer to a short and return a copy
	return *(float*)data;
}

int GetLocalPlayerId()
{
	return LocalPlayerId;
}

int getNumberOfActivePlayers() 
{ 
	int activePlayers = 0;

	for (int i = 0; i < 8; i++)
	{
		if (Players[i].Active)
		{
			activePlayers++;
		}
	}

	return activePlayers;
}

void UpdateLocalPlayer()
{
	Players[GetLocalPlayerId()].x = Bus -> Read32(pBase[0]+bXPos);
	Players[GetLocalPlayerId()].y = Bus -> Read32(pBase[0]+bYPos);
	Players[GetLocalPlayerId()].z = Bus -> Read32(pBase[0]+bZPos);
	Players[GetLocalPlayerId()].pitch = Bus -> Read32(pBase[0]+bPitch);
	Players[GetLocalPlayerId()].yaw = Bus -> Read32(pBase[0]+bYaw);

    for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (i == LocalPlayerId || !Players[i].Active)
		continue;

		Bus->Write32(Players[i].base + bXPos, Players[i].x);
		Bus->Write32(Players[i].base + bYPos, Players[i].y);
		Bus->Write32(Players[i].base + bZPos, Players[i].z);
		Bus->Write32(Players[i].base + bPitch, Players[i].pitch);
		Bus->Write32(Players[i].base + bYaw, Players[i].yaw);
		Bus->Write32(Players[i].base + bDisableAI, 0xFFFFFFFF); //Disable AI
	}
	
}

// A new remote player was added to our local simulation
void HandleAddPlayer(ENetPacket* packet, size_t* offset)
{
	// find out who the server is talking about
	int remotePlayer = ReadByte(packet, offset);
	if (remotePlayer >= MAX_PLAYERS || remotePlayer == LocalPlayerId)
		return;

	// set them as active and update the location
	Players[remotePlayer].Active = true;
	// check for available pBase
	for (int i = 1; i < MAX_PLAYERS; i++)
    {
        if (!usedPBase[i])
        {
            Players[remotePlayer].base = pBase[i];
            usedPBase[i] = true;
            break;
        }
	}
	Players[remotePlayer].x = ReadFloat(packet, offset);
	Players[remotePlayer].y = ReadFloat(packet, offset);
	Players[remotePlayer].z = ReadFloat(packet, offset);
    Players[remotePlayer].pitch = ReadFloat(packet, offset);
	Players[remotePlayer].yaw = ReadFloat(packet, offset);

	// In a more robust game, this message would have more info about the new player, such as what sprite or model to use, player name, or other data a client would need
	// this is where static data about the player would be sent, and any initial state needed to setup the local simulation
}

// A remote player has left the game and needs to be removed from the local simulation
void HandleRemovePlayer(ENetPacket* packet, size_t* offset)
{
	// find out who the server is talking about
	int remotePlayer = ReadByte(packet, offset);
	if (remotePlayer >= MAX_PLAYERS || remotePlayer == LocalPlayerId)
		return;

	// remove the player from the simulation. No other data is needed except the player id
	Players[remotePlayer].Active = false;
	// Free up the pBase that was assigned to the removed player
    for (int i = 1; i < MAX_PLAYERS; i++) 
	{ 
		if (Players[remotePlayer].base == pBase[i]) 
		{
			Players[remotePlayer].base = 0;
			usedPBase[i] = false; 
			break; 
		} 
	}
}

// The server has a new position for a player in our local simulation
void HandleUpdatePlayer(ENetPacket* packet, size_t* offset)
{
	// find out who the server is talking about
    while (*offset < packet->dataLength)
    {
		// find out who the server is talking about
		int remotePlayer = ReadByte(packet, offset);
		if (remotePlayer >= MAX_PLAYERS || remotePlayer == LocalPlayerId || !Players[remotePlayer].Active)
		   return;
		
		// update the last known position and movement
		Players[remotePlayer].x = ReadFloat(packet, offset);
		Players[remotePlayer].y = ReadFloat(packet, offset);
		Players[remotePlayer].z = ReadFloat(packet, offset);
		Players[remotePlayer].pitch = ReadFloat(packet, offset);
		Players[remotePlayer].yaw = ReadFloat(packet, offset);
	}
}

void Update()
{
	// Read localPlayer values from M3 ram
    UpdateLocalPlayer();

	if (BeginFrameVideo())
	{
		tickCount++;
	}
	
	if (tickCount == 3) 
	{
		tickCount = 0;
		// Pack up a buffer with the data we want to send
		uint8_t buffer[30] = { 0 }; // 9 bytes for a 1 byte command number and two bytes for each X and Y value
		buffer[0] = (uint8_t)UpdateInput;   // this tells the server what kind of data to expect in this packet
		*(float*)(buffer + 1) = (float)Players[LocalPlayerId].x;
		*(float*)(buffer + 5) = (float)Players[LocalPlayerId].y;
		*(float*)(buffer + 9) = (float)Players[LocalPlayerId].z;
		*(float*)(buffer + 13) = (float)Players[LocalPlayerId].pitch;
		*(float*)(buffer + 17) = (float)Players[LocalPlayerId].yaw;
		

		// copy this data into a packet provided by enet (TODO : add pack functions that write directly to the packet to avoid the copy)
		ENetPacket* packet = enet_packet_create(buffer, 30, ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);

		// send the packet to the server
		enet_peer_send(server, 0, packet);

		// NOTE enet_host_service will handle releasing send packets when the network system has finally sent them,
		// you don't have to destroy them

	}
	// read one event from enet and process it
	ENetEvent Event;

	// Check to see if we even have any events to do. Since this is a a client, we don't set a timeout so that the client can keep going if there are no events
	if (enet_host_service(client, &Event, 0) > 0)
	{
		// see what kind of event it is
		switch (Event.type)
		{
			// the server sent us some data, we should process it
			case ENET_EVENT_TYPE_RECEIVE:
			{
				// we know that all valid packets have a size >= 1, so if we get this, something is bad and we ignore it.
				if (Event.packet->dataLength < 1)
					break;

				// keep an offset of what data we have read so far
				size_t offset = 0;

				// read off the command that the server wants us to do
				NetworkCommands command = (NetworkCommands)ReadByte(Event.packet, &offset);

				// if the server has not accepted us yet, we are limited in what packets we can receive
				if (LocalPlayerId == -1)
				{
					if (command == AcceptPlayer)    // this is the only thing we can do in this state, so ignore anything else
					{
						// See who the server says we are
						LocalPlayerId = ReadByte(Event.packet, &offset);
						usedPBase[0] = true;

						// Make sure that it makes sense
						if (LocalPlayerId < 0)
						{
							usedPBase[0] = false;
							LocalPlayerId = -1;
							break;
						}

					}
				}
				else // we have been accepted, so process play messages from the server
				{
					// see what the server wants us to do
					switch (command)
					{
						case AddPlayer:
							HandleAddPlayer(Event.packet, &offset);
							break;

						case RemovePlayer:
							HandleRemovePlayer(Event.packet, &offset);
							break;

						case UpdatePlayer:
							HandleUpdatePlayer(Event.packet, &offset);
							break;
					}
				}

				// tell enet that it can recycle the packet data
				enet_packet_destroy(Event.packet);
				break;
			}

			// we were disconnected, we have a sad
			case ENET_EVENT_TYPE_DISCONNECT:
				server = NULL;
				break;
		}
	}
}

void Connect(const char* serverAddress, enet_uint16 port)
{
	// startup the network library
	enet_initialize();

	// create a client that we will use to connect to the server
	client = enet_host_create(NULL, 1, 1, 0, 0);

	// set the address and port we will connect to
	enet_address_set_host(&address, serverAddress);
	address.port = 4545;

	// start the connection process. Will be finished as part of our update
	server = enet_host_connect(client, &address, 1, 0);
}

bool Connected()
{
	return server != NULL;
}

void runClient()
{
   if (!Connected())
   {

	Connect("127.0.0.1", 4545);

   }
   else 
   {

	Update();

   }
   
}

// gamemod
void gamemod_run()
{
    // add detection to which game is running e.g. scudplus, dayto2pe, daytona2

    // run game patches
    SCUD_GamePatches();
    SCUD_SecretCars(true);
	//only show real player
	//if (getNumberOfActivePlayers() > 1)
	//{
	//	int val = getNumberOfActivePlayers();
	//	Bus->Write8(gCarCount^3, val - 1);
	//}

    //Bus->Write8(gCPUCounter^3, getNumberOfActivePlayers());
	//if ( (Bus->Read8(gMainState^3) == 0x11) && (Bus->Read32(gSubTimer^3) < 0x381)  )
	//{
	//	Bus->Write8(gLink^3, 1);
	//}

    // run 
    runClient();

}





