#include "Arduino.h"
#include "DrumPad.h"

void DrumPad::Init(int analogInput, byte note, int threshold, int sensitivity, int scanTime, int maskTime){
  _scanTimer = 0;
  _maskTimer = 0;
  _isPlaying = false;
  _padSleeps = false;
  _analogInput = analogInput;
  _note = note;
  _threshold = threshold;
  _sensitivity = sensitivity;
  _scanTime = scanTime;
  _maskTime = maskTime;
  _readValue = 0;
  _sumValue = 0;
  _numberOfCounts = 0;
}
void DrumPad::UpdateReadValue(){
  _readValue = analogRead(_analogInput);
}
int DrumPad::GetState(unsigned long currentTime){
  int result = -1;
  if (_padSleeps){
    result = 0;
  }
  if (_readValue >= _threshold && _isPlaying == false) {
    result = 1;
  }
  else if (_isPlaying == true && (currentTime - _scanTimer < _scanTime)) {
    result = 2;
  }
  else if (_isPlaying == true && (currentTime - _scanTimer >= _scanTime)) {
    result = 3;
  } 
  return result;
}
bool DrumPad::PadSleeps(){
  return _padSleeps;
}
void DrumPad::CheckIfWakeUp(unsigned long currentTime){
  if (currentTime - _maskTimer < _maskTime){
    _padSleeps = false;
    _maskTimer = 0;
  }
}
void DrumPad::Playing(bool isPlaying){
  _isPlaying = isPlaying;
}
void DrumPad::SetScanTimer(unsigned long currentTime){
  _scanTimer = currentTime;
}
void DrumPad::ResetScanTimer(){
  _scanTimer = 0;
}
void DrumPad::SetMaskTimer(unsigned long currentTime){
  _maskTimer = currentTime;
  _padSleeps = true;
}
void DrumPad::AddValue(){
  _sumValue = _sumValue + _readValue;
  _numberOfCounts = _numberOfCounts + 1;
}
void DrumPad::ResetCounters(){
  _sumValue = 0;
  _numberOfCounts = 0;
}
byte DrumPad::Velocity(){
  float value = (float(_sumValue / _numberOfCounts)/1022)*127;
  if (value > 127) value = 127;
  return byte(value);
}
byte DrumPad::Note(){
  return _note;
}


