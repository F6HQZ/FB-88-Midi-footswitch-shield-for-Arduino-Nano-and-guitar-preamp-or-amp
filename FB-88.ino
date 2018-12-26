/*
* FB-88 Midi footswitch.
* Arduino nano project.
* This is to propose a pedalboard able to replace the Carvin FS-77, but it's ok for other brands or to add Midi control to amps which have not.
* Need an hardware interface to correctly electricaly isolate and protect the Arduino Nano from the external Carvin Quad-X preamp footswitch connector negative voltage to avoid to destroy the Nano GPIOs.
* You will find my FB-88 Arduino/Nano "shield" (complete board, CMS components installed, soldered, card  :
* https://www.quintium.fr/19-musiciens
*
* V.1.1.0 2018-12-26
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

#include <MIDI.h>
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
int output[9];
int outputState[9];

// to memorise the features status for each channel
int memoOutput5Chan[5];
int memoOutput6Chan[5];
int memoOutput7Chan[5];
int memoOutput8Chan[5];

// Quad switch imputs for Midi address as example
const int MidiAddrSw1 = 10;
const int MidiAddrSw2 = 11;
const int MidiAddrSw3 = 12;
const int MidiAddrSw4 = 13;

// debouncing section :
unsigned long debounceDelay = 40; // debounce time; increase if some issues with switch debounces and output flickers
int actionDuration = 40; // contact duration in msec (if 0 = infinite, until switch status change)

int activeChan = 1; // what channel is selected at power on, change as you want

//unsigned int midiChan = 4; // Midi default channel, change as you want or let the quad-switches select it bellow
int midiChan = 4; // Midi default channel, change as you want or let the quad-switches select it bellow

// Channels Radio Buttons temporary ON check        
void checkRadioButton(int button, int reading) {  
  if (reading != lastButtonState[button]) {  // Button state change     
    if (lastButtonState[button] == HIGH) {    // Button was OFF
      if ((reading == LOW) && (lastButtonState[button] == HIGH)) {    // Button pushed AND was not before, then toggle output status
        digitalWrite(output[button], !(outputState[button]));
        outputState[button] = !(outputState[button]);
        lastButtonState[button] = LOW;        // button pushed ON   
        switchChan(button);     
        // delay(debounceDelay); // used for OFF to ON transition only; normaly not needed for a radio group buttons
      } else {
        // delay(debounceDelay); // used for ON to OFF transition only; normaly not needed for a radio group buttons
      }   
    }
    lastButtonState[button] = reading;         // synchronize with the switch status now
    delay(debounceDelay); // for each button status change   
  }
}

void switchChan(int chan) {
  int loop = 0;
  for (; loop < 5; loop++) {
    digitalWrite(output[loop], LOW); // all channels OFF
  }
  digitalWrite(output[chan], HIGH); // only the selected channel is ON now
  memoOutput5Chan[activeChan] = outputState[5]; // effect status are copied to effect memories for the previously selected chan# for later callback  
  memoOutput6Chan[activeChan] = outputState[6];
  memoOutput7Chan[activeChan] = outputState[7];
  memoOutput8Chan[activeChan] = outputState[8];
  digitalWrite(output[5],memoOutput5Chan[chan]); // read FX status memory and switch the effects outputs
  outputState[5] = memoOutput5Chan[chan];
  digitalWrite(output[6],memoOutput6Chan[chan]);
  outputState[6] = memoOutput6Chan[chan];
  digitalWrite(output[7],memoOutput7Chan[chan]);
  outputState[7] = memoOutput7Chan[chan];
  digitalWrite(output[8],memoOutput8Chan[chan]);
  outputState[8] = memoOutput8Chan[chan];
  activeChan = chan; // as remember or sync
  // Midi output for status copy in a Midi manager display
  midiA.sendProgramChange(chan-1,midiChan);
  midiA.sendControlChange(12,outputState[5],midiChan);
  midiA.sendControlChange(13,outputState[6],midiChan);
  midiA.sendControlChange(14,outputState[7],midiChan);
  midiA.sendControlChange(15,outputState[8],midiChan);
}

// Single button temporary ON type check :
void checkSingleOnOffButton(int button, int reading) {
  if (reading != lastButtonState[button]) {  // Button state change
    if (lastButtonState[button] == HIGH) {    // Button is OFF
      if ((reading == LOW) && (lastButtonState[button] == HIGH)) {    // Button pushed AND was not before then toggle output status
        digitalWrite(output[button], !(outputState[button]));
        outputState[button] = !(outputState[button]);
        lastButtonState[button] = LOW;        // button pushed ON  
        // Midi output for status copy in a Midi manager display
        midiA.sendControlChange(button +7,outputState[button],midiChan);  // CC start from 12 to 15  
        memorisation(activeChan);     
      }
    }
    lastButtonState[button] = reading;         // synchronize with the switch status now
    delay(debounceDelay);
  }
}

void memorisation(int chan) {
  memoOutput5Chan[chan] = outputState[5];
  memoOutput6Chan[chan] = outputState[6];
  memoOutput7Chan[chan] = outputState[7];
  memoOutput8Chan[chan] = outputState[8];
}
  
// -------------------------------------------------------------------------

void setup() {
  button[1] = A0;
  button[2] = A1;
  button[3] = A2;
  button[4] = A3;
  button[5] = A4;
  button[6] = A5;
  button[7] = A6;
  button[8] = A7;
  
  output[1] = 2;
  output[2] = 3;
  output[3] = 4;
  output[4] = 5;
  output[5] = 6;
  output[6] = 7;
  output[7] = 8;
  output[8] = 9;
  
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

  int loop = 0;
  for (; loop < 5; loop++) {
    memoOutput5Chan[loop] = LOW;
    memoOutput6Chan[loop] = LOW;
    memoOutput7Chan[loop] = LOW;
    memoOutput8Chan[loop] = LOW;
  }
  
  // Outputs 
  pinMode(output[1], OUTPUT); 
  pinMode(output[2], OUTPUT); 
  pinMode(output[3], OUTPUT);
  pinMode(output[4], OUTPUT);
  pinMode(output[5], OUTPUT);
  pinMode(output[6], OUTPUT);
  pinMode(output[7], OUTPUT);
  pinMode(output[8], OUTPUT);
  
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
 
  activeChan = 1;

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

  // midiChan = 4; // you can force the Midi channel at what value you want, disregarding the quad-switches positions here, by uncomment this line

  // display the Midi address by blinking effect LEDs
  digitalWrite(output[5], LOW);
  digitalWrite(output[6], LOW);
  digitalWrite(output[7], LOW);
  digitalWrite(output[8], LOW);
  delay(500); 
  loop = 0 ;
  for (; loop < midiChan ; loop++) { 
    digitalWrite(output[5], HIGH);
    digitalWrite(output[6], HIGH);
    digitalWrite(output[7], HIGH);
    digitalWrite(output[8], HIGH);
    delay(100);
    digitalWrite(output[5], LOW);
    digitalWrite(output[6], LOW);
    digitalWrite(output[7], LOW);
    digitalWrite(output[8], LOW);
    delay(100);  
  }

  switchChan(activeChan); // init on the Chan#1

//  MIDI.begin(midiChan); // set the MIDI channel and launch MIDI for the default serial port
  midiA.begin(midiChan); // set the Midi chan for a specific Midi instance named "midiA"
  midiA.turnThruOff(); // or turnThruOn(), toggle on/off midiThru
  
  // Midi output for status copy in a Midi manager display
  midiA.sendProgramChange(activeChan - 1,midiChan);
  midiA.sendControlChange(12,outputState[5],midiChan);
  midiA.sendControlChange(13,outputState[6],midiChan);
  midiA.sendControlChange(14,outputState[7],midiChan);
  midiA.sendControlChange(15,outputState[8],midiChan);
}

//----------------------------------------------------------------------

void loop() {
// These 4 first "radio" buttons are to select what channel is running :

// chan #1    
  int reading = analogRead(button[1]);
  if (reading < 512) {
    reading = LOW;
  } else {
    reading = HIGH;
  }
  checkRadioButton(1, reading);
   
// chan #2  
  reading = analogRead(button[2]);
  if (reading < 512) {
    reading = LOW;
  } else {
    reading = HIGH;
  }
  checkRadioButton(2, reading);
  
// chan #3  
  reading = analogRead(button[3]);
  if (reading < 512) {
    reading = LOW;
  } else {
    reading = HIGH;
  }
  checkRadioButton(3, reading);

// chan #4  
  reading = analogRead(button[4]);
  if (reading < 512) {
    reading = LOW;
  } else {
    reading = HIGH;
  }
  checkRadioButton(4, reading);
  
// These 4 following independant buttons are to select FX, REV, EQU for a Carvin Quad-X, and an OPT for other amp/preamp :

  // button #5 :
   reading = analogRead(button[5]); // read the buttons state
   if (reading < 512) {
     reading = LOW;
   } else {
     reading = HIGH;
   }
   checkSingleOnOffButton(5,reading);

  // button #6 :
   reading = analogRead(button[6]); // read the buttons state
   if (reading < 512) {
     reading = LOW;
   } else {
     reading = HIGH;
   }
   checkSingleOnOffButton(6,reading);
    
  // button #7 :
   reading = analogRead(button[7]); // read the buttons state
   if (reading < 512) {
     reading = LOW;
   } else {
     reading = HIGH;
   }
   checkSingleOnOffButton(7,reading);
 
  // button #8 :
   reading = analogRead(button[8]); // read the buttons state
   if (reading < 512) {
     reading = LOW;
   } else {
     reading = HIGH;
   }
   checkSingleOnOffButton(8,reading);
 
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
            digitalWrite(output[1], HIGH);
            activeChan = 1;
            digitalWrite(output[2], LOW);
            digitalWrite(output[3], LOW);
            digitalWrite(output[4], LOW);
            digitalWrite(output[5], memoOutput5Chan[1]);
            outputState[5] = memoOutput5Chan[1];
            digitalWrite(output[6], memoOutput6Chan[1]);
            outputState[6] = memoOutput6Chan[1];
            digitalWrite(output[7], memoOutput7Chan[1]);
            outputState[7] = memoOutput7Chan[1];
            digitalWrite(output[8],  memoOutput7Chan[1]);
            outputState[8] =  memoOutput8Chan[1];
            break;
          }
          case 1 : {
            digitalWrite(output[1], LOW);
            digitalWrite(output[2], HIGH);
            activeChan = 2;
            digitalWrite(output[3], LOW);
            digitalWrite(output[4], LOW);
            digitalWrite(output[5], memoOutput5Chan[2]);
            outputState[5] = memoOutput5Chan[2];
            digitalWrite(output[6], memoOutput6Chan[2]);
            outputState[6] = memoOutput6Chan[2];
            digitalWrite(output[7], memoOutput7Chan[2]);
            outputState[7] = memoOutput7Chan[2];
            digitalWrite(output[8], memoOutput8Chan[2]);
            outputState[8] = memoOutput8Chan[2];
            break;
          }
          case 2 : {
            digitalWrite(output[1], LOW);
            digitalWrite(output[2], LOW);
            digitalWrite(output[3], HIGH);
            activeChan = 3;
            digitalWrite(output[4], LOW);
            digitalWrite(output[5], memoOutput5Chan[3]);
            outputState[5] = memoOutput5Chan[3];
            digitalWrite(output[6], memoOutput6Chan[3]);
            outputState[6] = memoOutput6Chan[3];
            digitalWrite(output[7], memoOutput7Chan[3]);
            outputState[7] = memoOutput7Chan[3];
            digitalWrite(output[8], memoOutput8Chan[3]);
            outputState[8] = memoOutput8Chan[3];
            break;
          }
          case 3 : {
            digitalWrite(output[1], LOW);
            digitalWrite(output[2], LOW);
            digitalWrite(output[3], LOW);
            digitalWrite(output[4], HIGH);
            activeChan = 4;
            digitalWrite(output[5], memoOutput5Chan[4]);
            outputState[5] = memoOutput5Chan[4];
            digitalWrite(output[6], memoOutput6Chan[4]);
            outputState[6] = memoOutput6Chan[4];
            digitalWrite(output[7], memoOutput7Chan[4]);
            outputState[7] = memoOutput7Chan[4];
            digitalWrite(output[8], memoOutput8Chan[4]);
            outputState[8] = memoOutput8Chan[4];
            break;
          }
          default:
            break;
        }
        case midi::ControlChange:       // If it is a Control Change, do the job here, switch ON/OFF extra feature as REV, FX, EQ...
          switch(midiA.getData1()) {
            case 12 : {                     // an ON/OFF switch feature REV on Carvin Quad-X Amp as example
              switch(midiA.getData2()) {    // value : 0 = OFF and 127 = ON
                case 0 : {
                  digitalWrite(output[5], LOW);
                  outputState[5] = LOW;
                  //lastButtonState[5] = LOW;    
                  memorisation(activeChan);           // record options status for the current channel
                  break;
                }
                case 127 : {
                  digitalWrite(output[5], HIGH);
                  outputState[5] = HIGH;
                  //lastButtonState[5] = LOW;    
                  memorisation(activeChan);           // record options status for the current channel
                  break;
                }
                default:
                  break;
              }
            }
            break;            
            case 13 : {                     // an ON/OFF switch feature REV on Carvin Quad-X Amp as example
              switch(midiA.getData2()) {    // value : 0 = OFF and 127 = ON
                case 0 : {
                  digitalWrite(output[6], LOW);
                  outputState[6] = LOW;
                  //lastButtonState[6] = LOW;    
                  memorisation(activeChan);           // record options status for the current channel
                  break;
                }
                case 127 : {
                  digitalWrite(output[6], HIGH);
                  outputState[6] = HIGH; 
                  //lastButtonState[6] = LOW;    
                  memorisation(activeChan);           // record options status for the current channel
                  break;
                }
                default:
                  break;
              }
              break;
            }                  
            case 14 : {                     // an ON/OFF switch feature REV on Carvin Quad-X Amp as example
              switch(midiA.getData2()) {    // value : 0 = OFF and 127 = ON
                case 0 : {
                  digitalWrite(output[7], LOW);
                  outputState[7] = LOW;    
                  memorisation(activeChan);           // record options status for the current channel
                  break;
                }
                case 127 : {
                  digitalWrite(output[7], HIGH);
                  outputState[7] = HIGH;  
                  memorisation(activeChan);           // record options status for the current channel                  
                  break;
                }
                default:
                  break;
              }
              break;
            }
            case 15 : {                     // a fourth ON/OFF switch feature on another amp as Carvin Quad-X Amp which as only 3 
              switch(midiA.getData2()) {    // value : 0 = OFF and 127 = ON
                case 0 : {
                  digitalWrite(output[8], LOW);
                  outputState[8] = LOW;
                  //lastButtonState[8] = LOW;     
                  // send Midi message to copy status to manager in iPAD as example
                  //midiA.sendControlChange(15,0,midiChan);
                  memorisation(activeChan);           // record options status for the current channel
                  break;
                }
                case 127 : {
                  digitalWrite(output[8], HIGH);
                  outputState[8] = HIGH;
                  //lastButtonState[8] = LOW;
                  // send Midi message to copy status to manager in iPAD as example
                  //midiA.sendControlChange(15,1,midiChan);     
                  memorisation(activeChan);           // record options status for the current channel                  
                  break;
                }
                default:
                  break;
              }
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
