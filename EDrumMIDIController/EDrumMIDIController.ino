#include <MIDI.h> 
#include "DrumPad.h"
#include "SevSeg.h" 
#include <EEPROM.h>

#define PAD_COUNT 9
#define PARAM_COUNT 6
#define MIDI_CHANNEL 10
#define HIHAT_CONTROLLER_PIN 9    // analog
#define PAD_SELECTOR_PIN 10       // analog
#define PARAM_SELECTOR_PIN 11     // analog
#define PROGRAMMING_MODE_PIN 32   // digital
#define VALUE_ROT_ENC_A 33        // digital
#define VALUE_ROT_ENC_B 34        // digital
#define BTN_PIN 35                // digital

// Display pins, all digital
#define DISPLAY_DIG_1 23
#define DISPLAY_DIG_2 30
#define DISPLAY_A 29
#define DISPLAY_B 31
#define DISPLAY_C 28
#define DISPLAY_D 22
#define DISPLAY_E 26
#define DISPLAY_F 27
#define DISPLAY_G 25
#define DISPLAY_DP 24

////////////////////////////
//          MIDI          //
////////////////////////////

MIDI_CREATE_DEFAULT_INSTANCE();

const byte noteOff = 0x80;
const byte noteOn = 0x90;
const byte controlChange = 0xB9;

////////////////////////////
//   Pads and parameters  //
////////////////////////////

// tom1, tom2, tom3, kick, hi-hat, crash, ride, snare open, snare side
const byte note[] = {36, 38, 42, 43, 48, 49, 50, 51, 52};

// Default param values
const int baseThresholds[] = {280, 240, 230, 140, 180, 250, 180, 160, 240};
const int thresholdSlopes[] = {100, 100, 100, 70, 10, 40, 45, 100, 100};
const int scanTimes[] = {8, 8, 8, 8, 8, 8, 8, 8, 8}; //ms
const int maskTimes[]= {10, 10, 10, 10, 30, 16, 20, 10, 10}; //ms
const int velocityExponents[] = {12, 12, 12, 4, 4, 8, 8, 8, 18}; // 10 is linear, 20 is curving -y, 5 is curving +y
const int minVelocities[] = {25, 25, 25, 25, 35, 35, 35, 25, 25};

const int minParamVal[] = {100, 0, 0, 0, 0, 0};
const int maxParamVal[] = {610, 200, 99, 99, 99, 99};
const int stepParamVal[] = {5, 2, 1, 1, 1, 1}; //temporary

// Hi-hat
const byte hiHatNote = 0x04;
const int hiHatDelay = 5; //30
const int hiHatMin = 0;
const int hiHatMax = 610;
const int hiHatSensitivity = 5; //15
unsigned long hiHatTimer;
float hiHatRead;
int lastValue = 0;

DrumPad* PadList = new DrumPad[PAD_COUNT];

////////////////////////////
//    Programming mode    //
////////////////////////////

int currentVal;

// Display
SevSeg Display;
bool displayAck = false;
byte ackType; // 0 = Save, 1 = Set to default
unsigned long ackTimer = 0;
int displayAckLimitMs = 2000;

// Value setter, rotary encoder 
int rotEncAValue;
bool rotEncAIsLow;

// Pad selector, potentiometer
int currentSelectedPad;
int lastSelectedPad;

// Param selector, rotary switch
int currentSelectedParam;
int lastSelectedParam;

// Store values/set default values, push button
int currentBtnState;
int lastBtnState;
unsigned long btnPressedTimer = 0;
int saveValuesPressTimeMs = 1000;
int setValuesToDefaultPressTimeMS = 5000;

