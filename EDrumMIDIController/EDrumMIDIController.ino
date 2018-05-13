#include <MIDI.h> 
#include "DrumPad.h"

#define PADS 9
#define CHANNEL 10
#define HIHATCONTROLLER 9

MIDI_CREATE_DEFAULT_INSTANCE();

const byte noteOff = 0x80; //bin 1000
const byte noteOn = 0x90;  //bin 1001
const byte controlChange = 0xB9;  //bin 1011

//tom1, tom2, tom3, kick, hi-hat, crash, ride, snare open, snare side
const byte note[] = {36, 38, 42, 43, 48, 49, 50, 51, 52};
const int values[] = {2000, 2000, 2000, 2000, 2000, 3600, 2000, 2000, 2000}; //between 2000 and 4700
const int thresholds[] = {150, 150, 180, 120, 120, 120, 120, 120, 120}; //between 120 and 500
const int debounce[] = {36, 36, 36, 36, 160, 160, 160, 36, 36};

const int hiHatDelay = 40;
const int hiHatMin = 322;
const int hiHatMax = 490;
const int hiHatSensitivity = 20;
const byte hiHatNote = 0x04;
unsigned long hiHatTimer;
int lastValue = 0;

DrumPad* PadList = new DrumPad[PADS];

void InitPads() {
  for (int i = 0; i < PADS; i++) {
    PadList[i].Init(i, note[i], values[i], thresholds[i], debounce[i]);
  }
  hiHatTimer = 0;
}
void SendMidiNoteOff(DrumPad pad) { 
  MIDI.sendNoteOff(pad.Note(), 0, CHANNEL);
}
void SendMidiNoteOn(DrumPad pad) {
  float volume = (pad.SumValue()) / float(pad.Value()) * 127;
  float adjustedVolume = 0;
  if (volume > 127) adjustedVolume = 127;
  else adjustedVolume = volume;
  byte velocity = (byte)adjustedVolume;
  MIDI.sendNoteOn(pad.Note(), velocity, CHANNEL);
}
void SendHiHat(float input){
  float volume = ((input-hiHatMin) / (float(hiHatMax)-hiHatMin)) * 127;
  if (volume < 20) volume = 0;
  else if (volume > 107) volume = 127;
  if (pow((volume - lastValue), 2) > pow(hiHatSensitivity, 2)){ 
    lastValue = volume;
    byte midiMsg[3] = {controlChange, hiHatNote, (byte)volume};
    MIDI.sendControlChange(hiHatNote, (byte)volume, CHANNEL);
  }
}
void setup() {
  MIDI.begin(MIDI_CHANNEL_OFF);
  InitPads();  
}
void loop() {
  for (int i = 0; i < PADS; i++) {
    PadList[i].UpdateReadValue();
    int currentState = PadList[i].GetState(millis());  
    switch(currentState){
      case 0:
        PadList[i].AddToSum();
        PadList[i].Playing(true);        
        break;
      case 1:
        PadList[i].AddToSum();
        break;
      case 2:        
        SendMidiNoteOn(PadList[i]);
        SendMidiNoteOff(PadList[i]);
        PadList[i].ResetTimerAndSum(millis());
        PadList[i].Playing(false);
        break;      
    }
  }
  float hiHatVol = analogRead(HIHATCONTROLLER);
  if (hiHatVol > hiHatMin && hiHatVol < hiHatMax && millis() - hiHatTimer > hiHatDelay){
    hiHatTimer = millis();
    SendHiHat(hiHatVol);
  }
}

