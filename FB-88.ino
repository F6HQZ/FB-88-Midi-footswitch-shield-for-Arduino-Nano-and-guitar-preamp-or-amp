/*
* FB-88 Midi footswitch.
* Arduino nano project.
* 
* This is to propose a pedalboard able to replace the Carvin FS-77, but it's ok for other brands or to add Midi control to amps which have not.
* Need an hardware interface to correctly electricaly isolate and protect the Arduino Nano from the external Carvin Quad-X preamp footswitch connector negative voltage to avoid to destroy the Nano GPIOs.
* You will find my FB-88 Arduino/Nano "shield" (complete board, CMS components installed, soldered, card  :
* https://www.quintium.fr/19-musiciens
* 
* This source code is available at : 
* https://github.com/F6HQZ/FB-88-Midi-footswitch-shield-for-Arduino-Nano-and-guitar-preamp-or-amp
*
* V.1.2.5 2019-02-10
*
* created 15/09/2018
* by F6HQZ Francois BERGERET
* protected under Licence GNU Lesser General Public License (see below).
*/

/*=======================================================================\
|     - Copyright (c) - 2018 September - F6HQZ - Francois BERGERET -     |                                       |
|                                                                        |
| My library permits to create a appliance Midi compatible able to be    |
| used as a footswitch, replacing a proprietary one, adding more         |
| possiblities, or to upgrade an old equipment by adding it Midi         |
| protocol.                                                              |
|                                                                        |
| Thanks to friends who have supported me for this project and all guys  |
| who have shared their own codes with the community.                    |
|                                                                        |
| MIDI library from Francois Best (see the MIDI repository and licence). |
|                                                                        |
| Always exploring new technologies, curious about any new idea or       |
| solution, while respecting and thanking the work of alumni who have    |
| gone before us.                                                        |
|                                                                        |
| Enjoy !                                                                |
|                                                                        |
| Feedback for bugs or ameliorations to f6hqz-m@hamwlan.net, thank you ! |
\=======================================================================*/
 
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 3 of the 
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; 
 * if not, see <http://www.gnu.org/licenses/>.
 */

#include <MIDI.h> // you know for what it is !
#include <EEPROM.h> // to store/save values before switch off and reload them at power ON

// the starting address in the EEPROM to write the first byte
int addr = 0;

/*
#include <SoftwareSerial.h>
SoftwareSerial softSerial(2,3);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial,     midiA);
MIDI_CREATE_INSTANCE(SoftwareSerial, softSerial, midiB);
*/
// MIDI_CREATE_DEFAULT_INSTANCE();  // create a default serial port instance
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, midiA); // create an instance named midiA, for further concurrent instances on separated serial ports

// buttons (button[0] not used)
int button[9];
int lastButtonState[9];

// outputs (output[0] not used)
int output[9]; // physical GPIO lines declaration for hardware abstraction
int outputState[9]; // values : 0 = OFF and >0 = ON for swithcboard lines, 0 to 127 for the Midi values

// to memorise the features status for each channel in a 2 dimensions variable array
// the first dimension is the feature and the second the channel status
// ex: memOutputChan[1][2] is accessory #1 status (reverb) for the channel #2 
// the int memoOutputChan[x][0] could store which channel must be in use at starting 
int memoOutputChan[5][5]; // first variable is the FX channel, second variable is the amp channel

// Quad switch imputs for Midi address
const int MidiAddrSw1 = 10;
const int MidiAddrSw2 = 11;
const int MidiAddrSw3 = 12;
const int MidiAddrSw4 = 13;

// debouncing section :
unsigned long debounceDelay = 40; // debounce time; increase if some issues with switch debounces and output flickers

// permanent status or temporary pulses ouputs
int actionDuration = 40; // contact duration in msec (if 0 = infinite, until switch status change)

// what channel is selected at power on, change as you want
int activeChan = 1; 

//unsigned int midiChan = 4; // Midi default channel, change as you want or let the quad-switches select it bellow
int midiChan = 4; // Midi default channel, change as you want or let the quad-switches select it bellow

