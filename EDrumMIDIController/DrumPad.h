#ifndef DrumPad_H
#define DrumPad_H
#include <Arduino.h>

class DrumPad{
  public:
    void Init(int analogInput, byte note);
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
    int GetReadValue();
    int GetThreshold();
    int GetParamValue(int paramIndex);
    void SetParamValue(int paramIndex, int value);
  //Analyze only    
    int GetNumberOfCounts();
    int GetSumValue();               
  private:
    byte _note;
    int _analogInput;    
    int _readValue;
    
  //Params       
    int _baseThreshold;
    int _thresholdSlope;
    int _scanTime;
    int _maskTime;
    int _velocityExponent;
    int _minVelocity;   
    
  //Helpers  
    bool _isPlaying;
    bool _padSleeps;
    int _currentThreshold;
    int _maxValue;
    int _sumValue;
    int _numberOfCounts;
    unsigned long _scanTimer;
    unsigned long _maskTimer; 
};
#endif

