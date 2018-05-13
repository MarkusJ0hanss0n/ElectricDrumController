#ifndef DrumPad_H
#define DrumPad_H
#include <Arduino.h>

class DrumPad{
  public:
    void Init(int analogInput, byte note, int value, int threshold, int debounce);
    void UpdateReadValue();
    int ReadValue();
    void AddToSum();
    int GetState(unsigned long currentTime);
    void ResetTimerAndSum(unsigned long currentTime);
    void Playing(bool isPlaying); 
    byte Note();
    int Value();
    float SumValue();
    int AnalogInput();     
  private:
    int _analogInput;
    byte _note;
    int _readValue;
    float _valueSens;
    unsigned long _timer;
    bool _isPlaying;
    int _value;
    int _threshold; 
    float _sumValue;
    int _debounce;       
};
#endif