// Channels Radio Buttons temporary ON check        
void checkRadioButton(int button, int reading) {  
  if (reading < 512) {
    reading = LOW;
  } else {
    reading = HIGH;
  }          
  // if more than one button is pushed in same time
  // buttons #1 and #4 are pushed simultaneously
  if (lastButtonState[1] == LOW && lastButtonState[4] == LOW) { 
    lastButtonState[1] = HIGH;
    lastButtonState[4] = HIGH;
    backup();
  } else {  
    // only one button pushed         
    if (reading != lastButtonState[button]) {  // Button state change     
      if (lastButtonState[button] == HIGH) {    // Button was OFF
        if ((reading == LOW) && (lastButtonState[button] == HIGH)) {    // Button pushed ON AND was not before, then toggle output status
          outputState[button] = !(outputState[button]);
          lastButtonState[button] = LOW;        // button pushed ON   
          switchChan(button);
        }   
      }
      lastButtonState[button] = reading;         // synchronize with the switch status now
      delay(debounceDelay); // for each button status change   
    }
  }
}

void switchChan(int chan) {
  int loop = 1;
  for (; loop < 5; loop++) {
    digitalWrite(output[loop], LOW); // all channels OFF
  }
  digitalWrite(output[chan], HIGH); // only the selected channel is ON now
  
  memorisation(activeChan); // actual effect status are copied to effect memories for the previously selected chan# for later callback
  fxReload(chan);  // read FX status memory and switch the effects outputs
  activeChan = chan; // to re-sync
  
  // Midi output for status copy in a Midi manager display
  midiA.sendProgramChange(chan-1,midiChan);
  midiA.sendControlChange(12,outputState[5],midiChan);
  midiA.sendControlChange(13,outputState[6],midiChan);
  midiA.sendControlChange(14,outputState[7],midiChan);
  midiA.sendControlChange(15,outputState[8],midiChan);

  // send Midi messages to external Midi equipments
  MidiMsgSend (chan); 
}

// read FX status memory and switch the effects outputs
void fxReload(int chan) { 
  int loop = 1;
  for (; loop < 5; loop++) {
    digitalWrite(output[loop +4], memoOutputChan[loop][chan]);
    outputState[loop +4] = memoOutputChan[loop][chan];
  }
}

// Single button temporary ON type check :
void checkSingleOnOffButton(int button, int reading) {
  if (reading < 512) {
    reading = LOW;
  } else {
    reading = HIGH;
  }
  // if more than one button is pushed in same time
  // buttons #5 and #8 are pushed simultaneously
  if (lastButtonState[5] == LOW && lastButtonState[8] == LOW) { 
    lastButtonState[5] = HIGH;
    lastButtonState[8] = HIGH;
    //backup(); or any other insteresting needed action there
  } else {
    // only one button pushed
    if (reading != lastButtonState[button]) {  // Button state change
      if (lastButtonState[button] == HIGH) {    // Button is OFF
        if ((reading == LOW) && (lastButtonState[button] == HIGH)) {    // Button pushed AND was not before then toggle output status       
          outputState[button] = !(outputState[button]);
          lastButtonState[button] = LOW;    // button pushed         
          if (outputState[button] > 0) {
            outputState[button] = 127;      // OFF = 0 and ON = 127 for Midi compliancy
          }  
          switchFX(button,outputState[button]);    
        }
      }
      lastButtonState[button] = reading;         // synchronize with the switch status now
      delay(debounceDelay);
    }
  }
}

void switchFX(int button,int value) {     // button pushed and what value for Midi
  digitalWrite(output[button], value);    // enlight the local LED and trigger the footswitch line to AMP 

  memorisation(activeChan); // actual effect status are copied to effect memories for the previously selected chan# for later callback

  outputState[button] = value;
  memoOutputChan[button -4][activeChan] = value; 
  
  // Midi output for status copy in a Midi manager display
  midiA.sendControlChange(button +7, value, midiChan);  // CC start from 12 to 15. Used for buttons and outputs status replication on Midi manager software in iPad, iPhone or computer. 0 = OFF ; 127 = ON

  // send Midi messages to external Midi equipments depending what button actionned
  MidiMsgSend(button);
}