void InitPads() {
  for (int i = 0; i < PAD_COUNT; i++) {
    PadList[i].Init(i, note[i]);

    for (int j = 0; j < PARAM_COUNT; j++) {
      PadList[i].SetParamValue(j, GetValueFromEPROOM(i, j));
    }  
  }
  //SetValuesToDefault();
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
}
void HandleConfigurationAndDisplay(){
  if (displayAck) {
    if (ackType == 0) {             // Save
      Display.DisplayString(" S");
    }
    else if (ackType == 1) {        // Set to default
      Display.DisplayString(" -");
    }
    if (millis() >= (ackTimer + displayAckLimitMs)) {
      displayAck = false;
    }
  }
  else {  
    currentSelectedPad = GetSelectedPad(analogRead(PAD_SELECTOR_PIN));
    if (currentSelectedPad != -1) {
      lastSelectedPad = currentSelectedPad;
    }
    currentSelectedParam = GetSelectedParam(analogRead(PARAM_SELECTOR_PIN));
    if (currentSelectedParam != -1) {
      lastSelectedParam = currentSelectedParam;
    }
    if ((currentSelectedPad < PAD_COUNT) && (currentSelectedParam < PARAM_COUNT)) {
      currentVal = PadList[lastSelectedPad].GetParamValue(lastSelectedParam);

      rotEncAValue = digitalRead(VALUE_ROT_ENC_A);
      if (!rotEncAValue && !rotEncAIsLow) {
        if (digitalRead(VALUE_ROT_ENC_B)) {
          if (currentVal > minParamVal[lastSelectedParam]) {
            currentVal -= stepParamVal[lastSelectedParam];
            PadList[lastSelectedPad].SetParamValue(lastSelectedParam, currentVal);
          }
        }
        else {
          if (PadList[lastSelectedPad].GetParamValue(lastSelectedParam) < maxParamVal[lastSelectedParam]){
            currentVal += stepParamVal[lastSelectedParam];
            PadList[lastSelectedPad].SetParamValue(lastSelectedParam, currentVal);
          }
        }
        rotEncAIsLow = true;
      }
      else if (rotEncAValue) {
        rotEncAIsLow = false;
      }
      Display.DisplayInt(map(currentVal, minParamVal[lastSelectedParam], maxParamVal[lastSelectedParam], 0, 99));
    }
    else {
      Display.DisplayString("nn");
    }

    currentBtnState = digitalRead(BTN_PIN);
    // on button pressed
    if ((lastBtnState == HIGH) && (currentBtnState == LOW)) {
      btnPressedTimer = millis();
    }
    // on button released
    else if ((lastBtnState == LOW) && (currentBtnState == HIGH)){
      // Set values to default
      if (millis() >= btnPressedTimer + setValuesToDefaultPressTimeMS){
        SetValuesToDefault();
        ackType = 1;
        displayAck = true;
        ackTimer = millis();
      }
      // Save current values to EPROOM
      else if (millis() >= btnPressedTimer + saveValuesPressTimeMs) {
        SaveAllValuesToEPROOM();
        ackType = 0;
        displayAck = true;
        ackTimer = millis();
      }            
    }
    lastBtnState = currentBtnState;
  }  
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
  if (inputValue < 430 && inputValue > 390) return 7;
  else if (inputValue < 460 && inputValue > 430) return 6;
  else if (inputValue < 510 && inputValue > 480) return 5;
  else if (inputValue < 570 && inputValue > 540) return 4;
  else if (inputValue < 640 && inputValue > 610) return 3;
  else if (inputValue < 730 && inputValue > 700) return 2;
  else if (inputValue < 860 && inputValue > 830) return 1;
  else if (inputValue > 1010) return 0;
  else return -1;
}
void SaveAllValuesToEPROOM(){
  int paramVal;
  byte lowByte = 0;
  byte highByte = 0;
  int addrCounter = 0;
  for (int i = 0; i < PAD_COUNT; i++) {
    for (int j = 0; j < PARAM_COUNT; j++) {
      paramVal = PadList[i].GetParamValue(j);
      lowByte = ((paramVal >> 0) & 0xFF);
      highByte = ((paramVal >> 8) & 0xFF);
      EEPROM.write(addrCounter, lowByte);
      EEPROM.write(addrCounter + 1, highByte);      
      addrCounter += 2;
    }
  }  
}
void SetValuesToDefault(){
  for (int i = 0; i < PAD_COUNT; i++) {
    PadList[i].SetParamValue(0, baseThresholds[i]);
    PadList[i].SetParamValue(1, thresholdSlopes[i]);
    PadList[i].SetParamValue(2, scanTimes[i]);
    PadList[i].SetParamValue(3, maskTimes[i]);
    PadList[i].SetParamValue(4, velocityExponents[i]);
    PadList[i].SetParamValue(5, minVelocities[i]);
  }
}
int GetValueFromEPROOM(int padIndex, int paramIndex){
  int addrIndex = ((padIndex * PARAM_COUNT) + paramIndex) * 2;
  byte lowByte = EEPROM.read(addrIndex);
  byte highByte = EEPROM.read(addrIndex + 1);
  int resValue = ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
  return resValue;
}
void setup() {
  MIDI.begin(MIDI_CHANNEL_OFF);
  //Serial.begin(9600);
  InitPads();

  Display.Begin(1, 2, DISPLAY_DIG_1, DISPLAY_DIG_2, DISPLAY_A, DISPLAY_B,
    DISPLAY_C, DISPLAY_D, DISPLAY_E, DISPLAY_F, DISPLAY_G, DISPLAY_DP);

  pinMode(PROGRAMMING_MODE_PIN, INPUT);
  pinMode(VALUE_ROT_ENC_A, INPUT_PULLUP);
  pinMode(VALUE_ROT_ENC_B, INPUT_PULLUP);
  pinMode(BTN_PIN, INPUT_PULLUP);

  currentSelectedPad = GetSelectedPad(analogRead(PAD_SELECTOR_PIN));
  lastSelectedPad = currentSelectedPad;

  currentSelectedParam = GetSelectedParam(analogRead(PARAM_SELECTOR_PIN));
  lastSelectedParam = currentSelectedParam;

  currentVal = PadList[lastSelectedPad].GetParamValue(lastSelectedParam);

  rotEncAIsLow = !digitalRead(VALUE_ROT_ENC_A);

  currentBtnState = digitalRead(BTN_PIN);
  lastBtnState = currentBtnState;
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
        PadList[i].CheckIfWakeUp(millis());
        PadList[i].DecreaseThreshold();
        break;
      case 1:
        PadList[i].AddValue();
        PadList[i].Playing(true);
        PadList[i].SetScanTimer(millis());
        PadList[i].SetThreshold();       
        break;
      case 2:
        if (PadList[i].GetReadValue() > PadList[i].GetThreshold()) PadList[i].AddValue();
        PadList[i].DecreaseThreshold();
        break;
      case 3:     
        SendMidiNoteOn(PadList[i]);
        //SendMidiNoteOff(PadList[i]);
        PadList[i].Playing(false);
        PadList[i].ResetScanTimer();
        PadList[i].ResetCounters();
        PadList[i].Sleeping(true);
        PadList[i].SetMaskTimer(millis());     
        break;
      case 4:
        PadList[i].DecreaseThreshold();
        break;      
    }
  }
  hiHatRead = analogRead(HIHAT_CONTROLLER_PIN);
  if ((hiHatRead > hiHatMin) && (hiHatRead < hiHatMax) && (millis() - hiHatTimer > hiHatDelay)){
    hiHatTimer = millis();
    if (abs(hiHatRead - lastValue) > hiHatSensitivity){
      lastValue = hiHatRead;
      SendHiHat(hiHatRead);
    }    
  }

}