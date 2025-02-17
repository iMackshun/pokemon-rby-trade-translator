/*DESCRIPTION: A program that communicates with a Gameboy running an English Gen 1 Pokemon Game and another Gameboy running a Japanese Gen 1 Pokemon game, both being connected to the Arduino Pins via the link cable.*/
//Arduino(Master)
//Gameboy A (Slave)(English)
//Gameboy B (Slave)(Japanese)
/*C INCLUDES*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*PIN DEFINITIONS*/
#define DMG_GAMEBOY_A_LINK_CLK 2
#define DMG_GAMEBOY_A_LINK_SIN 3
#define DMG_GAMEBOY_A_LINK_SOUT 4
#define DMG_GAMEBOY_B_LINK_CLK 5
#define DMG_GAMEBOY_B_LINK_SIN 6
#define DMG_GAMEBOY_B_LINK_SOUT 7
//#define START_TRANSMISSION_BUTTON_PIN 8

/*BUFFER DEFINITIONS*/

/*OTHER DEFINITIONS*/
#define STATE_COUNT 26
#define TRAINER_ID_COUNT 4

/*STRUCTS*/
struct LinkState{
  //Controls the condition needed for the state to proceed.
  //0 = Waiting on Byte, 1 = Waiting For Data Transmission + Data Sync, 2 = Data Translation, 3 = Jump To State, 4 = Delay Transmission
  unsigned char stateType = 0;
  //The byte that the the state is waiting on.
  unsigned char targetByte = 0;
  //Whether the target byte consists of a range.
  bool targetByteUsesRange = false;
  //The inclusive end of the range of the target byte check.
  unsigned char targetByteEnd = 0;
  //Whether the condition needs to be inverted.
  bool inverseCondition = false;
  //The occurrences of the byte needed to satify the state.
  unsigned char targetByteOccurrenceCount = 1; 
  //The counter used to keep track of the amount of time the target byte was read while this state is active. One for each gameboy.
  unsigned char targetByteOccurrenceCounterA = 1;
  unsigned char targetByteOccurrenceCounterB = 1;
  //Whether the occurrence counter resets if the condition isn't satisfied.
  bool resetOccurrenceCounterUponConflictingByte = false;
  //Whether the state responds with a byte whenever it's condition gets satisfied.
  bool respondsWithByte = false;
  //Whether the state responds with a byte upon transitioning to a different state.
  bool respondsWithByteUponTransition = false;
  //The byte that will be sent to the gameboys when the state's condition gets satisfied, if applicable.
  unsigned char responseByte = 0xFE;
  //The amount of response bytes that will be sent to the gameboy.
  unsigned char responseByteRepeatCount = 0;
  //Whether the state has a byte is sends by default.
  bool sendsByteByDefault = true;
  //The byte to send by default whenever this state isn't satisfied.
  unsigned char defaultResponseByte = 0xFE;
  //Whether the default response byte is the last byte that was exchanged.
  bool useLastExchangedBytesAsDefaultResponseByte = false;
  //Stores the index of the buffer to be used.
  //0 = Random Number Block, 1 = Player Party Block, 2 = Patch List Block
  signed char bufferIndex = -1;
  //Whether the state exchanges bytes. Used for states with the type of 1.
  bool exchangesBytes = false;
  //Stores the amount of bytes that is expected to be exchanged while the state is active.
  unsigned short bufferASize = 0;
  unsigned short bufferBSize = 0;
  //The state that will be looped back to once this state is made active.
  unsigned char jumpStateIndex = 0;
  //Whether the state waits for a byte before it can be transitioned out of.
  bool canOnlyProceedOnCertainByte = false;
  //The byte the state will wait for before the it can be transitioned out of.
  unsigned char requiredByteToProceed = 0;
  //The amount of time(in ms) transmission will be disabled after this state is transitioned from.
  unsigned long transmissionDelayTime = 0;
};