// Here come the Midi instructions list sent to external Midi equipments for any button action
void MidiMsgSend (int button) {
  switch (button) {
    case 1 : {
      if (outputState[1] == HIGH) {    // "ON"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(0,3);
        midiA.sendProgramChange(0,4);
      } else {                              // "OFF"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(0,3);
        midiA.sendProgramChange(0,4);
      }
      break;
    }
    case 2 : {
      if (outputState[2] == HIGH) {    // "ON"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(1,3);
        midiA.sendProgramChange(1,4);
      } else {                              // "OFF"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(1,3);
        midiA.sendProgramChange(1,4);
      }
      break;
    }
    case 3 : {
      if (outputState[3] == HIGH) {    // "ON"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(2,3);
        midiA.sendProgramChange(2,4);
      } else {                              // "OFF"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(2,3);
        midiA.sendProgramChange(2,4);
      }
      break;
    }
    case 4 : {
      if (outputState[4] == HIGH) {    // "ON"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(3,3);
        midiA.sendProgramChange(3,4);
      } else {                              // "OFF"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(3,3);
        midiA.sendProgramChange(3,4);
      }
      break;
    }
    case 5 : {
      if (outputState[5] == 127) {    // "ON"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(5,3);
      } else {                              // "OFF"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(0,3);
      }
      break;
    }
    case 6 : {
      if (outputState[6] == 127) {    // "ON"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(6,3);
      } else {                              // "OFF"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(0,3);
      }
      break;
    }
    case 7 : {
      if (outputState[7] == 127) {    // "ON"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(7,3);
      } else {                              // "OFF"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(0,3);
      }
      break;
    }
    case 8 : {
      if (outputState[8] == 127) {    // "ON"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(8,3);
      } else {                              // "OFF"
        //midiA.sendProgramChange(3,4);     // PC control to switch to chan# 3; engine ID listening on Midi Chan #4
        //midiA.sendControlChange(12,0,3);  // CC control #12, level 0, engine ID listening on  Midi Chan #3
        midiA.sendProgramChange(0,3);
      }
      break;
    }
    default:
      break;
  }
}

// Analog voltage or resistor variation check (expression/volume pedal) :
void checkAnalogDeviceInput(int input, int reading) {
  // Convert the analog reading (which goes from 0 - 1023) to level between 0 and 127 as requested for a Control Change Midi use
  int controlChangeValue =  reading / 8; // result is a value from 0 to 127 ; 0 = shortcut ; 127 = open circuit
  if (controlChangeValue != outputState[input]) {  // voltage level at input is changed  
    outputState[input] = controlChangeValue; // CC value, from 0 to 127
    lastButtonState[input] = reading;
    // Midi output for status copy in a Midi manager display
    midiA.sendControlChange(input +7,controlChangeValue,midiChan);  // CC start from 12 to 15   
    
    // from here, instruction to an external Midi equipment, sent by each of 8 possible inputs
    switch (input) {
      case 1 : {
        //midiA.sendControlChange(12,controlChangeValue,3);  // CC control #12, level 0, target engine listening on  Midi Chan #3
        break;
      }
      case 2 : {
        //midiA.sendControlChange(12,controlChangeValue,3);  // CC control #12, level 0, target engine listening on  Midi Chan #3
        break;
      }
      case 3 : {
        //midiA.sendControlChange(12,controlChangeValue,3);  // CC control #12, level 0, target engine listening on  Midi Chan #3
        break;
      }
      case 4 : {
        //midiA.sendControlChange(12,controlChangeValue,3);  // CC control #12, level 0, target engine listening on  Midi Chan #3
        break;
      }
      case 5 : {
        //midiA.sendControlChange(12,controlChangeValue,3);  // CC control #12, level 0, target engine listening on  Midi Chan #3
        break;
      }
      case 6 : {
        //midiA.sendControlChange(12,controlChangeValue,3);  // CC control #12, level 0, target engine listening on  Midi Chan #3
        break;
      }
      case 7 : {
        //midiA.sendControlChange(12,controlChangeValue,3);  // CC control #12, level 0, target engine listening on  Midi Chan #3
        break;
      }
      case 8 : {
        //midiA.sendControlChange(12,controlChangeValue,3);  // CC control #12, level 0, target engine listening on  Midi Chan #3
        break;
      }
      default:
        break;
    }
    
    // set the brightness of a PWM compatible output pin
    analogWrite(output[input], controlChangeValue * 2 ); // 0 to 255 LED brightness
    //delay(debounceDelay);
  }
}

