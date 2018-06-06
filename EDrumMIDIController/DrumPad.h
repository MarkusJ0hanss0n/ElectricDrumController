#ifndef DrumPad_H
#define DrumPad_H
#include <Arduino.h>

class DrumPad{
  public:
    void Init(int analogInput, byte note, int baseThreshold, float thresholdSlope, int sensitivity, int scanTime, int maskTime, float velocityExponent, float minVelocity);
    void UpdateReadValue();
    int GetState(unsigned long currentTime);
    bool PadSleeps();
    void CheckIfWakeUp(unsigned long currentTime); 
    void Playing(bool isPlaying);
    void Sleeping(bool isSleeping);
    void SetScanTimer(unsigned long currentTime);
    void ResetScanTimer();
    void SetMaskTimer(unsigned long currentTime);
    void ResetCounters();
    void AddValue();
    void SetThreshold();
    void DecreaseThreshold();
    byte Velocity();
    byte Note();               
  private:
    byte _note;
    int _analogInput;    
    int _readValue;
    
  //Params       
    int _baseThreshold;
    float _thresholdSlope;
    int _sensitivity;
    int _scanTime;
    int _maskTime;
    float _velocityExponent;
    float _minVelocity;   
    
  //Helpers  
    bool _isPlaying;
    bool _padSleeps;
    float _currentThreshold;
    int _maxValue;
    int _sumValue;
    int _numberOfCounts;
    unsigned long _scanTimer;
    unsigned long _maskTimer;       
};
#endif

