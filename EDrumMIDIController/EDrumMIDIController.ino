#include <MIDI.h> 
#include "DrumPad.h"

#define PADS 9
#define CHANNEL 10
#define HIHATCONTROLLER 9

MIDI_CREATE_DEFAULT_INSTANCE();

const byte noteOff = 0x80;
const byte noteOn = 0x90;
const byte controlChange = 0xB9;

//Pad params
//tom1, tom2, tom3, kick, hi-hat, crash, ride, snare open, snare side
const byte note[] = {36, 38, 42, 43, 48, 49, 50, 51, 52};
const int baseThresholds[] = {280, 240, 230, 140, 180, 250, 180, 160, 240}; //between 120 and 500
const float thresholdSlopes[] = {100, 100, 100, 70, 10, 40, 45, 100, 100}; //between 0 and 200(?), 20 is probably good
const int sensitivities[] = {2000, 2000, 2000, 2000, 2000, 3600, 2000, 2000, 2000}; //between 2000 and 4700  ?? old value
const int scanTimes[] = {8, 8, 8, 8, 8, 8, 8, 8, 8}; //ms
const int maskTimes[]= {10, 10, 10, 10, 30, 16, 20, 10, 10}; //ms
const float velocityExponents[] = {1.2, 1.2, 1.2, 0.4, 0.4, 0.8, 0.8, 0.8, 1.8}; //between 0.1 and 10.0 //1.0 is linear, 2.0 is curving -y, 0.5 is curving +y
const float minVelocities[] = {25, 25, 25, 25, 35, 35, 35, 25, 25};

//Hi-hat
const int hiHatDelay = 5; //30
const int hiHatMin = 0;
const int hiHatMax = 610;
const int hiHatSensitivity = 5; //15
const byte hiHatNote = 0x04;
unsigned long hiHatTimer;
float hiHatRead;
int lastValue = 0;

//CrashStop
int inputVal;
bool isOver = false;

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
  if (volume < 10) volume = 0;
  else if (volume > 117) volume = 127;  
  MIDI.sendControlChange(hiHatNote, (byte)volume, CHANNEL);
  //Serial.println(String("Input ") + input + String(", Volume ") + volume);
}
void setup() {
  MIDI.begin(MIDI_CHANNEL_OFF);
  //Serial.begin(38400);
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
        //Serial.println(analogRead(i) + String(", padNr ") + i + String(", case 0"));
        PadList[i].CheckIfWakeUp(millis());
        PadList[i].DecreaseThreshold();
        break;
      case 1:
        //Serial.println(analogRead(i) + String(", padNr ") + i + String(", case 1, threshold ") + PadList[i].GetThreshold());
        PadList[i].AddValue();
        PadList[i].Playing(true);
        PadList[i].SetScanTimer(millis());
        PadList[i].SetThreshold();       
        break;
      case 2:
        //Serial.println(analogRead(i) + String(", padNr ") + i + String(", case 2"));
        if (PadList[i].GetReadValue() > PadList[i].GetThreshold()) PadList[i].AddValue();
        PadList[i].DecreaseThreshold();
        break;
      case 3: 
        //Serial.println(analogRead(i) + String(", padNr ") + i + String(", case 3, velocity ") + PadList[i].Velocity() + String(", sumVal ") + PadList[i].GetSumValue() + String(", numCount ") + PadList[i].GetNumberOfCounts());       
        SendMidiNoteOn(PadList[i]);
        //SendMidiNoteOff(PadList[i]);
        PadList[i].Playing(false);
        PadList[i].ResetScanTimer();
        PadList[i].ResetCounters();
        PadList[i].Sleeping(true);
        PadList[i].SetMaskTimer(millis());     
        break;
      case 4:
        //Serial.println(analogRead(i) + String(", padNr ") + i + String(", case 4, threshold ") + PadList[i].GetThreshold());
        PadList[i].DecreaseThreshold();
        break;      
    }
  }
  hiHatRead = analogRead(HIHATCONTROLLER);
  //Serial.println(hiHatRead);
  if ((hiHatRead > hiHatMin) && (hiHatRead < hiHatMax) && (millis() - hiHatTimer > hiHatDelay)){
    hiHatTimer = millis();
    if (abs(hiHatRead - lastValue) > hiHatSensitivity){
      lastValue = hiHatRead;
      SendHiHat(hiHatRead);
    }    
  }
  inputVal = analogRead(10);
  //Serial.println(inputVal);
  if ((inputVal >= 1000) && (isOver == false)){
    isOver = true;
    MIDI.sendNoteOn(byte(39), byte(40), CHANNEL);
    //Serial.println("Test");    
  }else if ((inputVal < 1000) && (isOver == true)){
    isOver = false;
  }
}