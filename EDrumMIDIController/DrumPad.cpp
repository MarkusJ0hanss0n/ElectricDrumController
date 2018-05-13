#include "Arduino.h"
#include "DrumPad.h"

void DrumPad::Init(int analogInput, byte note, int value, int threshold, int debounce){
  _timer = 0;
  _isPlaying = false;
  _sumValue = 0;
  _analogInput = analogInput;
  _note = note;
  _value = value;
  _threshold = threshold;
  _debounce = debounce;
}
int DrumPad::ReadValue(){
  return _readValue;
}
void DrumPad::UpdateReadValue(){
  _readValue = analogRead(_analogInput);
}
void DrumPad::AddToSum() {
  _sumValue = _sumValue + _readValue;
}
int DrumPad::GetState(unsigned long currentTime){
  int result = -1;
  if (_readValue >= _threshold && _isPlaying == false && (currentTime - _timer >= _debounce)) {
    result = 0;
  }
  else if (_readValue >= _threshold && _isPlaying == true) {
    result = 1;
  }
  else if (_readValue < _threshold && _isPlaying == true) {
    result = 2;
  }
  return result;
}
void DrumPad::ResetTimerAndSum(unsigned long currentTime){
  _sumValue = 0;
  _timer = currentTime;
}
void DrumPad::Playing(bool isPlaying){
  _isPlaying = isPlaying;
}
byte DrumPad::Note(){
  return _note;
}
int DrumPad::Value(){
  return _value;
}
float DrumPad::SumValue(){
  return _sumValue; 
}
int DrumPad::AnalogInput(){
  return _analogInput;
}


