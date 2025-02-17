# pokemon-RBY-trade-translator
A program that allows for trading between English and Japanese Gen 1 Pokemon Games without corruption due to the differing text encodings used within the games. Created for use with an Arduino Uno R3 or Arduino Mega, but could be adapted to work with most modern microcontrollers with slight modifications.

## How It Works
- This program fixes the save corruption that occurs when trading Pokemon between English and Japanese by replacing the Trainer Name and Pokemon OT names with a hard-coded list of bytes set within the program to match the text encoding within that corresponding game, and Pokemon Nicknames with that Pokemon's default name, using the text encoding from that corresponding game. The Arduino serves as the Master of the communication, and exchanges data between each Gameboy. Whenever the Arduino detects that region-specific data is about to be exchanged, it intercepts and sends data to match that game's respective text encoding.  
- Within the program, englishPokemonNameData and japanesePokemonNameData contains the bytes that make up all of the Pokemon Names using the encoding table associated with that language. By modifying these, this script can easily be modified to work with other languages. This does mean that the Pokemon's nickname will be reset, but I believe you can just go to the Nickname Rater to restore the nickname.

## Hardware Needed
- An Arduino Uno R3/Mega (Or Any Other Compatible Microcontroller)  
- A DMG Gameboy Running An English Copy of Pokemon Red, Blue, Or Yellow  
- A DMG Gameboy Running An Japanese Copy of Pokemon Red, Blue, Green, Or Yellow  
- A DMG-O4 Link Cable (Cut in half with the wires exposed, such that they can be plugged into the Arduino.)  

## Software Needed
- Arduino IDE

## Arduino Connections (Gameboy A = Runs English Pokemon Game, Gameboy B = Runs Japanese Pokemon Game)
- Arduino Pin 2 -> (A) DMG-04 Link Cable Clock (Green Wire)  
- Arduino Pin 3 -> (A) DMG-04 Link Cable Serial In (Orange Wire)  
- Arduino Pin 4 -> (A) DMG-04 Link Cable Serial Out (Red Wire)  
- Arduino Pin 5 -> (B) DMG-04 Link Cable CLK (Green Wire)  
- Arduino Pin 6 -> (B) DMG-04 Link Cable Serial In (Orange Wire)  
- Arduino Pin 7 -> (B) DMG-04 Link Cable Serial Out (Red Wire)  
- Arduino GND -> (A) DMG-04 Link Cable Ground  
- Arduino GND -> (B) DMG-04 Link Cable Ground  

## Disclaimer
- I am not responsible for any damage or data loss that may occur while attempting to use this program. Use at your own risk. That being said, you can essentially see all of the data on the Trade screen, so if anything looks off there, cancel the trade. The trade only occurs after both players have confirmed, and thus the SRAM isn't at all touched until you confirm the trade.

## Before Using
Before using the program, a few modifications need to be made:  
- Modify TRAINER_ID_COUNT to the amount of Trainer IDs that will be utilized in the trade. This includes the Trainer ID stored on each cartridge (Which you can see by going to the Pause Menu and viewing your badges), and most importantly, the Trainer IDs associated with each Pokemon that will be traded. This is IMPORTANT.
- Modify gameboyATrainerID and gameboyBTrainerID to be equal to the Trainer IDs of the cartridges on Gameboy's A and B.  
- Modify trainerIDList to contain all of the Trainer IDs that will be utilized within the trade.  
- Modify gameboyAToBEncodedTrainerNames to contain each of the trainers names as they would appear on Gameboy B. Use the Japanese encoding table here [Generation I Character Encoding](https://bulbapedia.bulbagarden.net/wiki/Character_encoding_(Generation_I)) to generate the trainer name you'd like to send to Gameboy B. Order does matter, as each element in gameboyAToBEncodedTrainerNames corresponds to the Trainer IDs within trainerIDList, in that exact order.  
- Modify gameboyBToAEncodedTrainerNames to contain each of the trainers names as they would appear on Gameboy A. Use the English encoding table here [Generation I Character Encoding](https://bulbapedia.bulbagarden.net/wiki/Character_encoding_(Generation_I)) to generate the trainer name you'd like to send to Gameboy A. Order does matter, as each element in gameboyBToAEncodedTrainerNames corresponds to the Trainer IDs within trainerIDList, in that exact order.  
- If you don't modify these variable to match the data on your Gameboys and the Pokemon that are being traded, **THIS WILL NOT WORK**.

## How to Use
1. Upload pokemon-rby-trade-translator.ino to the Arduino via the Arduino IDE. During this step, make sure that the Gameboys are not connected to the Arduino.  
2. On each Gameboy, boot up the game and navigate to the NPC in front of the Trade Room.  
3. Reset the Arduino, either by re-uploading the sketch or by pressing the Reset button on the Arduino.  
4. While the Arduino's built-in LED isn't lit, insert the DMG-04 link cable connectors into Gameboy A and Gameboy B. You have 15 seconds upon the startup of the program to connect the Gameboys to the Arduino.  
5. Wait for the Arduino's built-in LED to light up. This signals the start of the clock signal.  
6. Talk To the Trade Center NPC to kick off the trade sequence. From this point forward, the trade communication should mimic the functionality as if the Gameboy consoles were directly connected by a link cable.  
7. If any hanging occurs, disconnect and restart each Gameboy, and then return to Step 3.  

## Known Issues
- Sometimes, the communication will hang during certain steps of the trade process. This is seemingly random, so whenever it occurs, disconnect and reset each Gameboy and try again.

