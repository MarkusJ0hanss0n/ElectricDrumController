#include "SevSeg.h"

SevSeg::SevSeg() {}

void SevSeg::Begin(boolean mode_in, byte numOfDigits,
  byte dig1, byte dig2, byte segA, byte segB, byte segC, byte segD, byte segE,
  byte segF, byte segG, byte segDP)
{
  numberOfDigits = numOfDigits;
  digit1 = dig1;
  digit2 = dig2;
  segmentA = segA;
  segmentB = segB;
  segmentC = segC;
  segmentD = segD;
  segmentE = segE;
  segmentF = segF;
  segmentG = segG;
  segmentDP = segDP;

  mode = mode_in;
  if (mode == COMMON_ANODE)
  {
    DigitOn = HIGH;
    DigitOff = LOW;
    SegOn = LOW;
    SegOff = HIGH;
  }
  else
  {
    DigitOn = LOW;
    DigitOff = HIGH;
    SegOn = HIGH;
    SegOff = LOW;
  }

  DigitPins[0] = digit1;
  DigitPins[1] = digit2;
  SegmentPins[0] = segmentA;
  SegmentPins[1] = segmentB;
  SegmentPins[2] = segmentC;
  SegmentPins[3] = segmentD;
  SegmentPins[4] = segmentE;
  SegmentPins[5] = segmentF;
  SegmentPins[6] = segmentG;
  SegmentPins[7] = segmentDP;

  for (byte digit = 0 ; digit < numberOfDigits ; digit++)
  {
    digitalWrite(DigitPins[digit], DigitOff);
    pinMode(DigitPins[digit], OUTPUT);
  }

  for (byte seg = 0 ; seg < 8 ; seg++)
  {
    digitalWrite(SegmentPins[seg], SegOff);
    pinMode(SegmentPins[seg], OUTPUT);
  }

  brightnessDelay = FRAMEPERIOD;
}

void SevSeg::DisplayString(const char* toDisplay)
{
  for (byte digit = 1 ; digit < (numberOfDigits + 1) ; digit++)
  {
    switch (digit)
    {
      case 1:
        digitalWrite(digit1, DigitOn);
        break;
      case 2:
        digitalWrite(digit2, DigitOn);
        break;
    }

    unsigned char characterToDisplay = toDisplay[digit - 1];

    const uint8_t chr = pgm_read_byte(&characterArray[characterToDisplay]);
    for (byte seg = 0 ; seg < 7 ; seg++)
    {
      if (chr & (1 << seg)) {
        digitalWrite(SegmentPins[6 - seg], SegOn);
      }
      else {
        digitalWrite(SegmentPins[6 - seg], SegOff);
      }
    }

    delayMicroseconds(brightnessDelay + 1);

    digitalWrite(segmentA, SegOff);
    digitalWrite(segmentB, SegOff);
    digitalWrite(segmentC, SegOff);
    digitalWrite(segmentD, SegOff);
    digitalWrite(segmentE, SegOff);
    digitalWrite(segmentF, SegOff);
    digitalWrite(segmentG, SegOff);
    digitalWrite(segmentDP, SegOff);

    switch (digit)
    {
      case 1:
        digitalWrite(digit1, DigitOff);
        break;
      case 2:
        digitalWrite(digit2, DigitOff);
        break;
    }

    delayMicroseconds(FRAMEPERIOD - brightnessDelay + 1);
  }
}

void SevSeg::DisplayInt(int value) {
  if (value < 0) value = 0;
  if (value > 99) value = 99;
  if (value < 10) {
    char tmpArr[] = {0, value};  
    DisplayString(tmpArr);
  }
  else {
    DisplayString(itoa(value, cstr, 10));
  }
}

