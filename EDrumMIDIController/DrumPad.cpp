#include "Arduino.h"
#include "DrumPad.h"

void DrumPad::Init(int analogInput, byte note, int baseThreshold, float thresholdSlope, int sensitivity, int scanTime, int maskTime, float velocityExponent, float minVelocity){
  _note = note;
  _analogInput = analogInput;

//Params
  _baseThreshold = baseThreshold;  
  _thresholdSlope = thresholdSlope;
  _sensitivity = sensitivity;
  _scanTime = scanTime;
  _maskTime = maskTime;
  _velocityExponent = velocityExponent;
  _minVelocity = minVelocity;  

//Helpers
  _currentThreshold = baseThreshold;  
  _scanTimer = 0;
  _maskTimer = 0;
  _isPlaying = false;
  _padSleeps = false;
  _readValue = 0;
  _sumValue = 0;
  _numberOfCounts = 0;
  _maxValue = 0;
}
void DrumPad::UpdateReadValue(){
  _readValue = analogRead(_analogInput);
}
int DrumPad::GetState(unsigned long currentTime){
  int result = -1;
  if (_padSleeps == true){
    result = 0;
  }
  else if (_isPlaying == false && _readValue > _currentThreshold) {
    result = 1;
  }
  else if (_isPlaying == true && (currentTime - _scanTimer < _scanTime)) {
    result = 2;
  }
  else if (_isPlaying == true && (currentTime - _scanTimer >= _scanTime)) {
    result = 3;
  }
  else if (_currentThreshold > _baseThreshold){
    result = 4;     
  }
  return result;
}
bool DrumPad::PadSleeps(){
  return _padSleeps;
}
void DrumPad::CheckIfWakeUp(unsigned long currentTime){
  if (currentTime - _maskTimer > _maskTime){
    _padSleeps = false;
    _maskTimer = 0;
  }
}
void DrumPad::Playing(bool isPlaying){
  _isPlaying = isPlaying;
}
void DrumPad::Sleeping(bool isSleeping){
  _padSleeps = isSleeping;
}
void DrumPad::SetScanTimer(unsigned long currentTime){
  _scanTimer = currentTime;
}
void DrumPad::ResetScanTimer(){
  _scanTimer = 0;
}
void DrumPad::SetMaskTimer(unsigned long currentTime){
  _maskTimer = currentTime;
}
void DrumPad::AddValue(){
  _sumValue = _sumValue + _readValue;
  _numberOfCounts = _numberOfCounts + 1;
  if (_readValue > _maxValue) _maxValue = _readValue;
}
void DrumPad::SetThreshold(){
  _currentThreshold = _maxValue - _thresholdSlope;
  _maxValue = 0;
}
void DrumPad::DecreaseThreshold(){
  _currentThreshold = _currentThreshold - _thresholdSlope;
  if (_currentThreshold < _baseThreshold) _currentThreshold = _baseThreshold;
}
void DrumPad::ResetCounters(){
  _sumValue = 0;
  _numberOfCounts = 0;
}
byte DrumPad::Velocity(){
  //Use the highest top or mean value?
  float x = float(_sumValue / _numberOfCounts);
  //float x = _maxValue;
  float value = pow(abs((x-_baseThreshold)/(1023-_baseThreshold)), _velocityExponent)*127;
  if (value <= _minVelocity) value = _minVelocity;  
  else if (value > 127) value = 127;  
  return byte(value);
}
byte DrumPad::Note(){
  return _note;
}
int DrumPad::GetReadValue(){
  return _readValue;
}
float DrumPad::GetThreshold(){
  return _currentThreshold;
}
//Analyze only
int DrumPad::GetNumberOfCounts(){
  return _numberOfCounts;
}
int DrumPad::GetSumValue(){
  return _sumValue;
}