void memorisation(int chan) {
  int loop = 1;
  for (; loop < 5; loop++) {
    memoOutputChan[loop][chan] = outputState[loop +4]; // effect status are copied to effect memories for the previously selected chan# for later callback  
  }
}

void backup() {
  // record all footswitch status to EEPROM to save them if power off
  EEPROM.write(addr, activeChan);
  int eepromAddr = addr;
  int loop2 = 1;
  for (; loop2 < 5; loop2++) {
    int loop3 = 1;
    for (; loop3 < 5; loop3++) {
      eepromAddr = eepromAddr +1 ;
      EEPROM.write(eepromAddr,  memoOutputChan[loop3][loop2]);
    }
  }
  // acknowledge by blinking effect LEDs
  digitalWrite(output[5], LOW);
  digitalWrite(output[6], LOW);
  digitalWrite(output[7], LOW);
  digitalWrite(output[8], LOW);
  int loop = 0 ;
  for (; loop < 5 ; loop++) { 
    delay(100);
    digitalWrite(output[5], HIGH);
    digitalWrite(output[6], HIGH);
    digitalWrite(output[7], HIGH);
    digitalWrite(output[8], HIGH);
    delay(100);  
    digitalWrite(output[5], LOW);
    digitalWrite(output[6], LOW);
    digitalWrite(output[7], LOW);
    digitalWrite(output[8], LOW);
  } 
  // switch the output channels to match with the recorded FX status of the activeChan
  recallFxStatus();
  delay(1000);
}

void restore() {
  // restore all footswitch status from EEPROM at restarting after power on
  activeChan = EEPROM.read(addr);
  int eepromAddr = addr;
  int loop2 = 1;
  for (; loop2 < 5; loop2++) {
    int loop3 = 1;
    for (; loop3 < 5; loop3++) {
      eepromAddr = eepromAddr +1 ;
      memoOutputChan[loop3][loop2] = EEPROM.read(eepromAddr);
    }
  }
  // switch the output channels to match with the recorded FX status of the activeChan
  recallFxStatus();
}

void recallFxStatus() {
  // switch the output channels to match with the recorded FX status of the activeChan
  int loop = 1;
  for (; loop < 5; loop++) {
    digitalWrite(output[loop +4],memoOutputChan[loop][activeChan]); // read FX status memory and switch the effects outputs
    outputState[loop +4] = memoOutputChan[loop][activeChan];
  }
}

// -------------------------------------------------------------------------

