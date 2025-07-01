#include "KeyLib.h"

KeyLib::KeyLib(unsigned long debounceTime) {
  debounceDelay = debounceTime;
  keyCount = 0;
  
  // 初始化所有按键状态
  for (int i = 0; i < MAX_KEYS; i++) {
    keyStates[i] = {false, false, 0, 0, 0, false, false, 0};
  }
}

int KeyLib::getKeyIndex(uint8_t pin) {
  // 查找已存在的按键
  for (int i = 0; i < keyCount; i++) {
    if (i == pin) return i;
  }
  
  // 如果不存在，创建新的按键状态
  if (keyCount < MAX_KEYS) {
    pinMode(pin, INPUT_PULLUP);  // 默认使用上拉输入模式
    int index = keyCount;
    keyCount++;
    return index;
  }
  
  // 如果超过最大按键数，返回第一个按键的索引
  return 0;
}

void KeyLib::updateKeyState(uint8_t pin) {
  int index = getKeyIndex(pin);
  KeyState &state = keyStates[index];
  
  // 读取当前按键状态 (LOW表示按下，因为使用上拉电阻)
  bool reading = digitalRead(pin) == LOW;
  
  // 检查按键状态是否发生变化
  if (reading != state.lastState) {
    state.lastDebounceTime = millis();
  }
  
  // 如果状态稳定超过消抖时间
  if ((millis() - state.lastDebounceTime) > debounceDelay) {
    // 如果当前状态与之前记录的状态不同
    if (reading != state.currentState) {
      state.currentState = reading;
      
      // 按键按下
      if (state.currentState == true) {
        state.lastPressTime = millis();
        state.pressCount++;
        state.pressHandled = false;
      } 
      // 按键释放
      else {
        state.lastReleaseTime = millis();
        if (!state.pressHandled) {
          state.singlePressDetected = true;
        }
      }
    }
  }
  
  state.lastState = reading;
}

bool KeyLib::singlePress(uint8_t pin) {
  updateKeyState(pin);
  int index = getKeyIndex(pin);
  KeyState &state = keyStates[index];
  
  // 检测到单击并且未处理
  if (state.singlePressDetected) {
    state.singlePressDetected = false;
    state.pressHandled = true;
    return true;
  }
  
  return false;
}

bool KeyLib::doublePress(uint8_t pin, unsigned long doublePressTime) {
  updateKeyState(pin);
  int index = getKeyIndex(pin);
  KeyState &state = keyStates[index];
  
  // 检查是否在指定时间内有两次按下
  if (state.pressCount >= 2) {
    unsigned long timeBetweenPresses = state.lastPressTime - state.lastReleaseTime;
    if (timeBetweenPresses <= doublePressTime) {
      state.pressCount = 0;
      state.pressHandled = true;
      state.singlePressDetected = false;
      return true;
    }
    
    // 如果超时，重置计数
    if ((millis() - state.lastPressTime) > doublePressTime) {
      state.pressCount = 0;
    }
  }
  
  return false;
}

bool KeyLib::longPress(uint8_t pin, unsigned long longPressTime) {
  updateKeyState(pin);
  int index = getKeyIndex(pin);
  KeyState &state = keyStates[index];
  
  // 按键当前处于按下状态且持续时间超过长按时间
  if (state.currentState == true && !state.pressHandled) {
    if ((millis() - state.lastPressTime) >= longPressTime) {
      state.pressHandled = true;
      state.singlePressDetected = false;
      return true;
    }
  }
  
  return false;
}