#include <MIDI.h> 
#include "DrumPad.h"
#include "SevSeg.h" 
#include <EEPROM.h>

#define PAD_COUNT 9
#define MIDI_CHANNEL 10
#define HIHAT_CONTROLLER_PIN 9    // analog
#define PAD_SELECTOR_PIN 12       // analog
#define PARAM_SELECTOR_PIN 12       // analog
#define PROGRAMMING_MODE_PIN 36   // digital
#define VALUE_ROT_ENC_A 37        // digital
#define VALUE_ROT_ENC_B 38        // digital

// Display pins, all digital
#define DISPLAY_DIG_1 30
#define DISPLAY_DIG_2 31
#define DISPLAY_A 25
#define DISPLAY_B 23
#define DISPLAY_C 28
#define DISPLAY_D 22
#define DISPLAY_E 26
#define DISPLAY_F 27
#define DISPLAY_G 29
#define DISPLAY_DP 24

MIDI_CREATE_DEFAULT_INSTANCE();

const byte noteOff = 0x80;
const byte noteOn = 0x90;
const byte controlChange = 0xB9;

// Pad params
// tom1, tom2, tom3, kick, hi-hat, crash, ride, snare open, snare side
const byte note[] = {36, 38, 42, 43, 48, 49, 50, 51, 52};
const int baseThresholds[] = {280, 240, 230, 140, 180, 250, 180, 160, 240}; //between 120 and 500
const float thresholdSlopes[] = {100, 100, 100, 70, 10, 40, 45, 100, 100}; //between 0 and 200(?), 20 is probably good
const int sensitivities[] = {2000, 2000, 2000, 2000, 2000, 3600, 2000, 2000, 2000}; //between 2000 and 4700  ?? old value
const int scanTimes[] = {8, 8, 8, 8, 8, 8, 8, 8, 8}; //ms
const int maskTimes[]= {10, 10, 10, 10, 30, 16, 20, 10, 10}; //ms
const float velocityExponents[] = {1.2, 1.2, 1.2, 0.4, 0.4, 0.8, 0.8, 0.8, 1.8}; //between 0.1 and 10.0 //1.0 is linear, 2.0 is curving -y, 0.5 is curving +y
const float minVelocities[] = {25, 25, 25, 25, 35, 35, 35, 25, 25};

// Hi-hat
const int hiHatDelay = 5; //30
const int hiHatMin = 0;
const int hiHatMax = 610;
const int hiHatSensitivity = 5; //15
const byte hiHatNote = 0x04;
unsigned long hiHatTimer;
float hiHatRead;
int lastValue = 0;

// CrashStop
int inputVal;
bool isOver = false;

DrumPad* PadList = new DrumPad[PAD_COUNT];

// Display and programming mode
SevSeg Display;

bool rotEncAIsLow;
int rotEncAValue;

int currentSelectedPad;
int lastSelectedPad;

