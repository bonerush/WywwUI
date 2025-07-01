#ifndef KEYLIB_H
#define KEYLIB_H

#include <Arduino.h>

class KeyLib {
  private:
    // 存储按键状态的变量
    struct KeyState {
      bool lastState;           // 上一次按键状态
      bool currentState;        // 当前按键状态
      unsigned long lastDebounceTime;  // 上次消抖时间
      unsigned long lastPressTime;     // 上次按下时间
      unsigned long lastReleaseTime;   // 上次释放时间
      bool pressHandled;        // 按键是否已处理
      bool singlePressDetected; // 单击检测标志
      int pressCount;           // 按下计数(用于双击)
    };

    // 按键状态映射
    static const int MAX_KEYS = 10;  // 最多支持的按键数
    KeyState keyStates[MAX_KEYS];
    int keyCount;

    // 消抖时间(毫秒)
    unsigned long debounceDelay;

    // 查找或创建按键状态
    int getKeyIndex(uint8_t pin);
    
    // 更新按键状态
    void updateKeyState(uint8_t pin);

  public:
    KeyLib(unsigned long debounceTime = 50);
    
    // 检测单击
    bool singlePress(uint8_t pin);
    
    // 检测双击
    bool doublePress(uint8_t pin, unsigned long doublePressTime = 300);
    
    // 检测长按
    bool longPress(uint8_t pin, unsigned long longPressTime = 1000);
};

#endif