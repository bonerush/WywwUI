/**
 * Buzzer.h - 增强版蜂鸣器控制库
 * 支持短响、长响、频率可变和响度可调的响声
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
  public:
    /**
     * 构造函数
     * @param pin 蜂鸣器连接的引脚
     * @param volumePin 音量控制引脚(PWM引脚)，设为-1表示不使用音量控制
     */
    Buzzer(uint8_t pin, int8_t volumePin = -1);
    
    /**
     * 初始化蜂鸣器
     */
    void begin();
    
    /**
     * 设置响度级别
     * @param level 响度级别(0-50)，0为静音，20为最大音量
     */
    void setVolume(uint8_t level);
    
    /**
     * 获取当前响度级别
     * @return 当前响度级别(0-50)
     */
    uint8_t getVolume();
    
    /**
     * 发出短促的蜂鸣声
     * @param duration 持续时间(毫秒)
     * @param frequency 频率(Hz)，默认2000Hz
     * @param volume 临时响度级别(0-50)，默认使用当前设置的响度
     */
    void beep(unsigned int duration, unsigned int frequency = 2000, int8_t volume = -1);
    
    /**
     * 发出长时间的蜂鸣声
     * @param duration 持续时间(毫秒)
     * @param frequency 频率(Hz)，默认2000Hz
     * @param volume 临时响度级别(0-50)，默认使用当前设置的响度
     */
    void longBeep(unsigned int duration, unsigned int frequency = 2000, int8_t volume = -1);
    
    /**
     * 发出频率变化的蜂鸣声
     * @param duration 总持续时间(毫秒)
     * @param startFreq 起始频率(Hz)
     * @param endFreq 结束频率(Hz)
     * @param steps 频率变化的步数
     * @param volume 临时响度级别(0-50)，默认使用当前设置的响度
     */
    void sweepTone(unsigned int duration, unsigned int startFreq, unsigned int endFreq, unsigned int steps, int8_t volume = -1);
    
    /**
     * 停止蜂鸣
     */
    void stop();
    
    /**
     * 播放简单的音乐旋律
     * @param melody 频率数组
     * @param durations 对应的持续时间数组
     * @param length 数组长度
     * @param volume 临时响度级别(0-50)，默认使用当前设置的响度
     */
    void playMelody(unsigned int melody[], unsigned int durations[], unsigned int length, int8_t volume = -1);
    
    /**
     * 渐变音量
     * @param duration 渐变持续时间(毫秒)
     * @param startVolume 起始音量(0-50)
     * @param endVolume 结束音量(0-50)
     * @param frequency 频率(Hz)
     */
    void fadeVolume(unsigned int duration, uint8_t startVolume, uint8_t endVolume, unsigned int frequency = 1000);
    
  private:
    uint8_t _pin;           // 蜂鸣器引脚
    int8_t _volumePin;      // 音量控制引脚(PWM)
    uint8_t _volumeLevel;   // 当前音量级别(0-50)
    bool _isActive;         // 蜂鸣器是否激活
    
    /**
     * 应用音量设置
     * @param volume 音量级别(0-20)
     */
    void _applyVolume(uint8_t volume);
    
    /**
     * 使用软件PWM产生可控音量的音调
     * @param pin 蜂鸣器引脚
     * @param frequency 频率(Hz)
     * @param duration 持续时间(毫秒)
     * @param volume 音量级别(0-50)
     */
    void _toneWithVolume(uint8_t pin, unsigned int frequency, unsigned long duration, uint8_t volume);
};

#endif