// EPROOM
const byte defaultValues[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
byte values[11];

void InitPads() {
  for (int i = 0; i < PAD_COUNT; i++) {
    PadList[i].Init(i, note[i], baseThresholds[i], thresholdSlopes[i],
      sensitivities[i], scanTimes[i], maskTimes[i], velocityExponents[i],
      minVelocities[i]);
  }
  hiHatTimer = 0;
}
void SendMidiNoteOff(DrumPad pad) { 
  MIDI.sendNoteOff(pad.Note(), 0, MIDI_CHANNEL);
}
void SendMidiNoteOn(DrumPad pad) {
  MIDI.sendNoteOn(pad.Note(), pad.Velocity(), MIDI_CHANNEL);
}
void SendHiHat(float input){
  float volume = ((input-hiHatMin) / (float(hiHatMax)-hiHatMin)) * 127;
  if (volume < 10) volume = 0;
  else if (volume > 117) volume = 127;  
  MIDI.sendControlChange(hiHatNote, (byte)volume, MIDI_CHANNEL);
  //Serial.println(String("Input ") + input + String(", Volume ") + volume);
}
void HandleConfigurationAndDisplay(){
  currentSelectedPad = GetSelectedPad(analogRead(PAD_SELECTOR_PIN));
  if (currentSelectedPad != -1) {
    lastSelectedPad = currentSelectedPad;
  }
  rotEncAValue = digitalRead(VALUE_ROT_ENC_A);
  if (!rotEncAValue && !rotEncAIsLow) {
    if (digitalRead(VALUE_ROT_ENC_B)) {
      if (values[lastSelectedPad] > 0) {
        values[lastSelectedPad]--;
      }
    }
    else {
      if (values[lastSelectedPad] < 99) {
        values[lastSelectedPad]++;
      }
    }
    rotEncAIsLow = true;
  }
  else if (rotEncAValue) {
    rotEncAIsLow = false;
  }
  Display.DisplayInt(values[lastSelectedPad]);
}
int GetSelectedPad(int inputValue){  
  if (inputValue < 10) return 0;
  else if (inputValue < 35 && inputValue > 20) return 1;
  else if (inputValue < 160 && inputValue > 140) return 2;
  else if (inputValue < 290 && inputValue > 250) return 3;
  else if (inputValue < 400 && inputValue > 370) return 4;
  else if (inputValue < 520 && inputValue > 490) return 5;
  else if (inputValue < 640 && inputValue > 600) return 6;
  else if (inputValue < 750 && inputValue > 720) return 7;
  else if (inputValue < 870 && inputValue > 840) return 8;
  else if (inputValue < 990 && inputValue > 950) return 9;
  else if (inputValue > 1010) return 10;
  else return -1;
}
int GetSelectedParam(int inputValue){  
  if (inputValue < 430 && inputValue > 390) return 8;
  else if (inputValue < 460 && inputValue > 430) return 7;
  else if (inputValue < 510 && inputValue > 480) return 6;
  else if (inputValue < 570 && inputValue > 540) return 5;
  else if (inputValue < 640 && inputValue > 610) return 4;
  else if (inputValue < 730 && inputValue > 700) return 3;
  else if (inputValue < 860 && inputValue > 830) return 2;
  else if (inputValue > 1010) return 1;
  else return -1;
}
void setup() {
  MIDI.begin(MIDI_CHANNEL_OFF);
  //Serial.begin(38400);
  InitPads();

  Display.Begin(1, 2, DISPLAY_DIG_1, DISPLAY_DIG_2, DISPLAY_A, DISPLAY_B,
    DISPLAY_C, DISPLAY_D, DISPLAY_E, DISPLAY_F, DISPLAY_G, DISPLAY_DP);

  pinMode(PROGRAMMING_MODE_PIN, INPUT);
  pinMode(VALUE_ROT_ENC_A, INPUT_PULLUP);
  pinMode(VALUE_ROT_ENC_B, INPUT_PULLUP);

  currentSelectedPad = GetSelectedPad(analogRead(PAD_SELECTOR_PIN));
  lastSelectedPad = currentSelectedPad;
  
  rotEncAIsLow = !digitalRead(VALUE_ROT_ENC_A);

  // Write to memory the first time
  // for (int i = 0; i < 11; i++) {
  //   EEPROM.write(i, defaultValues[i]);
  // } 
  
  for (int i = 0; i < 11; i++) {
    values[i] = EEPROM.read(i);
  } 
}
void loop() {
  // If in programming mode
  if (digitalRead(PROGRAMMING_MODE_PIN) == HIGH) {
    HandleConfigurationAndDisplay();
  }
  
  for (int i = 0; i < PAD_COUNT; i++) {
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
  hiHatRead = analogRead(HIHAT_CONTROLLER_PIN);
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
    MIDI.sendNoteOn(byte(39), byte(40), MIDI_CHANNEL);
    //Serial.println("Test");    
  }else if ((inputVal < 1000) && (isOver == true)){
    isOver = false;
  }
}