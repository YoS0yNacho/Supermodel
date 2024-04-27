#ifndef INCLUDED_NET_CONSTANTS_H
#define INCLUDED_NET_CONSTANTS_H

#define MAX_PLAYERS 8
typedef enum
{
	// Server -> Client, You have been accepted. Contains the id for the client player to use
	AcceptPlayer = 1,

	// Server -> Client, Add a new player to your simulation, contains the ID of the player and a position
	AddPlayer = 2,

	// Server -> Client, Remove a player from your simulation, contains the ID of the player to remove
	RemovePlayer = 3,

	// Server -> Client, Update a player's position in the simulation, contains the ID of the player and a position
	UpdatePlayer = 4,

	// Client -> Server, Provide an updated location for the client's player, contains the postion to update
	UpdateInput = 5,
}NetworkCommands;


#endif	// INCLUDED_NET_CONSTANTS_H