void setup() {
  // input lines declaration for hardware abstraction
  button[1] = A0;
  button[2] = A1;
  button[3] = A2;
  button[4] = A3;
  button[5] = A4;
  button[6] = A5;
  button[7] = A6;
  button[8] = A7;

  // output lines declaration for hardware abstraction
  // PWM: 3, 5, 6, 9, 10, and 11. Provide 8-bit PWM output with the analogWrite() function level from 0 to 255.
  // You can use them to dim the LED light output to display the analog level of your expression or volume pedal
  // example is with the output pin #9 
  output[1] = 2;
  output[2] = 3;
  output[3] = 4;
  output[4] = 5;
  output[5] = 6;
  output[6] = 7;
  output[7] = 8;
  output[8] = 9;

  // Output status 0 to 254, used for physical outputs (0=OFF and >0=ON) and for Midi statuts output (0 to 127 usualy)
  outputState[1] = LOW;
  outputState[2] = LOW;
  outputState[3] = LOW;
  outputState[4] = LOW;
  outputState[5] = LOW;
  outputState[6] = LOW;
  outputState[7] = LOW;
  outputState[8] = LOW;
  
  lastButtonState[1] = HIGH;
  lastButtonState[2] = HIGH;
  lastButtonState[3] = HIGH;
  lastButtonState[4] = HIGH;
  lastButtonState[5] = HIGH;
  lastButtonState[6] = HIGH;
  lastButtonState[7] = HIGH;
  lastButtonState[8] = HIGH;

  int loop = 1;
  for (; loop < 5; loop++) {
    memoOutputChan[1][loop] = LOW;
    memoOutputChan[2][loop] = LOW;
    memoOutputChan[3][loop] = LOW;
    memoOutputChan[4][loop] = LOW;
  }
  
  // Outputs pinMode, all "OUTPUT"
  loop = 1;
  for (; loop < 9; loop++) {
    pinMode(output[loop], OUTPUT); 
  }
  
  // Inputs for the Midi Address quad-switch 
  pinMode(MidiAddrSw1,INPUT_PULLUP);
  pinMode(MidiAddrSw2,INPUT_PULLUP);
  pinMode(MidiAddrSw3,INPUT_PULLUP);
  pinMode(MidiAddrSw4,INPUT_PULLUP);
  
  // LAS VEGAS :
  digitalWrite(output[1], HIGH);
  delay(200);
  digitalWrite(output[1], LOW);
  digitalWrite(output[2], HIGH);
  delay(200);
  digitalWrite(output[2], LOW);
  digitalWrite(output[3], HIGH);
  delay(200);
  digitalWrite(output[3], LOW);
  digitalWrite(output[4], HIGH);
  delay(200);
  digitalWrite(output[4], LOW);
  digitalWrite(output[5], HIGH);
  delay(200);
  digitalWrite(output[5], LOW);
  digitalWrite(output[6], HIGH);
  delay(200);
  digitalWrite(output[6], LOW);
  digitalWrite(output[7], HIGH);
  delay(200);
  digitalWrite(output[7], LOW);
  digitalWrite(output[8], HIGH);
  delay(200);
  digitalWrite(output[8], LOW);
  digitalWrite(output[1], HIGH);
 
  // activeChan = 1;

// Read the quad switch on the PCB and display the requested Midi channel number with the LEDs # 5, 6, 7, and 8 status (in binary value)
  digitalWrite(output[5], !(digitalRead(MidiAddrSw4)));
  digitalWrite(output[6], !(digitalRead(MidiAddrSw3)));
  digitalWrite(output[7], !(digitalRead(MidiAddrSw2)));
  digitalWrite(output[8], !(digitalRead(MidiAddrSw1)));  
  delay(500); // let 1/2 secondes the Midi channel number displayed 
  
// Set the Midi channel number by reading the quad switch on the PCB
  if (!(digitalRead(MidiAddrSw4)) == LOW) {
    midiChan = midiChan & ~0b1000u; // force to 0 the most significant Midi address bit
  } else {
    midiChan = midiChan | 0b1000u; // force to 1 the most significant Midi address bit
  }
  if (!(digitalRead(MidiAddrSw3)) == LOW) {
    midiChan = midiChan & ~0b0100u;
  } else {
    midiChan = midiChan | 0b0100u;
  }
  if (!(digitalRead(MidiAddrSw2)) == LOW) {
    midiChan = midiChan & ~0b0010u;
  } else {
    midiChan = midiChan | 0b0010u;
  }
  if (!(digitalRead(MidiAddrSw1)) == LOW) {
    midiChan = midiChan & ~0b0001u;
  } else {
    midiChan = midiChan | 0b0001u;
  }

  // midiChan = 4 ; // you can force the Midi channel at what value you want, disregarding the quad-switches positions here, by uncomment this line

  // display the Midi address by blinking effect LEDs
  digitalWrite(output[5], LOW);
  digitalWrite(output[6], LOW);
  digitalWrite(output[7], LOW);
  digitalWrite(output[8], LOW);
  delay(500); 
  loop = 0 ;
  for (; loop < midiChan ; loop++) { 
    int  loop2 = 5 ;
    for (; loop2 < 9 ; loop2++) { 
      digitalWrite(output[loop2], HIGH);
    }    
    delay(200);
    loop2 = 5 ;
    for (; loop2 < 9 ; loop2++) { 
      digitalWrite(output[loop2], LOW);
    }
    delay(200);  
  }

//  MIDI.begin(midiChan); // set the MIDI channel and launch MIDI for the default serial port
  midiA.begin(midiChan); // set the Midi chan for a specific Midi instance named "midiA"
  midiA.turnThruOff(); // or turnThruOn(), toggle on/off midiThru
  restore(); // reload last status set before power off
  switchChan(activeChan); // init on the active Chan#
}

