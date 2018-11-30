/*
* FB-88 Midi footswitch.
* Arduino nano project.
* This is to propose a pedalboard able to replace the Carvin FS-77, but it's ok for other brands or to add Midi control to amps which have not.
* Need an hardware interface to correctly electricaly isolate and protect the Arduino Nano from the external Carvin Quad-X preamp footswitch connector negative voltage to avoid to destroy the Nano GPIOs.
* You will find my FB-88 Arduino/Nano "shield" (complete board, CMS components installed, soldered, card full tested by mysel) there :
* https://www.quintium.fr/19-musiciens
*
* V.1.0.1 28-11-2018
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

// constants don't change, used to set pins numbers :
// buttons : analog inputs
const int button1 = A0; 
const int button2 = A1;
const int button3 = A2;
const int button4 = A3;
const int button5 = A4;
const int button6 = A5;
const int button7 = A6;
const int button8 = A7;
// digital outputs
const int output1 = 2; 
const int output2 = 3;
const int output3 = 4;
const int output4 = 5;
const int output5 = 6;
const int output6 = 7;
const int output7 = 8;
const int output8 = 9;
// Quad switch imputs for Midi address as example
const int MidiAddrSw1 = 10;
const int MidiAddrSw2 = 11;
const int MidiAddrSw3 = 12;
const int MidiAddrSw4 = 13;
int output1State = 0; // to store if output is on or off
int output2State = 0;
int output3State = 0;
int output4State = 0;
int output5State = 0;
int output6State = 0;
int output7State = 0;
int output8State = 0;
int button1State = HIGH; // variable to store the button status
int button2State = HIGH;
int button3State = HIGH;
int button4State = HIGH;
int button5State = HIGH;
int button6State = HIGH;
int button7State = HIGH;
int button8State = HIGH;
// debouncing section :
unsigned long debounceDelay = 20; // debounce time; increase if some issues with switch debounces and output flickers
int lastButton1State = HIGH; // previous state for debouncing
int lastButton2State = HIGH;
int lastButton3State = HIGH;
int lastButton4State = HIGH;
int lastButton5State = HIGH;
int lastButton6State = HIGH;
int lastButton7State = HIGH;
int lastButton8State = HIGH;
unsigned long lastDebounceButton1 = 0; // last time the switch has debounced and state changed 
unsigned long lastDebounceButton2 = 0;
unsigned long lastDebounceButton3 = 0;
unsigned long lastDebounceButton4 = 0;
unsigned long lastDebounceButton5 = 0;
unsigned long lastDebounceButton6 = 0;
unsigned long lastDebounceButton7 = 0;
unsigned long lastDebounceButton8 = 0;
// Memories default status at starting. Change as you want.
int memoChan1Output5 = HIGH;
int memoChan1Output6 = LOW;
int memoChan1Output7 = LOW;
int memoChan1Output8 = LOW;
int memoChan2Output5 = LOW;
int memoChan2Output6 = HIGH;
int memoChan2Output7 = LOW;
int memoChan2Output8 = LOW;
int memoChan3Output5 = LOW;
int memoChan3Output6 = HIGH;
int memoChan3Output7 = HIGH;
int memoChan3Output8 = LOW;
int memoChan4Output5 = LOW;
int memoChan4Output6 = HIGH;
int memoChan4Output7 = HIGH;
int memoChan4Output8 = LOW;
int activeChan = 1; // what channel is selected at power on, change as you want
//unsigned int midiChan = 4; // Midi default channel, change as you want or let the quad-switches select it bellow
int midiChan = 4; // Midi default channel, change as you want or let the quad-switches select it bellow

void memorisation() {
  switch (activeChan) {
    case 1 : 
      memoChan1Output5 = output5State;
      memoChan1Output6 = output6State;
      memoChan1Output7 = output7State;
      memoChan1Output8 = output8State;
      break;
    case 2 : 
      memoChan2Output5 = output5State;
      memoChan2Output6 = output6State;
      memoChan2Output7 = output7State;
      memoChan2Output8 = output8State;
      break;
    case 3 : 
      memoChan3Output5 = output5State;
      memoChan3Output6 = output6State;
      memoChan3Output7 = output7State;
      memoChan3Output8 = output8State;
      break;
    case 4 : 
      memoChan4Output5 = output5State;
      memoChan4Output6 = output6State;
      memoChan4Output7 = output7State;
      memoChan4Output8 = output8State;
      break;
  }
}    
        
void switchChan1() {
  digitalWrite(output1, HIGH);
  activeChan = 1;
  digitalWrite(output2, LOW);
  digitalWrite(output3, LOW);
  digitalWrite(output4, LOW);
  digitalWrite(output5, memoChan1Output5);
  output5State = memoChan1Output5;
  digitalWrite(output6, memoChan1Output6);
  output6State = memoChan1Output6;
  digitalWrite(output7, memoChan1Output7);
  output7State = memoChan1Output7;
  digitalWrite(output8, memoChan1Output8);
  output8State = memoChan1Output8;
  // Midi output for status copy in a Midi manager display
  midiA.sendProgramChange(0,midiChan);
  midiA.sendControlChange(12,memoChan1Output5,midiChan);
  midiA.sendControlChange(13,memoChan1Output6,midiChan);
  midiA.sendControlChange(14,memoChan1Output7,midiChan);
  midiA.sendControlChange(15,memoChan1Output8,midiChan);
}

void switchChan2() {
  digitalWrite(output1, LOW);
  digitalWrite(output2, HIGH);
  activeChan = 2;
  digitalWrite(output3, LOW);
  digitalWrite(output4, LOW);
  digitalWrite(output5, memoChan2Output5);
  output5State = memoChan2Output5;
  digitalWrite(output6, memoChan2Output6);
  output6State = memoChan2Output6;
  digitalWrite(output7, memoChan2Output7);
  output7State = memoChan2Output7;
  digitalWrite(output8, memoChan2Output8);
  output8State = memoChan2Output8;
  // Midi output for status copy in a Midi manager display
  midiA.sendProgramChange(1,midiChan);
  midiA.sendControlChange(12,memoChan2Output5,midiChan);
  midiA.sendControlChange(13,memoChan2Output6,midiChan);
  midiA.sendControlChange(14,memoChan2Output7,midiChan);
  midiA.sendControlChange(15,memoChan2Output8,midiChan);
}

void switchChan3() {
  digitalWrite(output1, LOW);
  digitalWrite(output2, LOW);
  digitalWrite(output3, HIGH);
  activeChan = 3;
  digitalWrite(output4, LOW);
  digitalWrite(output5, memoChan3Output5);
  output5State = memoChan3Output5;
  digitalWrite(output6, memoChan3Output6);
  output6State = memoChan3Output6;
  digitalWrite(output7, memoChan3Output7);
  output7State = memoChan3Output7;
  digitalWrite(output8, memoChan3Output8);
  output8State = memoChan3Output8;
  // Midi output for status copy in a Midi manager display
  midiA.sendProgramChange(2,midiChan);
  midiA.sendControlChange(12,memoChan3Output5,midiChan);
  midiA.sendControlChange(13,memoChan3Output6,midiChan);
  midiA.sendControlChange(14,memoChan3Output7,midiChan);
  midiA.sendControlChange(15,memoChan3Output8,midiChan);
}

void switchChan4() {
  digitalWrite(output1, LOW);
  digitalWrite(output2, LOW);
  digitalWrite(output3, LOW);
  digitalWrite(output4, HIGH);
  activeChan = 4;
  digitalWrite(output5, memoChan4Output5);
  output5State = memoChan4Output5;
  digitalWrite(output6, memoChan4Output6);
  output6State = memoChan4Output6;
  digitalWrite(output7, memoChan4Output7);
  output7State = memoChan4Output7;
  digitalWrite(output8, memoChan4Output8);
  output8State = memoChan4Output8;
  // Midi output for status copy in a Midi manager display
  midiA.sendProgramChange(3,midiChan);
  midiA.sendControlChange(12,memoChan4Output5,midiChan);
  midiA.sendControlChange(13,memoChan4Output6,midiChan);
  midiA.sendControlChange(14,memoChan4Output7,midiChan);
  midiA.sendControlChange(15,memoChan4Output8,midiChan);
}

// -------------------------------------------------------------------------

void setup() {
  // Outputs 
  pinMode(output1, OUTPUT); 
  pinMode(output2, OUTPUT); 
  pinMode(output3, OUTPUT);
  pinMode(output4, OUTPUT);
  pinMode(output5, OUTPUT);
  pinMode(output6, OUTPUT);
  pinMode(output7, OUTPUT);
  pinMode(output8, OUTPUT);
  // Inputs for the Midi Address quad-switch 
  pinMode(MidiAddrSw1,INPUT_PULLUP);
  pinMode(MidiAddrSw2,INPUT_PULLUP);
  pinMode(MidiAddrSw3,INPUT_PULLUP);
  pinMode(MidiAddrSw4,INPUT_PULLUP);
  // LAS VEGAS :
  digitalWrite(output1, HIGH);
  delay(200);
  digitalWrite(output1, LOW);
  digitalWrite(output2, HIGH);
  delay(200);
  digitalWrite(output2, LOW);
  digitalWrite(output3, HIGH);
  delay(200);
  digitalWrite(output3, LOW);
  digitalWrite(output4, HIGH);
  delay(200);
  digitalWrite(output4, LOW);
  digitalWrite(output5, HIGH);
  delay(200);
  digitalWrite(output5, LOW);
  digitalWrite(output6, HIGH);
  delay(200);
  digitalWrite(output6, LOW);
  digitalWrite(output7, HIGH);
  delay(200);
  digitalWrite(output7, LOW);
  digitalWrite(output8, HIGH);
  delay(200);
  digitalWrite(output8, LOW);
  digitalWrite(output1, HIGH);
 
  //activeChan = 1;

// Read the quad switch on the PCB and display the requested Midi channel number with the LEDs # 5, 6, 7, and 8 status (in binary value)
  digitalWrite(output5, !(digitalRead(MidiAddrSw4)));
  digitalWrite(output6, !(digitalRead(MidiAddrSw3)));
  digitalWrite(output7, !(digitalRead(MidiAddrSw2)));
  digitalWrite(output8, !(digitalRead(MidiAddrSw1)));  
  delay(2000); // let 2 secondes the Midi channel number displayed 
  
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
  digitalWrite(output5, LOW);
  digitalWrite(output6, LOW);
  digitalWrite(output7, LOW);
  digitalWrite(output8, LOW);
  delay(300); 
  int loop = 0 ;
  for (; loop < midiChan ; loop++) { 
    digitalWrite(output5, HIGH);
    digitalWrite(output6, HIGH);
    digitalWrite(output7, HIGH);
    digitalWrite(output8, HIGH);
    delay(300);
    digitalWrite(output5, LOW);
    digitalWrite(output6, LOW);
    digitalWrite(output7, LOW);
    digitalWrite(output8, LOW);
    delay(300);  
  }

//  MIDI.begin(midiChan); // set the MIDI channel and launch MIDI for the default serial port
  midiA.begin(midiChan); // set the Midi chan for a specific Midi instance named "midiA"
  midiA.turnThruOff(); // or turnThruOn(), toggle on/off midiThru
  // Midi output for status copy in a Midi manager display
  midiA.sendProgramChange(activeChan - 1,midiChan);
  midiA.sendControlChange(12,memoChan1Output5,midiChan);
  midiA.sendControlChange(13,memoChan1Output6,midiChan);
  midiA.sendControlChange(14,memoChan1Output7,midiChan);
  midiA.sendControlChange(15,memoChan1Output8,midiChan);

  switchChan1(); // init on the Chan#1
}

void loop() {

// These 4 first "radio" buttons are to select what channel is running :

// chan #1    
  int reading1 = analogRead(button1); // read the buttons state
  if (reading1 < 512) {
    reading1 = LOW;
  } else {
    reading1 = HIGH;
  }
  if (reading1 != lastButton1State) {  // Button state change 
    if (lastButton1State == HIGH) {    // Button is OFF
      lastDebounceButton1 = millis();
      delay(debounceDelay);
      if ((reading1 == LOW) && (lastButton1State == HIGH)) {    // Button pushed AND was not before then toggle output status
        digitalWrite(output1, !(output1State));
        output1State = !(output1State);
        lastButton1State = LOW;        // button pushed ON     
        switchChan1();     
      } 
    }      
  } else {                             // Button state no change
    lastDebounceButton1 = millis();
    delay(debounceDelay);
  }
  lastButton1State = reading1;         // synchronize with the switch status now

// chan #2  
  int reading2 = analogRead(button2); // read the buttons state
  if (reading2 < 512) {
    reading2 = LOW;
  } else {
    reading2 = HIGH;
  }
  if (reading2 != lastButton2State) {  // Button state change 
    if (lastButton2State == HIGH) {    // Button is OFF
      lastDebounceButton2 = millis();
      delay(debounceDelay);
      if ((reading2 == LOW) && (lastButton2State == HIGH)) {    // Button pushed AND was not before then toggle output status
        digitalWrite(output2, !(output2State));
        output2State = !(output2State);
        lastButton2State = LOW;        // button pushed ON     
        switchChan2();     
      } 
    }      
  } else {                             // Button state no change
    lastDebounceButton2 = millis();
    delay(debounceDelay);
  }
  lastButton2State = reading2;         // synchronize with the switch status now
  
// chan #3  
  int reading3 = analogRead(button3); // read the buttons state
  if (reading3 < 512) {
    reading3 = LOW;
  } else {
    reading3 = HIGH;
  }
  if (reading3 != lastButton3State) {  // Button state change 
    if (lastButton3State == HIGH) {    // Button is OFF
      lastDebounceButton3 = millis();
      delay(debounceDelay);
      if ((reading3 == LOW) && (lastButton3State == HIGH)) {    // Button pushed AND was not before then toggle output status
        digitalWrite(output3, !(output3State));
        output3State = !(output3State);
        lastButton3State = LOW;        // button pushed ON     
        switchChan3();     
      } 
    }      
  } else {                             // Button state no change
    lastDebounceButton3 = millis();
    delay(debounceDelay);
  }
  lastButton3State = reading3;         // synchronize with the switch status now

// chan #4  
  int reading4 = analogRead(button4); // read the buttons state
  if (reading4 < 512) {
    reading4 = LOW;
  } else {
    reading4 = HIGH;
  }
  if (reading4 != lastButton4State) {  // Button state change 
    if (lastButton4State == HIGH) {    // Button is OFF
      lastDebounceButton4 = millis();
      delay(debounceDelay);
      if ((reading4 == LOW) && (lastButton4State == HIGH)) {    // Button pushed AND was not before then toggle output status
        digitalWrite(output4, !(output4State));
        output4State = !(output4State);
        lastButton4State = LOW;        // button pushed ON     
        switchChan4();     
      } 
    }      
  } else {                             // Button state no change
    lastDebounceButton4 = millis();
    delay(debounceDelay);
  }
  lastButton4State = reading4;         // synchronize with the switch status now

// These 4 following independant buttons are to select FX, REV, EQU for a Carvin Quad-X, and an OPT for other amp/preamp :

  // button #5 :
  int reading5 = analogRead(button5); // read the buttons state
   if (reading5 < 512) {
     reading5 = LOW;
   } else {
     reading5 = HIGH;
   }
  if (reading5 != lastButton5State) {  // Button state change
    if (lastButton5State == HIGH) {    // Button is OFF
      lastDebounceButton5 = millis();
      delay(debounceDelay);
      if ((reading5 == LOW) && (lastButton5State == HIGH)) {    // Button pushed AND was not before then toggle output status
        digitalWrite(output5, !(output5State));
        output5State = !(output5State);
        lastButton5State = LOW;        // button pushed ON  
        // Midi output for status copy in a Midi manager display
        midiA.sendControlChange(12,output5State,midiChan);  
        memorisation();  
      }
    }
  } else {                             // Button state no change
    lastDebounceButton5 = millis();
    delay(debounceDelay);
  }
  lastButton5State = reading5;         // synchronize with the switch status now

  // button #6 :
  int reading6 = analogRead(button6); // read the buttons state
   if (reading6 < 512) {
     reading6 = LOW;
   } else {
     reading6 = HIGH;
   }
  if (reading6 != lastButton6State) {  // Button state change
    if (lastButton6State == HIGH) {    // Button is OFF
      lastDebounceButton6 = millis();
      delay(debounceDelay);
      if ((reading6 == LOW) && (lastButton6State == HIGH)) {    // Button pushed AND was not before then toggle output status
        digitalWrite(output6, !(output6State));
        output6State = !(output6State);
        lastButton6State = LOW;        // button pushed ON
         // Midi output for status copy in a Midi manager display
        midiA.sendControlChange(13,output6State,midiChan);  
        memorisation();     
      }
    }
  } else {                             // Button state no change
    lastDebounceButton6 = millis();
    delay(debounceDelay);
  }
  lastButton6State = reading6;         // synchronize with the switch status now
    
  // button #7 :
  int reading7 = analogRead(button7); // read the buttons state
   if (reading7 < 512) {
     reading7 = LOW;
   } else {
     reading7 = HIGH;
   }
  if (reading7 != lastButton7State) {  // Button state change
    if (lastButton7State == HIGH) {    // Button is OFF
      lastDebounceButton7 = millis();
      delay(debounceDelay);
      if ((reading7 == LOW) && (lastButton7State == HIGH)) {    // Button pushed AND was not before then toggle output status
        digitalWrite(output7, !(output7State));
        output7State = !(output7State);
        lastButton7State = LOW;        // button pushed ON
        // Midi output for status copy in a Midi manager display
        midiA.sendControlChange(14,output7State,midiChan);        
        memorisation();            
      }
    }
  } else {                             // Button state no change
    lastDebounceButton7 = millis();
    delay(debounceDelay);
  }
  lastButton7State = reading7;         // synchronize with the switch status now

  // button #8 :
  int reading8 = analogRead(button8); // read the buttons state
   if (reading8 < 512) {
     reading8 = LOW;
   } else {
     reading8 = HIGH;
   }
  if (reading8 != lastButton8State) {  // Button state change
    if (lastButton8State == HIGH) {    // Button is OFF
      lastDebounceButton8 = millis();
      delay(debounceDelay);
      if ((reading8 == LOW) && (lastButton8State == HIGH)) {    // Button pushed AND was not before then toggle output status
        digitalWrite(output8, !(output8State));
        output8State = !(output8State);
        lastButton8State = LOW;        // button pushed ON
        // Midi output for status copy in a Midi manager display
        midiA.sendControlChange(15,output8State,midiChan);        
        memorisation();            
      }
    }
  } else {                             // Button state no change
    lastDebounceButton8 = millis();
    delay(debounceDelay);
  }
  lastButton8State = reading8;         // synchronize with the switch status now

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
            digitalWrite(output1, HIGH);
            activeChan = 1;
            digitalWrite(output2, LOW);
            digitalWrite(output3, LOW);
            digitalWrite(output4, LOW);
            digitalWrite(output5, memoChan1Output5);
            output5State = memoChan1Output5;
            digitalWrite(output6, memoChan1Output6);
            output6State = memoChan1Output6;
            digitalWrite(output7, memoChan1Output7);
            output7State = memoChan1Output7;
            digitalWrite(output8, memoChan1Output8);
            output8State = memoChan1Output8;
            break;
          }
          case 1 : {
            digitalWrite(output1, LOW);
            digitalWrite(output2, HIGH);
            activeChan = 2;
            digitalWrite(output3, LOW);
            digitalWrite(output4, LOW);
            digitalWrite(output5, memoChan2Output5);
            output5State = memoChan2Output5;
            digitalWrite(output6, memoChan2Output6);
            output6State = memoChan2Output6;
            digitalWrite(output7, memoChan2Output7);
            output7State = memoChan2Output7;
            digitalWrite(output8, memoChan2Output8);
            output8State = memoChan2Output8;
            break;
          }
          case 2 : {
            digitalWrite(output1, LOW);
            digitalWrite(output2, LOW);
            digitalWrite(output3, HIGH);
            activeChan = 3;
            digitalWrite(output4, LOW);
            digitalWrite(output5, memoChan3Output5);
            output5State = memoChan3Output5;
            digitalWrite(output6, memoChan3Output6);
            output6State = memoChan3Output6;
            digitalWrite(output7, memoChan3Output7);
            output7State = memoChan3Output7;
            digitalWrite(output8, memoChan3Output8);
            output8State = memoChan3Output8;
            break;
          }
          case 3 : {
            digitalWrite(output1, LOW);
            digitalWrite(output2, LOW);
            digitalWrite(output3, LOW);
            digitalWrite(output4, HIGH);
            activeChan = 4;
            digitalWrite(output5, memoChan4Output5);
            output5State = memoChan4Output5;
            digitalWrite(output6, memoChan4Output6);
            output6State = memoChan4Output6;
            digitalWrite(output7, memoChan4Output7);
            output7State = memoChan4Output7;
            digitalWrite(output8, memoChan4Output8);
            output8State = memoChan4Output8;
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
                  digitalWrite(output5, LOW);
                  output5State = LOW;
                  //lastButton5State = LOW;    
                  memorisation();           // record options status for the current channel
                  break;
                }
                case 127 : {
                  digitalWrite(output5, HIGH);
                  output5State = HIGH;
                  //lastButton5State = LOW;    
                  memorisation();           // record options status for the current channel
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
                  digitalWrite(output6, LOW);
                  output6State = LOW;
                  //lastButton6State = LOW;    
                  memorisation();           // record options status for the current channel
                  break;
                }
                case 127 : {
                  digitalWrite(output6, HIGH);
                  output6State = HIGH; 
                  //lastButton6State = LOW;    
                  memorisation();           // record options status for the current channel
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
                  digitalWrite(output7, LOW);
                  output7State = LOW;    
                  memorisation();           // record options status for the current channel
                  break;
                }
                case 127 : {
                  digitalWrite(output7, HIGH);
                  output7State = HIGH;  
                  memorisation();           // record options status for the current channel                  
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
                  digitalWrite(output8, LOW);
                  output8State = LOW;
                  //lastButton8State = LOW;     
                  // send Midi message to copy status to manager in iPAD as example
                  //midiA.sendControlChange(15,0,midiChan);
                  memorisation();           // record options status for the current channel
                  break;
                }
                case 127 : {
                  digitalWrite(output8, HIGH);
                  output8State = HIGH;
                  //lastButton7State = LOW;
                  // send Midi message to copy status to manager in iPAD as example
                  //midiA.sendControlChange(15,1,midiChan);     
                  memorisation();           // record options status for the current channel                  
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
