#include <MIDI.h> 
#include "DrumPad.h"

#define PADS 9
#define CHANNEL 10
#define HIHATCONTROLLER 9

MIDI_CREATE_DEFAULT_INSTANCE();

const byte noteOff = 0x80; //bin 1000
const byte noteOn = 0x90;  //bin 1001
const byte controlChange = 0xB9;  //bin 1011

//Params
//tom1, tom2, tom3, kick, hi-hat, crash, ride, snare open, snare side
const byte note[] = {36, 38, 42, 43, 48, 49, 50, 51, 52};
const int thresholds[] = {150, 150, 180, 120, 120, 120, 120, 120, 120}; //between 120 and 500
const int sensitivities[] = {2000, 2000, 2000, 2000, 2000, 3600, 2000, 2000, 2000}; //between 2000 and 4700  ?? old value
const int scanTimes[] = {2, 2, 2, 2, 2, 2, 2, 2, 2};
const int maskTimes[]= {9, 9, 9, 9, 9, 9, 9, 9, 9};

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
    PadList[i].Init(i, note[i], thresholds[i], sensitivities[i], scanTimes[i], maskTimes[i]);
  }
  hiHatTimer = 0;
}
void SendMidiNoteOff(DrumPad pad) { 
  MIDI.sendNoteOff(pad.Note(), 0, CHANNEL);
}
void SendMidiNoteOn(DrumPad pad) {
  MIDI.sendNoteOn(pad.Note(), pad.Velocity(), CHANNEL);
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
  //-1 default
  //0 pad is sleeping (after hit)
  //1 first hit
  //2 during hit
  //3 masktime over, hit end
    int currentState = PadList[i].GetState(millis()); 
    switch(currentState){
      case 0:
        PadList[i].CheckIfWakeUp(millis());
      case 1:
        PadList[i].AddValue();
        PadList[i].Playing(true);
        PadList[i].SetScanTimer(millis());        
        break;
      case 2:
        PadList[i].AddValue();
        break;
      case 3:        
        SendMidiNoteOn(PadList[i]);
        SendMidiNoteOff(PadList[i]);
        PadList[i].Playing(false);
        PadList[i].ResetScanTimer();
        PadList[i].ResetCounters();
        PadList[i].SetMaskTimer(millis());        
        break;      
    }
  }
  float hiHatVol = analogRead(HIHATCONTROLLER);
  if (hiHatVol > hiHatMin && hiHatVol < hiHatMax && millis() - hiHatTimer > hiHatDelay){
    hiHatTimer = millis();
    SendHiHat(hiHatVol);
  }
}

