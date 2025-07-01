/**
 * Buzzer.cpp - 增强版蜂鸣器控制库实现
 */

#include "Buzzer.h"

Buzzer::Buzzer(uint8_t pin, int8_t volumePin) {
  _pin = pin;
  _volumePin = volumePin;
  _volumeLevel = 10; // 默认最大音量
  _isActive = false;
}

void Buzzer::begin() {
  pinMode(_pin, OUTPUT);
  if (_volumePin >= 0) {
    pinMode(_volumePin, OUTPUT);
  }
  stop();
}

void Buzzer::setVolume(uint8_t level) {
  // 限制音量范围在0-50
  _volumeLevel = constrain(level, 0, 10);
}

uint8_t Buzzer::getVolume() {
  return _volumeLevel;
}

void Buzzer::_applyVolume(uint8_t volume) {
  // 仅当音量控制引脚有效时应用音量
  if (_volumePin >= 0) {
    // 将0-20的音量映射到0-255的PWM值
    int pwmValue = map(volume, 0, 50, 0, 255);
    analogWrite(_volumePin, pwmValue);
  }
}

void Buzzer::beep(unsigned int duration, unsigned int frequency, int8_t volume) {
  uint8_t volumeToUse = (volume >= 0) ? volume : _volumeLevel;
  
  if (volumeToUse == 0) {
    delay(duration); // 音量为0时不发声，但保持时间延迟
    return;
  }
  
  if (_volumePin >= 0) {
    _applyVolume(volumeToUse);
    tone(_pin, frequency);
    delay(duration);
    noTone(_pin);
  } else {
    // 没有专用音量控制引脚时，使用软件PWM模拟音量
    _toneWithVolume(_pin, frequency, duration, volumeToUse);
  }
}

void Buzzer::longBeep(unsigned int duration, unsigned int frequency, int8_t volume) {
  uint8_t volumeToUse = (volume >= 0) ? volume : _volumeLevel;
  
  if (volumeToUse == 0) {
    delay(duration);
    return;
  }
  
  if (_volumePin >= 0) {
    _applyVolume(volumeToUse);
    tone(_pin, frequency);
    delay(duration);
    noTone(_pin);
  } else {
    _toneWithVolume(_pin, frequency, duration, volumeToUse);
  }
}

void Buzzer::sweepTone(unsigned int duration, unsigned int startFreq, unsigned int endFreq, unsigned int steps, int8_t volume) {
  uint8_t volumeToUse = (volume >= 0) ? volume : _volumeLevel;
  
  if (volumeToUse == 0) {
    delay(duration);
    return;
  }
  
  unsigned int stepDuration = duration / steps;
  float freqStep = (float)(endFreq - startFreq) / steps;
  
  for (unsigned int i = 0; i < steps; i++) {
    unsigned int currentFreq = startFreq + (i * freqStep);
    
    if (_volumePin >= 0) {
      _applyVolume(volumeToUse);
      tone(_pin, currentFreq);
      delay(stepDuration);
    } else {
      _toneWithVolume(_pin, currentFreq, stepDuration, volumeToUse);
    }
  }
  
  if (_volumePin >= 0) {
    noTone(_pin);
  }
}

void Buzzer::stop() {
  noTone(_pin);
  if (_volumePin >= 0) {
    analogWrite(_volumePin, 0);
  }
  _isActive = false;
}

void Buzzer::playMelody(unsigned int melody[], unsigned int durations[], unsigned int length, int8_t volume) {
  uint8_t volumeToUse = (volume >= 0) ? volume : _volumeLevel;
  
  if (volumeToUse == 0) {
    // 计算总持续时间并延迟
    unsigned int totalDuration = 0;
    for (unsigned int i = 0; i < length; i++) {
      totalDuration += durations[i] + 50; // 包括音符间隔
    }
    delay(totalDuration);
    return;
  }
  
  for (unsigned int i = 0; i < length; i++) {
    if (_volumePin >= 0) {
      _applyVolume(volumeToUse);
      tone(_pin, melody[i]);
      delay(durations[i]);
      noTone(_pin);
    } else {
      _toneWithVolume(_pin, melody[i], durations[i], volumeToUse);
    }
    delay(50); // 音符之间的短暂停顿
  }
}

void Buzzer::fadeVolume(unsigned int duration, uint8_t startVolume, uint8_t endVolume, unsigned int frequency) {
  startVolume = constrain(startVolume, 0, 50);
  endVolume = constrain(endVolume, 0, 50);
  
  unsigned int steps = 5; // 音量变化的步数
  unsigned int stepDuration = duration / steps;
  float volumeStep = (float)(endVolume - startVolume) / steps;
  
  for (unsigned int i = 0; i < steps; i++) {
    uint8_t currentVolume = startVolume + (i * volumeStep);
    
    if (_volumePin >= 0) {
      _applyVolume(currentVolume);
      tone(_pin, frequency);
      delay(stepDuration);
    } else {
      _toneWithVolume(_pin, frequency, stepDuration, currentVolume);
    }
  }
  
  if (_volumePin >= 0) {
    noTone(_pin);
  }
}

void Buzzer::_toneWithVolume(uint8_t pin, unsigned int frequency, unsigned long duration, uint8_t volume) {
  // 将音量(0-10)转换为占空比(0-100%)
  int dutyCycle = map(volume, 0, 50, 0, 100);
  
  // 计算一个周期的微秒数
  unsigned long periodMicros = 1000000 / frequency;
  
  // 计算高电平持续的微秒数
  unsigned long highMicros = periodMicros * dutyCycle / 100;
  
  // 计算低电平持续的微秒数
  unsigned long lowMicros = periodMicros - highMicros;
  
  // 计算需要多少个周期
  unsigned long cycles = (duration * 1000) / periodMicros;
  
  // 生成指定频率和音量的方波
  for (unsigned long i = 0; i < cycles; i++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(highMicros);
    digitalWrite(pin, LOW);
    delayMicroseconds(lowMicros);
  }
}