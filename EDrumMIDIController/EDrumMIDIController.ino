#include <MIDI.h> 
#include "DrumPad.h"

#define PADS 9
#define CHANNEL 10
#define HIHATCONTROLLER 9

MIDI_CREATE_DEFAULT_INSTANCE();

const byte noteOff = 0x80;
const byte noteOn = 0x90;
const byte controlChange = 0xB9;

//Params
//tom1, tom2, tom3, kick, hi-hat, crash, ride, snare open, snare side
const byte note[] = {36, 38, 42, 43, 48, 49, 50, 51, 52};
const int baseThresholds[] = {280, 240, 220, 160, 180, 240, 240, 160, 240}; //between 120 and 500
const float thresholdSlopes[] = {25, 25, 25, 25, 12, 15, 15, 25, 25}; //between 0 and 200(?), 20 is probably good
const int sensitivities[] = {2000, 2000, 2000, 2000, 2000, 3600, 2000, 2000, 2000}; //between 2000 and 4700  ?? old value
const int scanTimes[] = {2, 2, 2, 2, 2, 2, 2, 2, 2}; //ms
const int maskTimes[]= {10, 10, 10, 10, 55, 25, 25, 10, 10}; //ms
const float velocityExponents[] = {0.8, 0.8, 0.8, 0.8, 0.3, 0.8, 0.8, 0.8, 0.8}; //between 0.1 and 10.0 //1.0 is linear, 2.0 is rising slower, 0.5 is rising faster
const float minVelocities[] = {25, 25, 25, 25, 35, 35, 35, 25, 25};

//Hi-hat
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
    PadList[i].Init(i, note[i], baseThresholds[i], thresholdSlopes[i], sensitivities[i], scanTimes[i], maskTimes[i], velocityExponents[i], minVelocities[i]);
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
  //3 scantime over, hit end
  //4 hit and sleep over, but dynamic threshold not yet back to base threshold
    int currentState = PadList[i].GetState(millis()); 
    switch(currentState){
      case 0:
        PadList[i].CheckIfWakeUp(millis());
        break;
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
        //SendMidiNoteOff(PadList[i]);
        PadList[i].Playing(false);
        PadList[i].ResetScanTimer();
        PadList[i].ResetCounters();
        PadList[i].Sleeping(true);
        PadList[i].SetMaskTimer(millis()); 
        PadList[i].SetThreshold();      
        break;
      case 4:
        PadList[i].DecreaseThreshold();
        break;      
    }
  }
  float hiHatVol = analogRead(HIHATCONTROLLER);
  if (hiHatVol > hiHatMin && hiHatVol < hiHatMax && millis() - hiHatTimer > hiHatDelay){
    hiHatTimer = millis();
    SendHiHat(hiHatVol);
  }
}