/*DATA*/
//1661 Byte Array of Data containing Encoded English Pokemon Names (11 Bytes Each)
const unsigned char englishPokemonNameData[] PROGMEM  = {
  0x81, 0x94, 0x8b, 0x81, 0x80, 0x92, 0x80, 0x94, 0x91, 0x50, 0x00, 0x88, 0x95, 0x98, 0x92, 0x80, 
  0x94, 0x91, 0x50, 0x00, 0x00, 0x00, 0x95, 0x84, 0x8d, 0x94, 0x92, 0x80, 0x94, 0x91, 0x50, 0x00, 
  0x00, 0x82, 0x87, 0x80, 0x91, 0x8c, 0x80, 0x8d, 0x83, 0x84, 0x91, 0x50, 0x82, 0x87, 0x80, 0x91, 
  0x8c, 0x84, 0x8b, 0x84, 0x8e, 0x8d, 0x50, 0x82, 0x87, 0x80, 0x91, 0x88, 0x99, 0x80, 0x91, 0x83, 
  0x50, 0x00, 0x92, 0x90, 0x94, 0x88, 0x91, 0x93, 0x8b, 0x84, 0x50, 0x00, 0x00, 0x96, 0x80, 0x91, 
  0x93, 0x8e, 0x91, 0x93, 0x8b, 0x84, 0x50, 0x00, 0x81, 0x8b, 0x80, 0x92, 0x93, 0x8e, 0x88, 0x92, 
  0x84, 0x50, 0x00, 0x82, 0x80, 0x93, 0x84, 0x91, 0x8f, 0x88, 0x84, 0x50, 0x00, 0x00, 0x8c, 0x84, 
  0x93, 0x80, 0x8f, 0x8e, 0x83, 0x50, 0x00, 0x00, 0x00, 0x81, 0x94, 0x93, 0x93, 0x84, 0x91, 0x85, 
  0x91, 0x84, 0x84, 0x50, 0x96, 0x84, 0x84, 0x83, 0x8b, 0x84, 0x50, 0x00, 0x00, 0x00, 0x00, 0x8a, 
  0x80, 0x8a, 0x94, 0x8d, 0x80, 0x50, 0x00, 0x00, 0x00, 0x00, 0x81, 0x84, 0x84, 0x83, 0x91, 0x88, 
  0x8b, 0x8b, 0x50, 0x00, 0x00, 0x8f, 0x88, 0x83, 0x86, 0x84, 0x98, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x8f, 0x88, 0x83, 0x86, 0x84, 0x8e, 0x93, 0x93, 0x8e, 0x50, 0x00, 0x8f, 0x88, 0x83, 0x86, 0x84, 
  0x8e, 0x93, 0x50, 0x00, 0x00, 0x00, 0x91, 0x80, 0x93, 0x93, 0x80, 0x93, 0x80, 0x50, 0x00, 0x00, 
  0x00, 0x91, 0x80, 0x93, 0x88, 0x82, 0x80, 0x93, 0x84, 0x50, 0x00, 0x00, 0x92, 0x8f, 0x84, 0x80, 
  0x91, 0x8e, 0x96, 0x50, 0x00, 0x00, 0x00, 0x85, 0x84, 0x80, 0x91, 0x8e, 0x96, 0x50, 0x00, 0x00, 
  0x00, 0x00, 0x84, 0x8a, 0x80, 0x8d, 0x92, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x91, 0x81, 
  0x8e, 0x8a, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x88, 0x8a, 0x80, 0x82, 0x87, 0x94, 0x50, 
  0x00, 0x00, 0x00, 0x91, 0x80, 0x88, 0x82, 0x87, 0x94, 0x50, 0x00, 0x00, 0x00, 0x00, 0x92, 0x80, 
  0x8d, 0x83, 0x92, 0x87, 0x91, 0x84, 0x96, 0x50, 0x00, 0x92, 0x80, 0x8d, 0x83, 0x92, 0x8b, 0x80, 
  0x92, 0x87, 0x50, 0x00, 0x8d, 0x88, 0x83, 0x8e, 0x91, 0x80, 0x8d, 0x50, 0x00, 0x00, 0x00, 0x8d, 
  0x88, 0x83, 0x8e, 0x91, 0x88, 0x8d, 0x80, 0x50, 0x00, 0x00, 0x8d, 0x88, 0x83, 0x8e, 0x90, 0x94, 
  0x84, 0x84, 0x8d, 0x50, 0x00, 0x8d, 0x88, 0x83, 0x8e, 0x91, 0x80, 0x8d, 0x50, 0x00, 0x00, 0x00, 
  0x8d, 0x88, 0x83, 0x8e, 0x91, 0x88, 0x8d, 0x8e, 0x50, 0x00, 0x00, 0x8d, 0x88, 0x83, 0x8e, 0x8a, 
  0x88, 0x8d, 0x86, 0x50, 0x00, 0x00, 0x82, 0x8b, 0x84, 0x85, 0x80, 0x88, 0x91, 0x98, 0x50, 0x00, 
  0x00, 0x82, 0x8b, 0x84, 0x85, 0x80, 0x81, 0x8b, 0x84, 0x50, 0x00, 0x00, 0x95, 0x94, 0x8b, 0x8f, 
  0x88, 0x97, 0x50, 0x00, 0x00, 0x00, 0x00, 0x8d, 0x88, 0x8d, 0x84, 0x93, 0x80, 0x8b, 0x84, 0x92, 
  0x50, 0x00, 0x89, 0x88, 0x86, 0x86, 0x8b, 0x98, 0x8f, 0x94, 0x85, 0x85, 0x50, 0x96, 0x88, 0x86, 
  0x86, 0x8b, 0x98, 0x93, 0x94, 0x85, 0x85, 0x50, 0x99, 0x94, 0x81, 0x80, 0x93, 0x50, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x86, 0x8e, 0x8b, 0x81, 0x80, 0x93, 0x50, 0x00, 0x00, 0x00, 0x00, 0x8e, 0x83, 
  0x83, 0x88, 0x92, 0x87, 0x50, 0x00, 0x00, 0x00, 0x00, 0x86, 0x8b, 0x8e, 0x8e, 0x8c, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x95, 0x88, 0x8b, 0x84, 0x8f, 0x8b, 0x94, 0x8c, 0x84, 0x50, 0x00, 0x8f, 
  0x80, 0x91, 0x80, 0x92, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x80, 0x91, 0x80, 0x92, 0x84, 
  0x82, 0x93, 0x50, 0x00, 0x00, 0x95, 0x84, 0x8d, 0x8e, 0x8d, 0x80, 0x93, 0x50, 0x00, 0x00, 0x00, 
  0x95, 0x84, 0x8d, 0x8e, 0x8c, 0x8e, 0x93, 0x87, 0x50, 0x00, 0x00, 0x83, 0x88, 0x86, 0x8b, 0x84, 
  0x93, 0x93, 0x50, 0x00, 0x00, 0x00, 0x83, 0x94, 0x86, 0x93, 0x91, 0x88, 0x8e, 0x50, 0x00, 0x00, 
  0x00, 0x8c, 0x84, 0x8e, 0x96, 0x93, 0x87, 0x50, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x84, 0x91, 0x92, 
  0x88, 0x80, 0x8d, 0x50, 0x00, 0x00, 0x00, 0x8f, 0x92, 0x98, 0x83, 0x94, 0x82, 0x8a, 0x50, 0x00, 
  0x00, 0x00, 0x86, 0x8e, 0x8b, 0x83, 0x94, 0x82, 0x8a, 0x50, 0x00, 0x00, 0x00, 0x8c, 0x80, 0x8d, 
  0x8a, 0x84, 0x98, 0x50, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x91, 0x88, 0x8c, 0x84, 0x80, 0x8f, 0x84, 
  0x50, 0x00, 0x00, 0x86, 0x91, 0x8e, 0x96, 0x8b, 0x88, 0x93, 0x87, 0x84, 0x50, 0x00, 0x80, 0x91, 
  0x82, 0x80, 0x8d, 0x88, 0x8d, 0x84, 0x50, 0x00, 0x00, 0x8f, 0x8e, 0x8b, 0x88, 0x96, 0x80, 0x86, 
  0x50, 0x00, 0x00, 0x00, 0x8f, 0x8e, 0x8b, 0x88, 0x96, 0x87, 0x88, 0x91, 0x8b, 0x50, 0x00, 0x8f, 
  0x8e, 0x8b, 0x88, 0x96, 0x91, 0x80, 0x93, 0x87, 0x50, 0x00, 0x80, 0x81, 0x91, 0x80, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x80, 0x83, 0x80, 0x81, 0x91, 0x80, 0x50, 0x00, 0x00, 0x00, 
  0x80, 0x8b, 0x80, 0x8a, 0x80, 0x99, 0x80, 0x8c, 0x50, 0x00, 0x00, 0x8c, 0x80, 0x82, 0x87, 0x8e, 
  0x8f, 0x50, 0x00, 0x00, 0x00, 0x00, 0x8c, 0x80, 0x82, 0x87, 0x8e, 0x8a, 0x84, 0x50, 0x00, 0x00, 
  0x00, 0x8c, 0x80, 0x82, 0x87, 0x80, 0x8c, 0x8f, 0x50, 0x00, 0x00, 0x00, 0x81, 0x84, 0x8b, 0x8b, 
  0x92, 0x8f, 0x91, 0x8e, 0x94, 0x93, 0x50, 0x96, 0x84, 0x84, 0x8f, 0x88, 0x8d, 0x81, 0x84, 0x8b, 
  0x8b, 0x50, 0x95, 0x88, 0x82, 0x93, 0x91, 0x84, 0x84, 0x81, 0x84, 0x8b, 0x50, 0x93, 0x84, 0x8d, 
  0x93, 0x80, 0x82, 0x8e, 0x8e, 0x8b, 0x50, 0x00, 0x93, 0x84, 0x8d, 0x93, 0x80, 0x82, 0x91, 0x94, 
  0x84, 0x8b, 0x50, 0x86, 0x84, 0x8e, 0x83, 0x94, 0x83, 0x84, 0x50, 0x00, 0x00, 0x00, 0x86, 0x91, 
  0x80, 0x95, 0x84, 0x8b, 0x84, 0x91, 0x50, 0x00, 0x00, 0x86, 0x8e, 0x8b, 0x84, 0x8c, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x8f, 0x8e, 0x8d, 0x98, 0x93, 0x80, 0x50, 0x00, 0x00, 0x00, 0x00, 0x91, 
  0x80, 0x8f, 0x88, 0x83, 0x80, 0x92, 0x87, 0x50, 0x00, 0x00, 0x92, 0x8b, 0x8e, 0x96, 0x8f, 0x8e, 
  0x8a, 0x84, 0x50, 0x00, 0x00, 0x92, 0x8b, 0x8e, 0x96, 0x81, 0x91, 0x8e, 0x50, 0x00, 0x00, 0x00, 
  0x8c, 0x80, 0x86, 0x8d, 0x84, 0x8c, 0x88, 0x93, 0x84, 0x50, 0x00, 0x8c, 0x80, 0x86, 0x8d, 0x84, 
  0x93, 0x8e, 0x8d, 0x50, 0x00, 0x00, 0x85, 0x80, 0x91, 0x85, 0x84, 0x93, 0x82, 0x87, 0xe0, 0x83, 
  0x50, 0x83, 0x8e, 0x83, 0x94, 0x8e, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x8e, 0x83, 0x91, 
  0x88, 0x8e, 0x50, 0x00, 0x00, 0x00, 0x00, 0x92, 0x84, 0x84, 0x8b, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x83, 0x84, 0x96, 0x86, 0x8e, 0x8d, 0x86, 0x50, 0x00, 0x00, 0x00, 0x86, 0x91, 0x88, 
  0x8c, 0x84, 0x91, 0x50, 0x00, 0x00, 0x00, 0x00, 0x8c, 0x94, 0x8a, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x92, 0x87, 0x84, 0x8b, 0x8b, 0x83, 0x84, 0x91, 0x50, 0x00, 0x00, 0x82, 0x8b, 
  0x8e, 0x98, 0x92, 0x93, 0x84, 0x91, 0x50, 0x00, 0x00, 0x86, 0x80, 0x92, 0x93, 0x8b, 0x98, 0x50, 
  0x00, 0x00, 0x00, 0x00, 0x87, 0x80, 0x94, 0x8d, 0x93, 0x84, 0x91, 0x50, 0x00, 0x00, 0x00, 0x86, 
  0x84, 0x8d, 0x86, 0x80, 0x91, 0x50, 0x00, 0x00, 0x00, 0x00, 0x8e, 0x8d, 0x88, 0x97, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x91, 0x8e, 0x96, 0x99, 0x84, 0x84, 0x50, 0x00, 0x00, 0x00, 
  0x87, 0x98, 0x8f, 0x8d, 0x8e, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x91, 0x80, 0x81, 0x81, 
  0x98, 0x50, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x88, 0x8d, 0x86, 0x8b, 0x84, 0x91, 0x50, 0x00, 0x00, 
  0x00, 0x95, 0x8e, 0x8b, 0x93, 0x8e, 0x91, 0x81, 0x50, 0x00, 0x00, 0x00, 0x84, 0x8b, 0x84, 0x82, 
  0x93, 0x91, 0x8e, 0x83, 0x84, 0x50, 0x00, 0x84, 0x97, 0x84, 0x86, 0x86, 0x82, 0x94, 0x93, 0x84, 
  0x50, 0x00, 0x84, 0x97, 0x84, 0x86, 0x86, 0x94, 0x93, 0x8e, 0x91, 0x50, 0x00, 0x82, 0x94, 0x81, 
  0x8e, 0x8d, 0x84, 0x50, 0x00, 0x00, 0x00, 0x00, 0x8c, 0x80, 0x91, 0x8e, 0x96, 0x80, 0x8a, 0x50, 
  0x00, 0x00, 0x00, 0x87, 0x88, 0x93, 0x8c, 0x8e, 0x8d, 0x8b, 0x84, 0x84, 0x50, 0x00, 0x87, 0x88, 
  0x93, 0x8c, 0x8e, 0x8d, 0x82, 0x87, 0x80, 0x8d, 0x50, 0x8b, 0x88, 0x82, 0x8a, 0x88, 0x93, 0x94, 
  0x8d, 0x86, 0x50, 0x00, 0x8a, 0x8e, 0x85, 0x85, 0x88, 0x8d, 0x86, 0x50, 0x00, 0x00, 0x00, 0x96, 
  0x84, 0x84, 0x99, 0x88, 0x8d, 0x86, 0x50, 0x00, 0x00, 0x00, 0x91, 0x87, 0x98, 0x87, 0x8e, 0x91, 
  0x8d, 0x50, 0x00, 0x00, 0x00, 0x91, 0x87, 0x98, 0x83, 0x8e, 0x8d, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x82, 0x87, 0x80, 0x8d, 0x92, 0x84, 0x98, 0x50, 0x00, 0x00, 0x00, 0x93, 0x80, 0x8d, 0x86, 0x84, 
  0x8b, 0x80, 0x50, 0x00, 0x00, 0x00, 0x8a, 0x80, 0x8d, 0x86, 0x80, 0x92, 0x8a, 0x87, 0x80, 0x8d, 
  0x50, 0x87, 0x8e, 0x91, 0x92, 0x84, 0x80, 0x50, 0x00, 0x00, 0x00, 0x00, 0x92, 0x84, 0x80, 0x83, 
  0x91, 0x80, 0x50, 0x00, 0x00, 0x00, 0x00, 0x86, 0x8e, 0x8b, 0x83, 0x84, 0x84, 0x8d, 0x50, 0x00, 
  0x00, 0x00, 0x92, 0x84, 0x80, 0x8a, 0x88, 0x8d, 0x86, 0x50, 0x00, 0x00, 0x00, 0x92, 0x93, 0x80, 
  0x91, 0x98, 0x94, 0x50, 0x00, 0x00, 0x00, 0x00, 0x92, 0x93, 0x80, 0x91, 0x8c, 0x88, 0x84, 0x50, 
  0x00, 0x00, 0x00, 0x8c, 0x91, 0xe8, 0x8c, 0x88, 0x8c, 0x84, 0x50, 0x00, 0x00, 0x00, 0x92, 0x82, 
  0x98, 0x93, 0x87, 0x84, 0x91, 0x50, 0x00, 0x00, 0x00, 0x89, 0x98, 0x8d, 0x97, 0x50, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x84, 0x8b, 0x84, 0x82, 0x93, 0x80, 0x81, 0x94, 0x99, 0x99, 0x50, 0x8c, 
  0x80, 0x86, 0x8c, 0x80, 0x91, 0x50, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x88, 0x8d, 0x92, 0x88, 0x91, 
  0x50, 0x00, 0x00, 0x00, 0x00, 0x93, 0x80, 0x94, 0x91, 0x8e, 0x92, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x8c, 0x80, 0x86, 0x88, 0x8a, 0x80, 0x91, 0x8f, 0x50, 0x00, 0x00, 0x86, 0x98, 0x80, 0x91, 0x80, 
  0x83, 0x8e, 0x92, 0x50, 0x00, 0x00, 0x8b, 0x80, 0x8f, 0x91, 0x80, 0x92, 0x50, 0x00, 0x00, 0x00, 
  0x00, 0x83, 0x88, 0x93, 0x93, 0x8e, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x84, 0x95, 0x84, 
  0x84, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x95, 0x80, 0x8f, 0x8e, 0x91, 0x84, 0x8e, 0x8d, 0x50, 
  0x00, 0x00, 0x89, 0x8e, 0x8b, 0x93, 0x84, 0x8e, 0x8d, 0x50, 0x00, 0x00, 0x00, 0x85, 0x8b, 0x80, 
  0x91, 0x84, 0x8e, 0x8d, 0x50, 0x00, 0x00, 0x00, 0x8f, 0x8e, 0x91, 0x98, 0x86, 0x8e, 0x8d, 0x50, 
  0x00, 0x00, 0x00, 0x8e, 0x8c, 0x80, 0x8d, 0x98, 0x93, 0x84, 0x50, 0x00, 0x00, 0x00, 0x8e, 0x8c, 
  0x80, 0x92, 0x93, 0x80, 0x91, 0x50, 0x00, 0x00, 0x00, 0x8a, 0x80, 0x81, 0x94, 0x93, 0x8e, 0x50, 
  0x00, 0x00, 0x00, 0x00, 0x8a, 0x80, 0x81, 0x94, 0x93, 0x8e, 0x8f, 0x92, 0x50, 0x00, 0x00, 0x80, 
  0x84, 0x91, 0x8e, 0x83, 0x80, 0x82, 0x93, 0x98, 0x8b, 0x50, 0x92, 0x8d, 0x8e, 0x91, 0x8b, 0x80, 
  0x97, 0x50, 0x00, 0x00, 0x00, 0x80, 0x91, 0x93, 0x88, 0x82, 0x94, 0x8d, 0x8e, 0x50, 0x00, 0x00, 
  0x99, 0x80, 0x8f, 0x83, 0x8e, 0x92, 0x50, 0x00, 0x00, 0x00, 0x00, 0x8c, 0x8e, 0x8b, 0x93, 0x91, 
  0x84, 0x92, 0x50, 0x00, 0x00, 0x00, 0x83, 0x91, 0x80, 0x93, 0x88, 0x8d, 0x88, 0x50, 0x00, 0x00, 
  0x00, 0x83, 0x91, 0x80, 0x86, 0x8e, 0x8d, 0x80, 0x88, 0x91, 0x50, 0x00, 0x83, 0x91, 0x80, 0x86, 
  0x8e, 0x8d, 0x88, 0x93, 0x84, 0x50, 0x00, 0x8c, 0x84, 0x96, 0x93, 0x96, 0x8e, 0x50, 0x00, 0x00, 
  0x00, 0x00, 0x8c, 0x84, 0x96, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

//1661 Byte Array of Data containing Encoded Japanese Pokemon Names (11 Bytes Each, although technically just 6)
const unsigned char japanesePokemonNameData[] PROGMEM  = {
  0x9b, 0x8b, 0x06, 0x0f, 0x97, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9b, 0x8b, 0x06, 0x8e, 0x82, 
  0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9b, 0x8b, 0x06, 0x19, 0x94, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x9a, 0x93, 0x85, 0x08, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x0a, 0xe3, 0x13, 
  0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x0a, 0xe3, 0x13, 0xab, 0x50, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x0d, 0x95, 0x05, 0xa0, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0xa0, 0xe3, 
  0xa6, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0xa0, 0xac, 0x87, 0x8c, 0x50, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x86, 0xad, 0x8f, 0x41, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x93, 0xa5, 
  0xab, 0x8d, 0xa6, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x8f, 0x9b, 0xd8, 0xe3, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x1a, 0xe3, 0x13, 0xa6, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 
  0x87, 0xe3, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8c, 0x41, 0x80, 0xe3, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0xac, 0x43, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x41, 0x0b, 0xaf, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x0b, 0xaf, 0xac, 0x93, 
  0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0xa5, 0xac, 0x8f, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xa5, 0xac, 0x8f, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x95, 0x8c, 0x0c, 
  0xa0, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x95, 0x13, 0xd8, 0xa6, 0x50, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x80, 0xe3, 0x1c, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xe3, 0x1c, 
  0xac, 0x87, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x85, 0x90, 0xae, 0x82, 0x50, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0xa5, 0x81, 0x90, 0xae, 0x82, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8a, 0xab, 
  0x13, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8a, 0xab, 0x13, 0x40, 0xab, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x95, 0x13, 0xa5, 0xab, 0xf5, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x95, 
  0x13, 0xd8, 0xe3, 0x94, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x95, 0x13, 0x87, 0x81, 0xab, 0x50, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x95, 0x13, 0xa5, 0xab, 0xef, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x95, 0x13, 0xd8, 0xe3, 0x98, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x95, 0x13, 0x86, 0xab, 0x07, 
  0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0xac, 0x41, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x41, 0x87, 0x8b, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa8, 0x89, 0xab, 0x50, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 0xae, 0x82, 0x89, 0xab, 0x50, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x42, 0xd8, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x87, 0xd8, 
  0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x19, 0xac, 0x93, 0x50, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x09, 0xa6, 0x19, 0xac, 0x93, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x94, 0x0e, 
  0x98, 0x87, 0x8a, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87, 0x8a, 0x81, 0x99, 0x94, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xa5, 0x9b, 0xa7, 0x8b, 0x80, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
  0xa5, 0x8c, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xa5, 0x8d, 0x87, 0x93, 0x50, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0xab, 0x40, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xa1, 0xa6, 0x9b, 0xf4, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0xb0, 0x07, 0x0f, 0x50, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x07, 0x93, 0xd8, 0x84, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x95, 0xad, 0xe3, 0x8c, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0xa6, 0x8b, 0x80, 
  0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0x0f, 0xac, 0x87, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x09, 0xa6, 0x0f, 0xac, 0x87, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9d, 0xab, 0x86, 
  0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x89, 0xd8, 0x0a, 0xa6, 0x50, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x05, 0xe3, 0x12, 0xb0, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0x81, 
  0xab, 0x12, 0xb0, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x95, 0xaf, 0xa8, 0xa1, 0x50, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x95, 0xaf, 0xa8, 0x0e, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x95, 
  0xaf, 0xa8, 0x1c, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0xe3, 0x8b, 0xb0, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xa3, 0xab, 0x08, 0xa5, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x9b, 0xe3, 0x12, 0xb0, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa9, 0xab, 0xd8, 0x86, 0xe3, 
  0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0xe3, 0xd8, 0x86, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x85, 0x81, 0xd8, 0x86, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9d, 0x0f, 0x91, 0x1c, 
  0x9e, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0x91, 0x13, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x82, 0x91, 0x1c, 0xac, 0x93, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x98, 0x87, 
  0xa5, 0x08, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x87, 0x87, 0xa5, 0x08, 0x50, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x81, 0x8b, 0x91, 0x1b, 0x92, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0xa8, 
  0xe3, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0xa8, 0xe3, 0x95, 0xad, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x43, 0x95, 0xe3, 0x8f, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
  0xad, 0xa8, 0xac, 0x42, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa2, 0x13, 0xab, 0x50, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xa2, 0x13, 0xa5, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x89, 0x81, 0xa6, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa7, 0x80, 0x89, 0x81, 0xa6, 
  0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0xa1, 0x97, 0x06, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x13, 0xe3, 0x13, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0xe3, 0x13, 0xd8, 
  0x84, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x82, 0xa9, 0x82, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x0b, 0xae, 0x09, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x93, 0x3d, 
  0x8f, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x93, 0x3d, 0x93, 0xab, 0x50, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x8b, 0xeb, 0xa6, 0x0f, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xa6, 
  0x8b, 0xeb, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0xe3, 0x8c, 0x50, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x09, 0xe3, 0x8c, 0x93, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 
  0xab, 0x05, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0xa9, 0xe3, 0x87, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x8c, 0xd8, 0xe3, 0x42, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x8c, 0xd8, 0xe3, 0x40, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87, 0xa5, 0x1b, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 0xab, 0x07, 0xa5, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x1a, 0xd8, 0xd8, 0x0f, 0x9d, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9d, 0xa6, 0x9d, 0x81, 
  0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x9d, 0x8f, 0x9d, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x94, 0xac, 0x8b, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0xa5, 0x85, 
  0xa5, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0xa5, 0x05, 0xa5, 0x50, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x8a, 0xa9, 0x9f, 0xa5, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x1a, 
  0xa9, 0xa5, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0xa8, 0xd8, 0xab, 0x05, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x13, 0x05, 0xe3, 0x8c, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9d, 
  0x8f, 0x13, 0x05, 0x8c, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x81, 0x9c, 0xe3, 0xab, 0x50, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x81, 0x13, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xa5, 0xac, 0x86, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa1, 0xab, 0x0b, 0xad, 0xa5, 
  0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0xa6, 0xe3, 0xa5, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x8f, 0xac, 0x91, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8b, 0xe3, 0x13, 0xa5, 
  0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x93, 0x8a, 0x86, 0xab, 0x93, 0x50, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x80, 0x0c, 0x9d, 0x84, 0x82, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9a, 0x93, 0x12, 
  0x9d, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8c, 0x8f, 0xe3, 0x9e, 0xe3, 0x50, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x19, 0xd8, 0xa2, 0xe3, 0x13, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8c, 0x93, 
  0xa5, 0x81, 0x87, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa6, 0xe3, 0x0b, 0xae, 0xa5, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x83, 0xa7, 0x1b, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1b, 
  0xe3, 0x19, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x81, 0xa8, 0x8c, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0xab, 0x8f, 0xa8, 0x8c, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x89, 0x81, 0x86, 0xab, 0x07, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xad, 0xa5, 0x13, 0x8c, 
  0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa5, 0x42, 0xa5, 0x8c, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xa0, 0x8f, 0xa1, 0xab, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0xe3, 0x1b, 0x81, 
  0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8b, 0xad, 0xa9, 0xe3, 0x0c, 0x50, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x8a, 0xab, 0x0f, 0xe3, 0x8c, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1b, 0xe3, 0x8c, 
  0x8f, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0xd8, 0x09, 0xab, 0x50, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x84, 0x9f, 0x94, 0x81, 0x93, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x9f, 
  0x8c, 0x8f, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x1b, 0x93, 0x50, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x85, 0x1b, 0x93, 0x42, 0x8c, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 
  0x92, 0xa5, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x1a, 0x09, 0xab, 0x50, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x9b, 0xd8, 0xe3, 0x0a, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x8a, 0xab, 0x0f, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9b, 0xe9, 0x81, 0xa2, 0xe3, 
  0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9e, 0x95, 0xd8, 0xae, 0x82, 0x50, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x99, 0x87, 0xd8, 0xae, 0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x81, 0xd8, 0xae, 
  0xe3, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9e, 0xae, 0x82, 0x91, 0xe3, 0x50, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x9e, 0xae, 0x82, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
//Array to get the Pokedex # from the Species ID. Subtract 1 to get the index number within the array.
signed short speciesIDToPokedexNumberArray[] = { -1,112,115,32,35,21,100,34,80,2,103,108,102,88,94,29,31,104,111,131,59,151,130,90,72,92,123,120,9,127,114,-1,-1,58,95,22,16,79,64,75,113,67,122,106,107,24,47,54,96,76,-1,126,-1,125,82,109,-1,56,86,50,128,-1,-1,-1,83,48,149,-1,-1,-1,84,60,124,146,144,145,132,52,98,-1,-1,-1,37,38,25,26,-1,-1,147,148,140,141,116,117,-1,-1,27,28,138,139,39,40,133,136,135,134,66,41,23,46,61,62,13,14,15,-1,85,57,51,49,87,-1,-1,10,11,12,68,-1,55,97,42,150,143,129,-1,-1,89,-1,99,91,-1,101,36,110,53,105,-1,93,63,65,17,18,121,1,3,73,-1,118,119,-1,-1,-1,-1,77,78,19,20,33,30,74,137,142,-1,81,-1,-1,4,7,5,8,6,-1,-1,-1,-1,43,44,45,69,70,71 };
//Stores the TrainerIDs associated with each Gameboy.
unsigned short gameboyATrainerID = 24879;
unsigned short gameboyBTrainerID = 16987;
//Stores all of the trainer IDs that can participate in the trade.
unsigned short trainerIDList[] = {24879, 16987, 56838, 63599};
//Stores the encoded names that will be used for Pokemon Traded from Gameboy A(ENG) to Gameboy B(JPN).
unsigned char gameboyAToBEncodedTrainerNames[][6] = {
  { 0x9D, 0xAC, 0x87, 0x50, 0x00, 0x00 }, //マック -> MAKKU
  { 0x8A, 0x81, 0x93, 0x50, 0x00, 0x00 }, //サイト -> SAITO
  { 0x93, 0xA7, 0xE3, 0x94, 0xE3, 0x50 }, //トレーナー -> TOREENAA/TRAINER
  { 0x9D, 0xAC, 0x87, 0x50, 0x00, 0x00 } //マック -> MAKKU
}; 
//Stores the encoded names that will be used for Pokemon Traded from Gameboy B(ENG) to Gameboy A(JPN).
unsigned char gameboyBToAEncodedTrainerNames[][11] = {
  { 0x8C, 0x80, 0x82, 0x8A, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, //MACK
  { 0x92, 0x80, 0x88, 0x93, 0x8E, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00 }, //SAITO
  { 0x93, 0x91, 0x80, 0x88, 0x8D, 0x84, 0x91, 0x50, 0x00, 0x00, 0x00 }, //TRAINER
  { 0x8C, 0x80, 0x82, 0x8A, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, //MACK
};
//Stores the party data sent from each Gameboy.
unsigned char gameboyAPartyData[7];
unsigned char gameboyBPartyData[7];
unsigned char gameboyAPartyDataPos = 0;
unsigned char gameboyBPartyDataPos = 0;
//Stores the Trainer IDs of the pokemon from each Gameboy.
unsigned char gameboyAPokemonTrainerIDs[12];
unsigned char gameboyBPokemonTrainerIDs[12];
unsigned char gameboyAPokemonTrainerIDPos = 0;
unsigned char gameboyBPokemonTrainerIDPos = 0;
unsigned char gameboyACurrentPokemonTrainerIDIndex = 0;
unsigned char gameboyBCurrentPokemonTrainerIDIndex = 0;
/*GLOBAL VARIABLES*/
//Stores the bytes read from each gameboy.
unsigned char linkIncomingByteA = 0xFE;
unsigned char linkIncomingByteB = 0xFE;
//Stores the bytes that need to be sent to each gameboy.
unsigned char linkOutgoingByteA = 0xFE;
unsigned char lastLinkOutgoingByteA = 0xFE;
unsigned char linkOutgoingByteB = 0xFE;
unsigned char lastLinkOutgoingByteB = 0xFE;
//Stores the last exchanged bytes triggered by a State of Type 1.
unsigned char lastExchangedByteToA = 0x0;
unsigned char lastExchangedByteToB = 0x0;
//Buffers that are used to store translated data to send to the Gameboys.
unsigned char translatedIncomingBufferDataA[32];
unsigned char translatedIncomingBufferDataB[32];
//Stores the amount of translated bytes present within the buffers.
unsigned char translatedIncomingBufferALength = 0;
unsigned char translatedIncomingBufferBLength = 0;
//Stores the position of the byte to send from within the buffer.
unsigned char translatedIncomingBufferAPosition = 0;
unsigned char translatedIncomingBufferBPosition = 0;
//Stores whether a translation exchange point has been reached.
bool translationPointReachedForGameboyA = false;
bool translationPointReachedForGameboyB = false;
//Stores the amount of times to re-transmit a particular byte.
unsigned char linkOutgoingByteARepeatCounter = 0;
unsigned char linkOutgoingByteBRepeatCounter = 0;
//Booleans that control whether transmission with a particular gameboy is allowed or not.
bool enableTransmission = false;
bool enableTransmissionA = true;
bool enableTransmissionB = true;
//A flag used to disable transmission after a clock pulse.
bool disableTransmissionAAfterPulse = false;
bool disableTransmissionBAfterPulse = false;
bool enableTransmissionAAfterPulse = false;
bool enableTransmissionBAfterPulse = false;
//Stores the amount of bytes that was exchanged with the Gameboys.
unsigned short transferredByteCounterA = 0;
unsigned short transferredByteCounterB = 0;
//Whether transmission has been temporarily disabled between a particular Gameboy.
bool transmissionATemporarilyDisabled = false;
bool transmissionBTemporarilyDisabled = false;
//Stores the time when transmission was temporarily disabled and how long it should be disabled.
unsigned long transmissionATemporaryDisabledTime = 0;
unsigned long transmissionBTemporaryDisabledTime = 0;
unsigned long transmissionATemporaryDisableDuration = 0;
unsigned long transmissionBTemporaryDisableDuration = 0;
//Stores all of the states that drive the communication.
LinkState linkStates[STATE_COUNT];
//The index of the active state.
unsigned int activeLinkStateIndexA = 0;
unsigned int activeLinkStateIndexB = 0;
//Used to keep track of the microseconds that have passed.
unsigned long time = 0;
//Obtain the indices of each gameboy.
signed int gameboyATrainerIndex = 0;
signed int gameboyBTrainerIndex = 0;
//Debugging Variables
bool enableDebugOutput = false;
char charBuffer[64];

//Obtains the index of a particular trainer, by ID
signed int getTrainerIndexFromID(unsigned short trainerID){
  for(signed int i = 0; i < TRAINER_ID_COUNT; i++){
    if(trainerIDList[i] == trainerID){
      return i;
    }
  }
  return -1;
}

void setup() {
  //Set the serial output speed.
  Serial.begin(9600);
  //Set the Input and Output pins.
  pinMode(DMG_GAMEBOY_A_LINK_CLK, OUTPUT);
  pinMode(DMG_GAMEBOY_B_LINK_CLK, OUTPUT);
  pinMode(DMG_GAMEBOY_A_LINK_SIN, OUTPUT);
  pinMode(DMG_GAMEBOY_B_LINK_SOUT, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(DMG_GAMEBOY_A_LINK_SOUT, INPUT);
  pinMode(DMG_GAMEBOY_B_LINK_SIN, INPUT);
  pinMode(DMG_GAMEBOY_B_LINK_SIN, INPUT);
  //pinMode(START_TRANSMISSION_BUTTON_PIN, INPUT);
  //Set the defauly states of the signals.
  digitalWrite(DMG_GAMEBOY_A_LINK_CLK, HIGH);
  digitalWrite(DMG_GAMEBOY_B_LINK_CLK, HIGH);
  digitalWrite(DMG_GAMEBOY_A_LINK_SIN, LOW);
  digitalWrite(DMG_GAMEBOY_B_LINK_SOUT, LOW);
  /*STATE DATA SETUP*/
  //State 0: The state where the Arduino waits for connections from each Gameboy. 
  linkStates[0].stateType = 0;
  linkStates[0].targetByte = 0x60;
  linkStates[0].sendsByteByDefault = true;
  linkStates[0].defaultResponseByte = 0x1;
  //State 1: The state that precedes the menu where the options are displayed. 0x60 is exchanged between the Gameboys and the Arduino.
  linkStates[1].stateType = 0;
  linkStates[1].targetByte = 0x60;
  linkStates[1].respondsWithByte = true;
  linkStates[1].responseByte = 0x60;
  linkStates[1].sendsByteByDefault = true;
  linkStates[1].defaultResponseByte = 0x60;
  linkStates[1].transmissionDelayTime = 600;
  //State 2: The state where the options are being presented to each gameboy. 0xD4 indicates the intention to enter the Trade Center. Send this to each Gameboy and wait for each Gameboy to send 0xD0. Repeat to make sure the Gameboy gets the byte.
  linkStates[2].stateType = 0;
  linkStates[2].targetByte = 0xD0;
  linkStates[2].respondsWithByte = true;
  linkStates[2].responseByte = 0xD4;
  linkStates[2].responseByteRepeatCount = 3;
  linkStates[2].sendsByteByDefault = false;
  linkStates[2].transmissionDelayTime = 600;
  //State 3: The state where each Gameboy is in the Trade Center and the Arduino is waiting for the Trade to begin. 0x60 indicates that the trade is beginning.
  linkStates[3].stateType = 0;
  linkStates[3].targetByte = 0x60;
  linkStates[3].respondsWithByte = true;
  linkStates[3].responseByte = 0x60;
  linkStates[3].responseByteRepeatCount = 5;
  linkStates[3].sendsByteByDefault = true;
  linkStates[3].defaultResponseByte = 0xFE;
  linkStates[3].transmissionDelayTime = 100;
  //State 4: The state where the Arduino waits for 16 consecutive Preamble Bytes(0xFD) from each gameboy, and then responds with the Preamble Byte. 
  linkStates[4].stateType = 0;
  linkStates[4].targetByte = 0xFD;
  linkStates[4].targetByteOccurrenceCount = 16;
  linkStates[4].respondsWithByteUponTransition = true;
  linkStates[4].responseByte = 0xFD;
  linkStates[4].sendsByteByDefault = false;
  linkStates[4].transmissionDelayTime = 50;
  //State 5: The state where the Arduino waits for the Preamble Byte(0xFD) from each gameboy, and then responds with the Preamble Byte. Just another countermeasure to make sure the Gameboy gets the byte.
  linkStates[5].stateType = 0;
  linkStates[5].targetByte = 0xFD;
  linkStates[5].respondsWithByte = true;
  linkStates[5].responseByte = 0xFD;
  linkStates[5].sendsByteByDefault = false;
  //State 6: The state where the Arduino waits for the Gameboys to send anything but the Preamble Byte, signaling the start of data.
  linkStates[6].stateType = 1;
  linkStates[6].exchangesBytes = true;
  linkStates[6].targetByte = 0xFD;
  linkStates[6].inverseCondition = true;
  linkStates[6].sendsByteByDefault = false;
  //State 7: The state where the Arduino transmits Random Number data between Gameboy A and Gameboy B.
  linkStates[7].stateType = 2;
  linkStates[7].bufferIndex = 0;
  linkStates[7].bufferASize = 10;//17 if Preamble Bytes were included, but we are skipping over them.
  linkStates[7].bufferBSize = 10;//17 if Preamble Bytes were included, but we are skipping over them.
  linkStates[7].sendsByteByDefault = false;
  //State 4: The state where the Arduino waits for 16 consecutive Preamble Bytes(0xFD) from each gameboy, and then responds with the Preamble Byte. 
  linkStates[8].stateType = 0;
  linkStates[8].targetByte = 0xFD;
  linkStates[8].targetByteOccurrenceCount = 16;
  linkStates[8].respondsWithByteUponTransition = true;
  linkStates[8].responseByte = 0xFD;
  linkStates[8].sendsByteByDefault = false;
  linkStates[8].transmissionDelayTime = 50;
  //State 9: The state where the Arduino waits for the Preamble Byte(0xFD) from each gameboy, and then responds with the Preamble Byte.
  linkStates[9].stateType = 0;
  linkStates[9].targetByte = 0xFD;
  linkStates[9].respondsWithByte = true;
  linkStates[9].responseByte = 0xFD;
  linkStates[9].sendsByteByDefault = false;
  //State 10: The state where the Arduino waits for the Gameboys to send anything but the Preamble Byte, signaling the start of data.
  linkStates[10].stateType = 1;
  linkStates[10].targetByte = 0xFD;
  linkStates[10].inverseCondition = true;
  linkStates[10].sendsByteByDefault = false;
  //State 11: The state where the Arduino transmits the Trainer Name and Party Pokemon data between Gameboy A and Gameboy B.
  linkStates[11].stateType = 2;
  linkStates[11].bufferIndex = 1;
  linkStates[11].bufferASize = 418; //424 if Preamble Bytes were included, but we are skipping over them.
  linkStates[11].bufferBSize = 353; //359 if Preamble Bytes were included, but we are skipping over them.
  linkStates[11].sendsByteByDefault = false;
  //State 4: The state where the Arduino waits for 16 consecutive Preamble Bytes(0xFD) from each gameboy, and then responds with the Preamble Byte. 
  linkStates[12].stateType = 0;
  linkStates[12].targetByte = 0xFD;
  linkStates[12].targetByteOccurrenceCount = 16;
  linkStates[12].respondsWithByteUponTransition = true;
  linkStates[12].responseByte = 0xFD;
  linkStates[12].sendsByteByDefault = false;
  linkStates[12].transmissionDelayTime = 50;
  //State 13: The state where the Arduino waits for the Preamble Byte(0xFD) from each gameboy, and then responds with the Preamble Byte.
  linkStates[13].stateType = 0;
  linkStates[13].targetByte = 0xFD;
  linkStates[13].respondsWithByte = true;
  linkStates[13].responseByte = 0xFD;
  linkStates[13].sendsByteByDefault = false;
  //State 14: The state where the Arduino waits for the Gameboys to send anything but the Preamble Byte, signaling the start of data.
  linkStates[14].stateType = 1;
  linkStates[14].exchangesBytes = true;
  linkStates[14].targetByte = 0xFD;
  linkStates[14].inverseCondition = true;
  linkStates[14].sendsByteByDefault = false;
  //State 15: The state where the Arduino transmits patch list data between Gameboy A and Gameboy B.
  linkStates[15].stateType = 2;
  linkStates[15].bufferIndex = 2;
  linkStates[15].bufferASize = 197; //200 if Preamble Bytes were included, but we are skipping over them.
  linkStates[15].bufferBSize = 197; //200 if Preamble Bytes were included, but we are skipping over them.
  linkStates[15].sendsByteByDefault = false;
  linkStates[15].transmissionDelayTime = 500;
  //State 16: The state where the Arduino waits for the Gameboys to select the pokemon they'd like to trade.
  linkStates[16].stateType = 1;
  linkStates[16].exchangesBytes = true;
  linkStates[16].targetByteUsesRange = true;
  linkStates[16].targetByte = 0x60;
  linkStates[16].targetByteEnd = 0x65;
  linkStates[16].sendsByteByDefault = false;
  //State 17: The state where the Arduino waits for the gameboy to send 0x0, following the trade selection.
  linkStates[17].stateType = 0;
  linkStates[17].targetByte = 0x0;
  linkStates[17].sendsByteByDefault = false;
  //State 18: The state where the Arduino waits for the Gameboys to send 0x62 to finalize the trade, and it responds back with 0x62.
  linkStates[18].stateType = 1;
  linkStates[18].exchangesBytes = true;
  linkStates[18].targetByte = 0x62;
  linkStates[18].sendsByteByDefault = false;
  //State 19: The state where the Arduino waits for a while as the trade animation occurs. Just to make the clock stop for a while, as no communication happens during the trade animation.
  linkStates[19].stateType = 4;
  linkStates[19].transmissionDelayTime = 40000;
  linkStates[19].sendsByteByDefault = false;
  //State 19: The state where 0x62 is sent to the Gameboys to complete the trade.
  linkStates[20].stateType = 0;
  linkStates[20].targetByte = 0xFD;
  linkStates[20].defaultResponseByte = 0x62;
  //State 21: The state where the Arduino waits for the Gameboys to both send the 0xFD, to make sure the gameboys stay sync'd, even in the case of evolution.
  linkStates[21].stateType = 1;
  linkStates[21].targetByte = 0xFD;
  linkStates[21].sendsByteByDefault = false;
  //State 22: Jump back to state 4 to re-exchange data with the Gameboys.
  linkStates[22].stateType = 3;
  linkStates[22].sendsByteByDefault = false;
  linkStates[22].jumpStateIndex = 4;
  //State 23: The state that gets manually triggered whenever the Gameboy cancels the trade.
  linkStates[23].stateType = 0;
  linkStates[23].targetByte = 0x0;
  linkStates[23].sendsByteByDefault = true;
  linkStates[23].defaultResponseByte = 0x6F;
  //State 24: The state where the Arduino waits for the Gameboys to send 0x6F, which is what gets sent whenever a player cancels the trade session and starts a new one.
  linkStates[24].stateType = 0;
  linkStates[24].targetByte = 0x6F;
  linkStates[24].respondsWithByte = true;
  linkStates[24].responseByte = 0x60;
  linkStates[24].responseByteRepeatCount = 5;
  linkStates[24].sendsByteByDefault = true;
  linkStates[24].defaultResponseByte = 0xFE;
  linkStates[24].transmissionDelayTime = 100;
  //State 25: Jump back to state 4 to re-exchange data with the Gameboys.
  linkStates[25].stateType = 3;
  linkStates[25].sendsByteByDefault = false;
  linkStates[25].jumpStateIndex = 4;

  //Obtain the indices of each gameboy.
  gameboyATrainerIndex = getTrainerIndexFromID(gameboyATrainerID);
  gameboyBTrainerIndex = getTrainerIndexFromID(gameboyBTrainerID);

  //Give the user 15 seconds to make sure their gameboys are connected.
  digitalWrite(LED_BUILTIN, LOW);
  delay(15000);
  digitalWrite(LED_BUILTIN, HIGH);
  enableTransmission = true;
}

//Handles the logic associated with states.
void UpdateStates(unsigned int* activeLinkStateIndex){
  //Set the output bytes if the active state responds with one by default.
  if(linkStates[(*activeLinkStateIndex)].sendsByteByDefault){
    if(!linkStates[(*activeLinkStateIndex)].useLastExchangedBytesAsDefaultResponseByte){
      //STATE A
      if(activeLinkStateIndex == &activeLinkStateIndexA){
        linkOutgoingByteA = linkStates[(*activeLinkStateIndex)].defaultResponseByte;
      }
      //STATE B
      else{
        linkOutgoingByteB = linkStates[(*activeLinkStateIndex)].defaultResponseByte;
      }
    }else{
      //STATE A
      if(activeLinkStateIndex == &activeLinkStateIndexA){
        linkOutgoingByteA = lastExchangedByteToA;
      }
      //STATE B
      else{
        linkOutgoingByteB = lastExchangedByteToB;
      }
    }
  }
  //Proceed if this is a state that waits on a particular byte.
  if(linkStates[(*activeLinkStateIndex)].stateType == 0){
    //Stores whether a target byte has been received that satisfies the condition.
    bool targetByteSatisfiesCondition = false;
    //Check to see if the byte received by Gameboy A matches that which is required by the state.
    //STATE A
    if(activeLinkStateIndex == &activeLinkStateIndexA){
      if(linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterA > 0){
        if(!linkStates[(*activeLinkStateIndex)].targetByteUsesRange){
          if(linkIncomingByteA == linkStates[(*activeLinkStateIndex)].targetByte){
              linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterA--;
              targetByteSatisfiesCondition = true;
              if(linkStates[(*activeLinkStateIndex)].respondsWithByte){
                linkOutgoingByteA = linkStates[(*activeLinkStateIndex)].responseByte;
                lastLinkOutgoingByteA = linkOutgoingByteA;
                linkOutgoingByteARepeatCounter = linkStates[(*activeLinkStateIndex)].responseByteRepeatCount + 1;
              }
          }
        }else{
          if(linkIncomingByteA >= linkStates[(*activeLinkStateIndex)].targetByte && linkIncomingByteA <= linkStates[(*activeLinkStateIndex)].targetByteEnd){
              linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterA--;
              targetByteSatisfiesCondition = true;
              if(linkStates[(*activeLinkStateIndex)].respondsWithByte){
                linkOutgoingByteA = linkStates[(*activeLinkStateIndex)].responseByte;
                lastLinkOutgoingByteA = linkOutgoingByteA;
                linkOutgoingByteARepeatCounter = linkStates[(*activeLinkStateIndex)].responseByteRepeatCount + 1;
              }
          }
        }
      }
    }
    //STATE B
    else{
      //Check to see if the byte received by Gameboy A matches that which is required by the state.
      if(linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterB > 0){
        if(!linkStates[(*activeLinkStateIndex)].targetByteUsesRange){
          if(linkIncomingByteB == linkStates[(*activeLinkStateIndex)].targetByte){
              linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterB--;
              targetByteSatisfiesCondition = true;
              if(linkStates[(*activeLinkStateIndex)].respondsWithByte){
                linkOutgoingByteB = linkStates[(*activeLinkStateIndex)].responseByte;
                lastLinkOutgoingByteB = linkOutgoingByteB;
                linkOutgoingByteBRepeatCounter = linkStates[(*activeLinkStateIndex)].responseByteRepeatCount + 1;
              }
          }
        }else{
          if(linkIncomingByteB >= linkStates[(*activeLinkStateIndex)].targetByte && linkIncomingByteB <= linkStates[(*activeLinkStateIndex)].targetByteEnd){
              linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterB--;
              targetByteSatisfiesCondition = true;
              if(linkStates[(*activeLinkStateIndex)].respondsWithByte){
                linkOutgoingByteB = linkStates[(*activeLinkStateIndex)].responseByte;
                lastLinkOutgoingByteB = linkOutgoingByteB;
                linkOutgoingByteBRepeatCounter = linkStates[(*activeLinkStateIndex)].responseByteRepeatCount + 1;
              }
          }
        }
      } 
    }
    //Proceed if the counter resets itself upon unsatisfied conditions.
    if(linkStates[(*activeLinkStateIndex)].resetOccurrenceCounterUponConflictingByte){
      if(!targetByteSatisfiesCondition){
        //STATE A
        if(activeLinkStateIndex == &activeLinkStateIndexA){
          linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterA = linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCount;
        }
        //STATE B
        else{
          linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterB = linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCount;
        }
      }
    }
    //Check to see if the next state needs to be transitioned to.
    bool canProceedToNextState = false;
    //STATE A
    if(activeLinkStateIndex == &activeLinkStateIndexA){
      if(linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterA == 0){
        if(!linkStates[(*activeLinkStateIndex)].canOnlyProceedOnCertainByte){
            canProceedToNextState = true;
        }else{
          if(linkIncomingByteA == linkStates[(*activeLinkStateIndex)].requiredByteToProceed){
            canProceedToNextState = true;
          }
        }
      }
    }
    //STATE B
    else{
      if(linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterB == 0){
        if(!linkStates[(*activeLinkStateIndex)].canOnlyProceedOnCertainByte){
            canProceedToNextState = true;
        }else{
          if(linkIncomingByteB == linkStates[(*activeLinkStateIndex)].requiredByteToProceed){
            canProceedToNextState = true;
          }
        }
      }
    }
    //If more bytes need to be sent, remain in this state.
    if(activeLinkStateIndex == &activeLinkStateIndexA){
      if(linkOutgoingByteARepeatCounter > 0){
        canProceedToNextState = false;
      }
    }else{
      if(linkOutgoingByteBRepeatCounter > 0){
        canProceedToNextState = false;
      }
    }
    //Proceed to the next state, if applicable.
    if(canProceedToNextState){
      //If another state exists, proceed.
      if((*activeLinkStateIndex) < STATE_COUNT - 1){
        //Send the byte whenever the state is transitioning, if applicable.
        if(linkStates[(*activeLinkStateIndex)].respondsWithByteUponTransition){
          //STATE A
          if(activeLinkStateIndex == &activeLinkStateIndexA){
            linkOutgoingByteA = linkStates[(*activeLinkStateIndex)].responseByte;
          }
          //STATE B
          else{
            linkOutgoingByteB = linkStates[(*activeLinkStateIndex)].responseByte;
          }
        }
        //If the state temporarily disables transmission, apply it.
        DelayTransmissionBasedOnActiveState(activeLinkStateIndex);
        //Make the next state active.
        (*activeLinkStateIndex)++;
        //Reset the counters.
        ResetStateByteCounters(activeLinkStateIndex);
        //Debugging.
        OutputStateChangeDebugString(activeLinkStateIndex);
      }
    }
  }
  //Proceed if this state waits for the data to start following the transmission of a particular byte.
  else if(linkStates[(*activeLinkStateIndex)].stateType == 1){
    //Prevent this state from performing any checks if data transmission is temporarily disabled.
    if((activeLinkStateIndex == &activeLinkStateIndexA && !transmissionATemporarilyDisabled) || (activeLinkStateIndex == &activeLinkStateIndexB && !transmissionBTemporarilyDisabled)){
      //Check for a specific byte or range of bytes. Once they are received, stop transmission with the corresponding gameboy.
      bool conditionMet = false;
      //STATE A
      if(activeLinkStateIndex == &activeLinkStateIndexA){
        if(!linkStates[(*activeLinkStateIndex)].targetByteUsesRange){
          if(linkIncomingByteA == linkStates[(*activeLinkStateIndex)].targetByte){
            conditionMet = true;
          }
        }else{
          if(linkIncomingByteA >= linkStates[(*activeLinkStateIndex)].targetByte && linkIncomingByteA <= linkStates[(*activeLinkStateIndex)].targetByteEnd){
            conditionMet = true;
          }
        }
        //Apply the inverse, if applicable.
        if(linkStates[(*activeLinkStateIndex)].inverseCondition){
          conditionMet = !conditionMet;
        }
        //Disable the transmission upon the condition being met.
        if(conditionMet){
          enableTransmissionA = false;
        }
      }
      //STATE B
      else{
        if(!linkStates[(*activeLinkStateIndex)].targetByteUsesRange){
          if(linkIncomingByteB == linkStates[(*activeLinkStateIndex)].targetByte){
            conditionMet = true;
          }
        }else{
          if(linkIncomingByteB >= linkStates[(*activeLinkStateIndex)].targetByte && linkIncomingByteB <= linkStates[(*activeLinkStateIndex)].targetByteEnd){
            conditionMet = true;
          }
        }
        //Apply the inverse, if applicable.
        if(linkStates[(*activeLinkStateIndex)].inverseCondition){
          conditionMet = !conditionMet;
        }
        //Disable the transmission upon the condition being met.
        if(conditionMet){
          enableTransmissionB = false;
        }
      }
      //Proceed to the next state once all transmissions are disabled.
      if(!enableTransmissionA && !enableTransmissionB && !transmissionATemporarilyDisabled && !transmissionBTemporarilyDisabled){
        //Only apply this logic when Gameboy B's states are being updated.
        if(activeLinkStateIndex == &activeLinkStateIndexB){
          if(linkStates[(*activeLinkStateIndex)].exchangesBytes){
            //Set the outgoing bytes to exchange the data from each gameboy.
            linkOutgoingByteA = linkIncomingByteB;
            linkOutgoingByteB = linkIncomingByteA;
          }
          //Increment both A and B's states.
          activeLinkStateIndexA++;
          activeLinkStateIndexB++;
          if(enableDebugOutput){
            if(linkStates[activeLinkStateIndexA].bufferIndex != -1){
              Serial.print("Proceeding to Read Buffer ");
              Serial.println(linkStates[activeLinkStateIndexA].bufferIndex);
            }else{
              Serial.print("A and B Proceeding to State ");
              Serial.println(activeLinkStateIndexA);
            }
          }
          //Reset the byte counters.
          linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterA = linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCount;
          linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterB = linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCount;
          //Re-enable the transmission.
          enableTransmissionA = true;
          enableTransmissionB = true;
          //Reset the byte counter.
          transferredByteCounterA = 1;
          transferredByteCounterB = 1;
          //If this state starts the transmission of Buffer 1, copy the translated trainer names into the buffers to be transmitted.
          if(linkStates[activeLinkStateIndexA].bufferIndex == 1){
            //JAPANESE TRAINER NAME IN ENGLISH
            for(signed int i = 0; i < 11; i++){
              translatedIncomingBufferDataA[i] = gameboyBToAEncodedTrainerNames[gameboyBTrainerIndex][i];
            }
            translatedIncomingBufferALength = 11;
            translatedIncomingBufferAPosition = 1;
            //Copy the data into each of the buffers and set the variables to continue the transfer.
            //ENGLISH TRAINER NAME IN JAPANESE
            for(signed int i = 0; i < 6; i++){
              translatedIncomingBufferDataB[i] = gameboyAToBEncodedTrainerNames[gameboyATrainerIndex][i];
            }
            translatedIncomingBufferBLength = 6;
            translatedIncomingBufferBPosition = 1;
            //Set the outgoing bytes to the first byte of the buffers.
            linkOutgoingByteA = translatedIncomingBufferDataA[0];
            linkOutgoingByteB = translatedIncomingBufferDataB[0];
            //Reset some variables.
            gameboyAPokemonTrainerIDPos = 0;
            gameboyBPokemonTrainerIDPos = 0;
            gameboyACurrentPokemonTrainerIDIndex = 0;
            gameboyBCurrentPokemonTrainerIDIndex = 0;
            gameboyAPartyDataPos = 0;
            gameboyBPartyDataPos = 0;
          }
        }
      }
    }
  }
  //Proceed if this state translates and exchanges data between the Gameboys.
  else if(linkStates[(*activeLinkStateIndex)].stateType == 2){
    //Skip over the logic of this state if transmission is disabled.
    bool skipLogic = false;
    if(activeLinkStateIndex == &activeLinkStateIndexA && !enableTransmissionA){
      skipLogic = true;
    }
    else if(activeLinkStateIndex == &activeLinkStateIndexB && !enableTransmissionB){
      skipLogic = true;
    }
    if(!skipLogic){
      //Obtain the transferred byte counter associated with this gameboy.
      unsigned short* transferredByteCounterPtr = &transferredByteCounterA;
      if(activeLinkStateIndex == &activeLinkStateIndexB){
        transferredByteCounterPtr = &transferredByteCounterB;
      }
      //Record party information, if applicable.
      if(linkStates[(*activeLinkStateIndex)].bufferIndex == 1){
        if(activeLinkStateIndex == &activeLinkStateIndexA){
          if(transferredByteCounterA >= 11 && transferredByteCounterA < 18){
            if(gameboyAPartyDataPos < 7){
              if(enableDebugOutput){
                sprintf(charBuffer, "(A) Party Data Index %d: %02X\n", gameboyAPartyDataPos, linkIncomingByteA);
                Serial.print(charBuffer);
              }
              gameboyAPartyData[gameboyAPartyDataPos] = linkIncomingByteA;
              gameboyAPartyDataPos++;
            }
          }
        }else{
          if(transferredByteCounterB >= 6 && transferredByteCounterB < 13){
            if(gameboyBPartyDataPos < 7){
              if(enableDebugOutput){
                sprintf(charBuffer, "(B) Party Data Index %d: %02X\n", gameboyBPartyDataPos, linkIncomingByteB);
                Serial.print(charBuffer);
              }
              gameboyBPartyData[gameboyBPartyDataPos] = linkIncomingByteB;
              gameboyBPartyDataPos++;
            }
          }
        }
      }
      //Record Trainer ID information if applicable.
      if(linkStates[(*activeLinkStateIndex)].bufferIndex == 1){
        if(activeLinkStateIndex == &activeLinkStateIndexA){
          if(transferredByteCounterA > 18 && transferredByteCounterA < 282){
            if(((transferredByteCounterA - 19) % 44) == 12 || ((transferredByteCounterA - 19) % 44) == 13){
              gameboyAPokemonTrainerIDs[gameboyAPokemonTrainerIDPos] = linkIncomingByteA;
              gameboyAPokemonTrainerIDPos++;
            }
          }
        }else{
          if(transferredByteCounterB > 13 && transferredByteCounterB < 277){
            if(((transferredByteCounterB - 14) % 44) == 12 || ((transferredByteCounterB - 14) % 44) == 13){
              gameboyBPokemonTrainerIDs[gameboyBPokemonTrainerIDPos] = linkIncomingByteB;
              gameboyBPokemonTrainerIDPos++;
            }
          }
        }
      }
      
      //Handle intercepting the exchanging of bytes at specific offsets within the buffer.
      //GAMEBOY A(USA)
      if(activeLinkStateIndex == &activeLinkStateIndexA && !translationPointReachedForGameboyA){
        //(0x11B, 0x126, 0x131, 0x13C, 0x147, 0x152) (OT Names)
        if(transferredByteCounterA == 0x11B || transferredByteCounterA == 0x126 || transferredByteCounterA == 0x131 || transferredByteCounterA == 0x13C || transferredByteCounterA == 0x147 || transferredByteCounterA == 0x152){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyA = true;
          //Disable transmission.
          enableTransmissionA = false;
        }
        //(0x15D)(Pokemon A Nickname)
        else if(transferredByteCounterA == 0x15D){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyA = true;
          //Disable transmission.
          enableTransmissionA = false;
        }
        //(0x168)(Pokemon B Nickname)
        else if(transferredByteCounterA == 0x168){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyA = true;
          //Disable transmission.
          enableTransmissionA = false;
        }
        //(0x173)(Pokemon C Nickname)
        else if(transferredByteCounterA == 0x173){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyA = true;
          //Disable transmission.
          enableTransmissionA = false;
        }
        //(0x17E)(Pokemon D Nickname)
        else if(transferredByteCounterA == 0x17E){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyA = true;
          //Disable transmission.
          enableTransmissionA = false;
        }
        //(0x189)(Pokemon E Nickname)
        else if(transferredByteCounterA == 0x189){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyA = true;
          //Disable transmission.
          enableTransmissionA = false;
        }
        //(0x194)(Pokemon F Nickname)
        else if(transferredByteCounterA == 0x194){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyA = true;
          //Disable transmission.
          enableTransmissionA = false;
        }
      }
      //GAMEBOY B(JPN)
      else if(activeLinkStateIndex == &activeLinkStateIndexB && !translationPointReachedForGameboyB){
        //(0x116, 0x11C, 0x122, 0x128, 12E, 134) (OT Names)
        if(transferredByteCounterB == 0x116 || transferredByteCounterB == 0x11C || transferredByteCounterB == 0x122 || transferredByteCounterB == 0x128 || transferredByteCounterB == 0x12E || transferredByteCounterB == 0x134){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyB = true;
          //Disable transmission.
          enableTransmissionB = false;
        }
        //(0x13A)(Pokemon A Nickname)
        else if(transferredByteCounterB == 0x13A){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyB = true;
          //Disable transmission.
          enableTransmissionB = false;
        }
        //(0x140)(Pokemon B Nickname)
        else if(transferredByteCounterB == 0x140){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyB = true;
          //Disable transmission.
          enableTransmissionB = false;
        }
        //(0x146)(Pokemon C Nickname)
        else if(transferredByteCounterB == 0x146){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyB = true;
          //Disable transmission.
          enableTransmissionB = false;
        }
        //(0x14C)(Pokemon D Nickname)
        else if(transferredByteCounterB == 0x14C){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyB = true;
          //Disable transmission.
          enableTransmissionB = false;
        }
        //(0x152)(Pokemon E Nickname)
        else if(transferredByteCounterB == 0x152){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyB = true;
          //Disable transmission.
          enableTransmissionB = false;
        }
        //(0x158)(Pokemon F Nickname)
        else if(transferredByteCounterB == 0x158){
          //Mark the translation point as having been reached.
          translationPointReachedForGameboyB = true;
          //Disable transmission.
          enableTransmissionB = false;
        }
      }
      //Set the next byte that needs to be sent.
      if(activeLinkStateIndex == &activeLinkStateIndexA){
        //Proceed if data needs to be read from the translated buffers.
        if(translatedIncomingBufferALength > 0){
          linkOutgoingByteA = translatedIncomingBufferDataA[translatedIncomingBufferAPosition];
          //Increment the position to the next byte within the buffer.
          translatedIncomingBufferAPosition++;
          //If all of the bytes have been read, disable reading from the buffer.
          if(translatedIncomingBufferAPosition >= translatedIncomingBufferALength){
            translatedIncomingBufferAPosition = 0;
            translatedIncomingBufferALength = 0;
            enableTransmissionBAfterPulse = true;
          }
        }else{
          //Set the outgoing bytes to exchange the data from each gameboy.
          linkOutgoingByteA = linkIncomingByteB;
        }
      }else{
        //Proceed if data needs to be read from the translated buffers.
        if(translatedIncomingBufferBLength > 0){
          linkOutgoingByteB = translatedIncomingBufferDataB[translatedIncomingBufferBPosition];
          //Increment the position to the next byte within the buffer.
          translatedIncomingBufferBPosition++;
          //If all of the bytes have been read, disable reading from the buffer.
          if(translatedIncomingBufferBPosition >= translatedIncomingBufferBLength){
            translatedIncomingBufferBPosition = 0;
            translatedIncomingBufferBLength = 0;
            disableTransmissionBAfterPulse = true;
          }
        }else{
          //Set the outgoing bytes to exchange the data from each gameboy.
          linkOutgoingByteB = linkIncomingByteA;
        }
      }

      //Debugging.
      if(enableDebugOutput){
        if(linkStates[(*activeLinkStateIndex)].bufferIndex == 0){
          if(activeLinkStateIndex == &activeLinkStateIndexA){
            sprintf(charBuffer, "(A) Byte at %d: %02X\n", transferredByteCounterA, linkIncomingByteA);
            Serial.print(charBuffer);
          }else{
            sprintf(charBuffer, "(B) Byte at %d: %02X\n", transferredByteCounterB, linkIncomingByteB);
            Serial.print(charBuffer);
          }
        }else if(linkStates[(*activeLinkStateIndex)].bufferIndex == 1){
          if(activeLinkStateIndex == &activeLinkStateIndexA && transferredByteCounterA < 30){
            sprintf(charBuffer, "(A) Byte at %d: %02X\n", transferredByteCounterA, linkIncomingByteA);
            Serial.print(charBuffer);
          }else{
            if(transferredByteCounterB < 30){
              sprintf(charBuffer, "(B) Byte at %d: %02X\n", transferredByteCounterB, linkIncomingByteB);
              Serial.print(charBuffer);
            }
          }
        }
      }
      //Increment the amount of bytes exchanged.
      if(activeLinkStateIndex == &activeLinkStateIndexA){
        //Only increment if the translation point hasn't been reached.
        if(!translationPointReachedForGameboyA){
          transferredByteCounterA++;
        }
      }else{
        //Only increment if the translation point hasn't been reached.
        if(!translationPointReachedForGameboyB){
          transferredByteCounterB++;
        }
      }
      //Obtain the buffer size and byte counter to compare against.
      unsigned short bufferSize = 0;
      unsigned short transferredByteCounter = 0;
      if(activeLinkStateIndex == &activeLinkStateIndexA){
        bufferSize = linkStates[(*activeLinkStateIndex)].bufferASize;
        transferredByteCounter = transferredByteCounterA;
      }else{
        bufferSize = linkStates[(*activeLinkStateIndex)].bufferBSize;
        transferredByteCounter = transferredByteCounterB;
      }
      //Once all of the bytes have been exchanged, proceed to the next state.
      if(transferredByteCounter >= bufferSize){
        //If another state exists, proceed.
        if((*activeLinkStateIndex) < STATE_COUNT - 1){
          //If the state temporarily disables transmission, apply it.
          DelayTransmissionBasedOnActiveState(activeLinkStateIndex);
          //Increment the state index.
          (*activeLinkStateIndex)++;
          //Reset the counters.
          ResetStateByteCounters(activeLinkStateIndex);
          //Debugging.
          OutputStateChangeDebugString(activeLinkStateIndex);
        }
      }
    }
    //Proceed if this is an update for Gameboy B.
    if(activeLinkStateIndex == &activeLinkStateIndexB){
      //Proceed if both gameboys have reached the translation points.
      if(translationPointReachedForGameboyA && translationPointReachedForGameboyB){
        if(enableDebugOutput){
          sprintf(charBuffer, "Translation Point Reached!: %02X|%02X\n", transferredByteCounterA, transferredByteCounterB);
          Serial.print(charBuffer);
        }
        //Disable the flags.
        translationPointReachedForGameboyA = false;
        translationPointReachedForGameboyB = false;
        //Re-enable the transmission.
        enableTransmissionA = true;
        enableTransmissionB = true;
        //Set the data that needs to be sent.
        //GAMEBOY A
        //(0x11B, 0x126, 0x131, 0x13C, 0x147, 0x152) (OT Names)
        if(transferredByteCounterA == 0x11B || transferredByteCounterA == 0x126 || transferredByteCounterA == 0x131 || transferredByteCounterA == 0x13C || transferredByteCounterA == 0x147 || transferredByteCounterA == 0x152){
          //Make sure the index is within bounds.
          if(gameboyACurrentPokemonTrainerIDIndex < gameboyAPartyData[0]){
            //Obtain the trainer ID of the pokemon.
            unsigned short trainerIDUpperByte = gameboyAPokemonTrainerIDs[(gameboyACurrentPokemonTrainerIDIndex * 2)];
            unsigned short trainerIDLowerByte = gameboyAPokemonTrainerIDs[(gameboyACurrentPokemonTrainerIDIndex * 2) + 1];
            unsigned short trainerID = (trainerIDUpperByte << 8) | trainerIDLowerByte;
            if(enableDebugOutput){
              Serial.print("(A) ");
              Serial.println(trainerID);
            }
            //Obtain the index to the trainer.
            signed int pokemonOTTrainerIndex = getTrainerIndexFromID(trainerID);
            if(pokemonOTTrainerIndex != -1){
              //ENGLISH TRAINER NAME IN JAPANESE
              for(signed int i = 0; i < 6; i++){
                translatedIncomingBufferDataB[i] = gameboyAToBEncodedTrainerNames[pokemonOTTrainerIndex][i];
              }
              translatedIncomingBufferBLength = 6;
              translatedIncomingBufferBPosition = 1;
            }
            //Increment the index.
            gameboyACurrentPokemonTrainerIDIndex++;
          }else{
            //BLANK DATA
            for(signed int i = 0; i < 6; i++){
              translatedIncomingBufferDataB[i] = 0;
            }
            translatedIncomingBufferBLength = 6;
            translatedIncomingBufferBPosition = 1;
          }
        }
        //(0x15D)(Pokemon A Nickname)
        else if(transferredByteCounterA == 0x15D){
          //Proceed if the Species ID is valid.
          if(gameboyAPartyData[1] >= 0 && gameboyAPartyData[1] <= 190){
            signed short pokemonDexID = speciesIDToPokedexNumberArray[gameboyAPartyData[1]];
            if(pokemonDexID != -1){
              signed short nameDataOffset = (pokemonDexID - 1) * 11;
              for(signed int i = 0; i < 6; i++){
                translatedIncomingBufferDataB[i] = pgm_read_byte_near(japanesePokemonNameData + nameDataOffset + i);
              }
              translatedIncomingBufferBLength = 6;
              translatedIncomingBufferBPosition = 1;
            }
          }
        }
        //(0x168)(Pokemon B Nickname)
        else if(transferredByteCounterA == 0x168){
          //Make sure the index is within bounds.
          if(gameboyAPartyData[0] > 1){
            //Proceed if the Species ID is valid.
            if(gameboyAPartyData[2] >= 0 && gameboyAPartyData[2] <= 190){
              signed short pokemonDexID = speciesIDToPokedexNumberArray[gameboyAPartyData[2]];
              if(pokemonDexID != -1){
                signed short nameDataOffset = (pokemonDexID - 1) * 11;
                for(signed int i = 0; i < 6; i++){
                  translatedIncomingBufferDataB[i] = pgm_read_byte_near(japanesePokemonNameData + nameDataOffset + i);
                }
                translatedIncomingBufferBLength = 6;
                translatedIncomingBufferBPosition = 1;
              }
            }
          }else{
            //BLANK DATA
            for(signed int i = 0; i < 6; i++){
              translatedIncomingBufferDataB[i] = 0;
            }
            translatedIncomingBufferBLength = 6;
            translatedIncomingBufferBPosition = 1;
          }
        }
        //(0x173)(Pokemon C Nickname)
        else if(transferredByteCounterA == 0x173){
          //Make sure the index is within bounds.
          if(gameboyAPartyData[0] > 2){
            //Proceed if the Species ID is valid.
            if(gameboyAPartyData[3] >= 0 && gameboyAPartyData[3] <= 190){
              signed short pokemonDexID = speciesIDToPokedexNumberArray[gameboyAPartyData[3]];
              if(pokemonDexID != -1){
                signed short nameDataOffset = (pokemonDexID - 1) * 11;
                for(signed int i = 0; i < 6; i++){
                  translatedIncomingBufferDataB[i] = pgm_read_byte_near(japanesePokemonNameData + nameDataOffset + i);
                }
                translatedIncomingBufferBLength = 6;
                translatedIncomingBufferBPosition = 1;
              }
            }
          }else{
            //BLANK DATA
            for(signed int i = 0; i < 6; i++){
              translatedIncomingBufferDataB[i] = 0;
            }
            translatedIncomingBufferBLength = 6;
            translatedIncomingBufferBPosition = 1;
          }
        }
        //(0x17E)(Pokemon D Nickname)
        else if(transferredByteCounterA == 0x17E){
          //Make sure the index is within bounds.
          if(gameboyAPartyData[0] > 3){
            //Proceed if the Species ID is valid.
            if(gameboyAPartyData[4] >= 0 && gameboyAPartyData[4] <= 190){
              signed short pokemonDexID = speciesIDToPokedexNumberArray[gameboyAPartyData[4]];
              if(pokemonDexID != -1){
                signed short nameDataOffset = (pokemonDexID - 1) * 11;
                for(signed int i = 0; i < 6; i++){
                  translatedIncomingBufferDataB[i] = pgm_read_byte_near(japanesePokemonNameData + nameDataOffset + i);
                }
                translatedIncomingBufferBLength = 6;
                translatedIncomingBufferBPosition = 1;
              }
            }
          }else{
            //BLANK DATA
            for(signed int i = 0; i < 6; i++){
              translatedIncomingBufferDataB[i] = 0;
            }
            translatedIncomingBufferBLength = 6;
            translatedIncomingBufferBPosition = 1;
          }
        }
        //(0x189)(Pokemon E Nickname)
        else if(transferredByteCounterA == 0x189){
          //Make sure the index is within bounds.
          if(gameboyAPartyData[0] > 4){
            //Proceed if the Species ID is valid.
            if(gameboyAPartyData[5] >= 0 && gameboyAPartyData[5] <= 190){
              signed short pokemonDexID = speciesIDToPokedexNumberArray[gameboyAPartyData[5]];
              if(pokemonDexID != -1){
                signed short nameDataOffset = (pokemonDexID - 1) * 11;
                for(signed int i = 0; i < 6; i++){
                  translatedIncomingBufferDataB[i] = pgm_read_byte_near(japanesePokemonNameData + nameDataOffset + i);
                }
                translatedIncomingBufferBLength = 6;
                translatedIncomingBufferBPosition = 1;
              }
            }
          }else{
            //BLANK DATA
            for(signed int i = 0; i < 6; i++){
              translatedIncomingBufferDataB[i] = 0;
            }
            translatedIncomingBufferBLength = 6;
            translatedIncomingBufferBPosition = 1;
          }
        }
        //(0x194)(Pokemon F Nickname)
        else if(transferredByteCounterA == 0x194){
          //Make sure the index is within bounds.
          if(gameboyAPartyData[0] > 5){
            //Proceed if the Species ID is valid.
            if(gameboyAPartyData[6] >= 0 && gameboyAPartyData[6] <= 190){
              signed short pokemonDexID = speciesIDToPokedexNumberArray[gameboyAPartyData[6]];
              if(pokemonDexID != -1){
                signed short nameDataOffset = (pokemonDexID - 1) * 11;
                for(signed int i = 0; i < 6; i++){
                  translatedIncomingBufferDataB[i] = pgm_read_byte_near(japanesePokemonNameData + nameDataOffset + i);
                }
                translatedIncomingBufferBLength = 6;
                translatedIncomingBufferBPosition = 1;
              }
            }
          }else{
            //BLANK DATA
            for(signed int i = 0; i < 6; i++){
              translatedIncomingBufferDataB[i] = 0;
            }
            translatedIncomingBufferBLength = 6;
            translatedIncomingBufferBPosition = 1;
          }
        }
        //GAMEBOY B
        //(0x116, 0x11C, 0x122, 0x128, 12E, 134) (OT Names)
        if(transferredByteCounterB == 0x116 || transferredByteCounterB == 0x11C || transferredByteCounterB == 0x122 || transferredByteCounterB == 0x128 || transferredByteCounterB == 0x12E || transferredByteCounterB == 0x134){
          //Make sure the index is within bounds.
          if(gameboyBCurrentPokemonTrainerIDIndex < gameboyBPartyData[0]){
            //Obtain the trainer ID of the pokemon.
            unsigned short trainerIDUpperByte = gameboyBPokemonTrainerIDs[(gameboyBCurrentPokemonTrainerIDIndex * 2)];
            unsigned short trainerIDLowerByte = gameboyBPokemonTrainerIDs[(gameboyBCurrentPokemonTrainerIDIndex * 2) + 1];
            unsigned short trainerID = (trainerIDUpperByte << 8) | trainerIDLowerByte;
            if(enableDebugOutput){
              Serial.print("(B) ");
              Serial.println(trainerID);
            }
            //Obtain the index to the trainer.
            signed int pokemonOTTrainerIndex = getTrainerIndexFromID(trainerID);
            if(pokemonOTTrainerIndex != -1){
              //JAPANESE TRAINER NAME IN ENGLISH
              for(signed int i = 0; i < 11; i++){
                translatedIncomingBufferDataA[i] = gameboyBToAEncodedTrainerNames[pokemonOTTrainerIndex][i];
              }
              translatedIncomingBufferALength = 11;
              translatedIncomingBufferAPosition = 1;
            }
            //Increment the index.
            gameboyBCurrentPokemonTrainerIDIndex++;
          }else{
            //BLANK DATA
            for(signed int i = 0; i < 11; i++){
              translatedIncomingBufferDataA[i] = 0;
            }
            translatedIncomingBufferALength = 11;
            translatedIncomingBufferAPosition = 1;
          }
        }
        //(0x13A)(Pokemon A Nickname)
        else if(transferredByteCounterB == 0x13A){
          //Proceed if the Species ID is valid.
          if(gameboyBPartyData[1] >= 0 && gameboyBPartyData[1] <= 190){
            signed short pokemonDexID = speciesIDToPokedexNumberArray[gameboyBPartyData[1]];
            if(pokemonDexID != -1){
              signed short nameDataOffset = (pokemonDexID - 1) * 11;
              for(signed int i = 0; i < 11; i++){
                translatedIncomingBufferDataA[i] = pgm_read_byte_near(englishPokemonNameData + nameDataOffset + i);
              }
              translatedIncomingBufferALength = 11;
              translatedIncomingBufferAPosition = 1;
            }
          }
        }
        //(0x140)(Pokemon B Nickname)
        else if(transferredByteCounterB == 0x140){
          //Make sure the index is within bounds.
          if(gameboyBPartyData[0] > 1){
            //Proceed if the Species ID is valid.
            if(gameboyBPartyData[2] >= 0 && gameboyBPartyData[2] <= 190){
              signed short pokemonDexID = speciesIDToPokedexNumberArray[gameboyBPartyData[2]];
              if(pokemonDexID != -1){
                signed short nameDataOffset = (pokemonDexID - 1) * 11;
                for(signed int i = 0; i < 11; i++){
                  translatedIncomingBufferDataA[i] = pgm_read_byte_near(englishPokemonNameData + nameDataOffset + i);
                }
                translatedIncomingBufferALength = 11;
                translatedIncomingBufferAPosition = 1;
              }
            }
          }else{
            //BLANK DATA
            for(signed int i = 0; i < 11; i++){
              translatedIncomingBufferDataA[i] = 0;
            }
            translatedIncomingBufferALength = 11;
            translatedIncomingBufferAPosition = 1;
          }
        }
        //(0x146)(Pokemon C Nickname)
        else if(transferredByteCounterB == 0x146){
          //Make sure the index is within bounds.
          if(gameboyBPartyData[0] > 2){
            //Proceed if the Species ID is valid.
            if(gameboyBPartyData[3] >= 0 && gameboyBPartyData[3] <= 190){
              signed short pokemonDexID = speciesIDToPokedexNumberArray[gameboyBPartyData[3]];
              if(pokemonDexID != -1){
                signed short nameDataOffset = (pokemonDexID - 1) * 11;
                for(signed int i = 0; i < 11; i++){
                  translatedIncomingBufferDataA[i] = pgm_read_byte_near(englishPokemonNameData + nameDataOffset + i);
                }
                translatedIncomingBufferALength = 11;
                translatedIncomingBufferAPosition = 1;
              }
            }
          }else{
            //BLANK DATA
            for(signed int i = 0; i < 11; i++){
              translatedIncomingBufferDataA[i] = 0;
            }
            translatedIncomingBufferALength = 11;
            translatedIncomingBufferAPosition = 1;
          }
        }
        //(0x14C)(Pokemon D Nickname)
        else if(transferredByteCounterB == 0x14C){
          //Make sure the index is within bounds.
          if(gameboyBPartyData[0] > 3){
            //Proceed if the Species ID is valid.
            if(gameboyBPartyData[4] >= 0 && gameboyBPartyData[4] <= 190){
              signed short pokemonDexID = speciesIDToPokedexNumberArray[gameboyBPartyData[4]];
              if(pokemonDexID != -1){
                signed short nameDataOffset = (pokemonDexID - 1) * 11;
                for(signed int i = 0; i < 11; i++){
                  translatedIncomingBufferDataA[i] = pgm_read_byte_near(englishPokemonNameData + nameDataOffset + i);
                }
                translatedIncomingBufferALength = 11;
                translatedIncomingBufferAPosition = 1;
              }
            }
          }else{
            //BLANK DATA
            for(signed int i = 0; i < 11; i++){
              translatedIncomingBufferDataA[i] = 0;
            }
            translatedIncomingBufferALength = 11;
            translatedIncomingBufferAPosition = 1;
          }
        }
        //(0x152)(Pokemon E Nickname)
        else if(transferredByteCounterB == 0x152){
          //Make sure the index is within bounds.
          if(gameboyBPartyData[0] > 4){
            //Proceed if the Species ID is valid.
            if(gameboyBPartyData[5] >= 0 && gameboyBPartyData[5] <= 190){
              signed short pokemonDexID = speciesIDToPokedexNumberArray[gameboyBPartyData[5]];
              if(pokemonDexID != -1){
                signed short nameDataOffset = (pokemonDexID - 1) * 11;
                for(signed int i = 0; i < 11; i++){
                  translatedIncomingBufferDataA[i] = pgm_read_byte_near(englishPokemonNameData + nameDataOffset + i);
                }
                translatedIncomingBufferALength = 11;
                translatedIncomingBufferAPosition = 1;
              }
            }
          }else{
            //BLANK DATA
            for(signed int i = 0; i < 11; i++){
              translatedIncomingBufferDataA[i] = 0;
            }
            translatedIncomingBufferALength = 11;
            translatedIncomingBufferAPosition = 1;
          }
        }
        //(0x158)(Pokemon F Nickname)
        else if(transferredByteCounterB == 0x158){
          //Make sure the index is within bounds.
          if(gameboyBPartyData[0] > 5){
            //Proceed if the Species ID is valid.
            if(gameboyBPartyData[6] >= 0 && gameboyBPartyData[6] <= 190){
              signed short pokemonDexID = speciesIDToPokedexNumberArray[gameboyBPartyData[6]];
              if(pokemonDexID != -1){
                signed short nameDataOffset = (pokemonDexID - 1) * 11;
                for(signed int i = 0; i < 11; i++){
                  translatedIncomingBufferDataA[i] = pgm_read_byte_near(englishPokemonNameData + nameDataOffset + i);
                }
                translatedIncomingBufferALength = 11;
                translatedIncomingBufferAPosition = 1;
              }
            }
          }else{
            //BLANK DATA
            for(signed int i = 0; i < 11; i++){
              translatedIncomingBufferDataA[i] = 0;
            }
            translatedIncomingBufferALength = 11;
            translatedIncomingBufferAPosition = 1;
          }
        }
        //Set the outgoing bytes to the first byte of the buffers.
        linkOutgoingByteA = translatedIncomingBufferDataA[0];
        linkOutgoingByteB = translatedIncomingBufferDataB[0];
        //Increment the byte counters.
        transferredByteCounterA++;
        transferredByteCounterB++;
      }
    }
  }
  //Proceed if this state translates and exchanges data between the Gameboys.
  else if(linkStates[(*activeLinkStateIndex)].stateType == 3){
    //If the state temporarily disables transmission, apply it.
    DelayTransmissionBasedOnActiveState(activeLinkStateIndex);
    //Set the state.
    (*activeLinkStateIndex) = linkStates[(*activeLinkStateIndex)].jumpStateIndex;
    //Reset the counters.
    ResetStateByteCounters(activeLinkStateIndex);
    //Debugging.
    OutputStateChangeDebugString(activeLinkStateIndex);
  }
  //Proceed if this state causes a delay in transmission.
  else if(linkStates[(*activeLinkStateIndex)].stateType == 4){
    //If the state temporarily disables transmission, apply it.
    DelayTransmissionBasedOnActiveState(activeLinkStateIndex);
    //Proceed to the next state.
    (*activeLinkStateIndex)++;
    //Reset the counters.
    ResetStateByteCounters(activeLinkStateIndex);
    //Debugging.
    OutputStateChangeDebugString(activeLinkStateIndex);
  }
  //Allow the player to cancel.
  if((*activeLinkStateIndex) == 16){
    if(activeLinkStateIndex == &activeLinkStateIndexA && linkIncomingByteA == 0x6F){
     //Send the cancel byte back.
      linkOutgoingByteA = 0x6F;
      //Update the active state.
      (*activeLinkStateIndex) = 23;
      //Debugging.
      OutputStateChangeDebugString(activeLinkStateIndex);
      //Reset the target byte counter.
      linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterA = linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCount;
    }else if(activeLinkStateIndex == &activeLinkStateIndexB && linkIncomingByteB == 0x6F){
      //Send the cancel byte back.
      linkOutgoingByteB = 0x6F;
      //Update the active state.
      (*activeLinkStateIndex) = 23;
      //Debugging.
      OutputStateChangeDebugString(activeLinkStateIndex);
      //Reset the target byte counter.
      linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCounterB = linkStates[(*activeLinkStateIndex)].targetByteOccurrenceCount;
    }
  }
}

//Called to reset the byte counters back to their initial values.
void ResetStateByteCounters(unsigned int* activeLinkStateIndexPtr){
  //STATE A
  if(activeLinkStateIndexPtr == &activeLinkStateIndexA){
    linkStates[(*activeLinkStateIndexPtr)].targetByteOccurrenceCounterA = linkStates[(*activeLinkStateIndexPtr)].targetByteOccurrenceCount;
  }
  //STATE B
  else{
    linkStates[(*activeLinkStateIndexPtr)].targetByteOccurrenceCounterB = linkStates[(*activeLinkStateIndexPtr)].targetByteOccurrenceCount;
  }
}

//Called to keep track of state transitions.
void OutputStateChangeDebugString(unsigned int* activeLinkStateIndexPtr){
  if(enableDebugOutput){
    if(activeLinkStateIndexPtr == &activeLinkStateIndexA){
      Serial.print("A Proceeding to State ");
    }else{
      Serial.print("B Proceeding to State ");
    }          
    Serial.println((*activeLinkStateIndexPtr));
  }
}

//Called to delay transmission if the active state calls for it.
void DelayTransmissionBasedOnActiveState(unsigned int* activeLinkStateIndexPtr){
  //If the state temporarily disables transmission, apply it.
  if(linkStates[(*activeLinkStateIndexPtr)].transmissionDelayTime > 0){
    //STATE A
    if(activeLinkStateIndexPtr == &activeLinkStateIndexA){
      if(enableDebugOutput){
        Serial.print("Disabling Transmission with A for ");
        Serial.print(linkStates[(*activeLinkStateIndexPtr)].transmissionDelayTime);
        Serial.println(" milliseconds!");
      }
      enableTransmissionA = false;
      transmissionATemporarilyDisabled = true;
      transmissionATemporaryDisabledTime = millis();
      transmissionATemporaryDisableDuration = linkStates[(*activeLinkStateIndexPtr)].transmissionDelayTime;
    }
    //STATE B
    else{
      if(enableDebugOutput){
        Serial.print("Disabling Transmission with B for ");
        Serial.print(linkStates[(*activeLinkStateIndexPtr)].transmissionDelayTime);
        Serial.println(" milliseconds!");
      }
      enableTransmissionB = false;
      transmissionBTemporarilyDisabled = true;
      transmissionBTemporaryDisabledTime = millis();
      transmissionBTemporaryDisableDuration = linkStates[(*activeLinkStateIndexPtr)].transmissionDelayTime;
    }
  }
}

void loop() {
  //If transmission is disabled, check the state of the button to see if transmission need to start.
  /*if(!enableTransmission){
    if(digitalRead(START_TRANSMISSION_BUTTON_PIN) == HIGH){
      enableTransmission = true;
    }
  }*/
  //Update the LED based on the transmission status.
  if(enableTransmission){
    digitalWrite(LED_BUILTIN, HIGH);
  }else{
    digitalWrite(LED_BUILTIN, LOW);
  }
  //If transmission was temporarily disabled, check to see if it needs to be re-enabled.
  if(transmissionATemporarilyDisabled){
    if(millis() - transmissionATemporaryDisabledTime >= transmissionATemporaryDisableDuration){
      enableTransmissionA = true;
      transmissionATemporarilyDisabled = false;
      if(enableDebugOutput){
        Serial.println("Transmission A has become re-enabled!");
      }
    }
  }
  if(transmissionBTemporarilyDisabled){
    if(millis() - transmissionBTemporaryDisabledTime >= transmissionBTemporaryDisableDuration){
      enableTransmissionB = true;
      transmissionBTemporarilyDisabled = false;
      if(enableDebugOutput){
        Serial.println("Transmission B has become re-enabled!");
      }
    }
  }
  //Only proceed is transmission has been enabled.
  if(enableTransmission){
    //Update the states.
    UpdateStates(&activeLinkStateIndexA);
    UpdateStates(&activeLinkStateIndexB);
    if(enableTransmissionA){
      if(linkOutgoingByteARepeatCounter > 0){
        linkOutgoingByteARepeatCounter--;
        linkOutgoingByteA = lastLinkOutgoingByteA;
      }
    }
    if(enableTransmissionB){
      if(linkOutgoingByteBRepeatCounter > 0){
        linkOutgoingByteBRepeatCounter--;
        linkOutgoingByteB = lastLinkOutgoingByteB;
      }
    }
    //Transmit 8 bits of data to and from both Gameboys.
    for(signed int i = 0; i < 8; i++){
      //Obtain the current uptime of the application, in microseconds.
      time = micros();
      //Make the clock signal go low, where data gets written to the MOSI lines.
      if(enableTransmissionA){
        digitalWrite(DMG_GAMEBOY_A_LINK_CLK, LOW);
      }
      if(enableTransmissionB){
        digitalWrite(DMG_GAMEBOY_B_LINK_CLK, LOW);
      }
      //Set the state of the signals based on the most significant bits of the bytes to send, then shift over by a bit.
      if(enableTransmissionA){
        if(linkOutgoingByteA & 0x80){
          digitalWrite(DMG_GAMEBOY_A_LINK_SIN, HIGH);
        }else{
          digitalWrite(DMG_GAMEBOY_A_LINK_SIN, LOW);
        }
        linkOutgoingByteA = (linkOutgoingByteA << 1);
      }
      if(enableTransmissionB){
        if(linkOutgoingByteB & 0x80){
          digitalWrite(DMG_GAMEBOY_B_LINK_SOUT, HIGH);
        }else{
          digitalWrite(DMG_GAMEBOY_B_LINK_SOUT, LOW);
        }
        linkOutgoingByteB = (linkOutgoingByteB << 1);
      }
      //Sleep for 50 microseconds to give the Gameboy some time to process.
      delayMicroseconds(100);
      //Make the clock signal go high, where data gets read from the MISO lines.
      if(enableTransmissionA){
        digitalWrite(DMG_GAMEBOY_A_LINK_CLK, HIGH);
      }
      if(enableTransmissionB){
        digitalWrite(DMG_GAMEBOY_B_LINK_CLK, HIGH);
      }
      //Read the states of the signals and update the incoming values accordingly.
      if(enableTransmissionA){
        if(digitalRead(DMG_GAMEBOY_A_LINK_SOUT) == HIGH){
          linkIncomingByteA = (linkIncomingByteA << 1) | 0x1;
        }else{
          linkIncomingByteA = (linkIncomingByteA << 1);
        }
      }
      if(enableTransmissionB){
        if(digitalRead(DMG_GAMEBOY_B_LINK_SIN) == HIGH){
          linkIncomingByteB = (linkIncomingByteB << 1) | 0x1;
        }else{
          linkIncomingByteB = (linkIncomingByteB << 1);
        }
      }
      //Sleep for 50 microseconds to give the Gameboy some time to process.
      delayMicroseconds(100);
    }
    digitalWrite(DMG_GAMEBOY_A_LINK_SIN, LOW);
    digitalWrite(DMG_GAMEBOY_B_LINK_SOUT, LOW);
    //Disable/Enable certain transmissions based on the flags.
    if(enableTransmissionAAfterPulse){
      enableTransmissionA = true;
      enableTransmissionAAfterPulse = false;
    }
    if(enableTransmissionBAfterPulse){
      enableTransmissionB = true;
      enableTransmissionBAfterPulse = false;
    }
    if(disableTransmissionAAfterPulse){
      enableTransmissionA = false;
      disableTransmissionAAfterPulse = false;
    }
    if(disableTransmissionBAfterPulse){
      enableTransmissionB = false;
      disableTransmissionBAfterPulse = false;
    }
    //Wait 15 milliseconds to give the Gameboy some time to process.
    if(linkStates[activeLinkStateIndexA].stateType != 2){
      delay(15);
    }
    else{
      //Wait to give the Gameboy some time to process.
      delayMicroseconds(450);
    }
  }
}