//----------------------------------------------------------------------

void loop() {
// These 4 first "radio" buttons are to select what channel is running :

// chan #1    
  int reading = analogRead(button[1]);
  checkRadioButton(1, reading);
   
// chan #2  
  reading = analogRead(button[2]);
  checkRadioButton(2, reading);
  
// chan #3  
  reading = analogRead(button[3]);
  checkRadioButton(3, reading);

// chan #4  
  reading = analogRead(button[4]);
  checkRadioButton(4, reading);
  
// These 4 following independant buttons are to select FX, REV, EQU for a Carvin Quad-X, and an OPT for other amp/preamp :

  // button #5 :
   reading = analogRead(button[5]); // read the buttons state
   checkSingleOnOffButton(5,reading);

  // button #6 :
   reading = analogRead(button[6]); // read the buttons state
   checkSingleOnOffButton(6,reading);
    
  // button #7 :
   reading = analogRead(button[7]); // read the buttons state
   checkSingleOnOffButton(7,reading);
 
  // button #8 :
   reading = analogRead(button[8]); // read the buttons state
   checkSingleOnOffButton(8,reading);
   // checkAnalogDeviceInput(8,reading);  // this input is connected to an expression or volume pedal
 
  //----------------------------------------------------------------------------
  
  // Midi reception datas processing :

  if (midiA.read()) {  // if we have received a MIDI message
    /*
    // forward the received msg to the midi output as passthrough
        midiA.send(midiA.getType(),
                   midiA.getData1(),
                   midiA.getData2(),
                   midiA.getChannel());
    */
    switch(midiA.getType()) {           // Get the type of the message we caught
      case midi::ProgramChange:         // If it is a Program Change, do the job here, change amp chan and so...
        switch(midiA.getData1()) {
          case 0 : {
            switchChan(1);
            break;
          }
          case 1 : {
            switchChan(2);
            break;
          }
          case 2 : {
            switchChan(3);
            break;
          }
          case 3 : {
            switchChan(4);
            break;
          }
          default:
            break;
        }
        case midi::ControlChange:       // If it is a Control Change, do the job here, switch ON/OFF extra feature as REV, FX, EQ...
          switch(midiA.getData1()) {
            case 12 : {                     // an ON/OFF switch feature REV on Carvin Quad-X Amp as example
              switchFX(5,midiA.getData2());
              break;
            }
            case 13 : {                     // an ON/OFF switch feature REV on Carvin Quad-X Amp as example
              switchFX(6,midiA.getData2());
              break;
            }                  
            case 14 : {                     // an ON/OFF switch feature REV on Carvin Quad-X Amp as example
              switchFX(7,midiA.getData2());
              break;
            }
            case 15 : {                     // a fourth ON/OFF switch feature on another amp as Carvin Quad-X Amp which as only 3 
              switchFX(8,midiA.getData2());
              break;
            }
            default:
              break;
          } 
       default:
         break;
    }
  }

  // delay(1); // slow down the loop for stability if needed
}
