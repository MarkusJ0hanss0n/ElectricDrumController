#ifndef DrumPad_H
#define DrumPad_H
#include <Arduino.h>

class DrumPad{
  public:
    void Init(int analogInput, byte note, int threshold, int sensitivity, int scanTime, int maskTime);
    void UpdateReadValue();
    int GetState(unsigned long currentTime);
    bool PadSleeps();
    void CheckIfWakeUp(unsigned long currentTime); 
    void Playing(bool isPlaying);
    void SetScanTimer(unsigned long currentTime);
    void ResetScanTimer();
    void SetMaskTimer(unsigned long currentTime);
    void ResetCounters();
    void AddValue();
    byte Velocity(); 
    byte Note();           
  private:
    int _analogInput;
    byte _note;
    int _readValue;
    
  //Params    
    int _threshold;
    int _sensitivity;
    int _scanTime;
    int _maskTime;   
    
  //Helpers  
    bool _isPlaying;
    bool _padSleeps;
    int _sumValue;
    int _numberOfCounts;
    unsigned long _scanTimer;
    unsigned long _maskTimer;       
};
#endif

