#include <TFT_eSPI.h>
#include <TFT_Menu.h>
#include <ESP32Encoder.h>
#include <KeyLib.h>
#include <Buzzer.h>
#include <Arduino.h>
#include <esp32-hal.h>

#define BUZZ_VPIN 25
#define BUZZ_PIN 32

const int BTN_SELECT = 16;
unsigned long lastPressTime = 0;
const unsigned long doubleClickDelay = 300;
bool lastButtonState = HIGH;

//--------------------------declar---------------------------

ESP32Encoder encoder;
TFT_eSPI tft = TFT_eSPI();
KeyLib keyLib(50);
Buzzer buzzer(BUZZ_PIN,BUZZ_VPIN); // Buzzer pin
MenuSystem menu(&tft,&buzzer); // 菜单对象

void AnimationCallback(uint8_t animationType);
void BuzzCallback();
void DelayCallBack();
void StepCallBack();
//--------------------------menu items---------------------------
MenuItem main_menu_items[2] = {                                                                                                        
  MenuItem("Main", NULL),
  MenuItem("Setting", NULL),
};
  
MenuItem set_menu_items[3] = {
  MenuItem("Animation", NULL), 
  MenuItem("Buzz vol",BuzzCallback),  // 电压范围设置
  MenuItem("Back", []() { menu.back(); }),
};

MenuItem Anim_menu_items[4] = {
  MenuItem("Form",NULL),
  MenuItem("Para",NULL),
  MenuItem("Layout",NULL),
  MenuItem("Back", []() { menu.back(); }),
};

MenuItem layout_menu_items[3] = {
  MenuItem("Landscape",[](){tft.setRotation(1);
  menu.drawMenu(1);}),
  MenuItem("Portrait",[](){tft.setRotation(2);
  menu.drawMenu(1);}),
  MenuItem("Back", []() { menu.back();}),
};

MenuItem Form_menu_items[3] = {
  MenuItem("Stable",[](){AnimationCallback(1);}),
  MenuItem("Bounce",[](){AnimationCallback(2);}),
  MenuItem("Back", []() { menu.back(); }),
};

MenuItem Para_menu_items[3] = {
  MenuItem("Delay Time",DelayCallBack),
  MenuItem("Step",StepCallBack),
  MenuItem("Back", []() { menu.back(); }),
};

//--------------------------main---------------------------
void setup() {
  Serial.begin(115200); // 用于调试
  // 初始化TFT显示屏
  tft.init();
  tft.setRotation(2);	
  // 初始化编码器
  encoder.attachSingleEdge(14, 36);  // 使用 CLK=GPIO18, DT=GPIO16
  encoder.setCount(0);  // 初始计数值设为 0

  pinMode(BTN_SELECT, INPUT_PULLUP);  // 设置摁钮引脚为输入模式
  // 配置菜单外观
  menu.setBackgroundColor(TFT_BLACK);
  menu.setMenuBgColor(TFT_BLACK);
  menu.setHighlightColor(TFT_WHITE);
  menu.setTextColor(TFT_WHITE);
  menu.setSelectedTextColor(TFT_BLACK);
  menu.setTitleColor(TFT_WHITE);
  menu.setBorderColor(TFT_DARKGREY);

  
  // // 配置菜单字体和动画
  // menu.setMenuFontSize(1);  // 使用更大的字体
  // menu.setTitleFontSize(2);
  // 设置子菜单
  main_menu_items[1].setSubMenu(set_menu_items, 3);
  set_menu_items[0].setSubMenu(Anim_menu_items, 4);
  Anim_menu_items[0].setSubMenu(Form_menu_items, 3);
  Anim_menu_items[1].setSubMenu(Para_menu_items, 3);
  Anim_menu_items[2].setSubMenu(layout_menu_items, 3);
  // 设置根菜单
  menu.setRootMenu(main_menu_items, 2);
  menu.setSliderDisplayMode(SLIDER_DISPLAY_FOLLOW_SELECTION);

  
  // 初始显示菜单
  menu.drawMenu(0);
}

void loop() {

    // 菜单模式
    static long lastCount = 0;
    long currentCount = encoder.getCount();

    if (currentCount > lastCount) {
      // 编码器向前旋转
      menu.selectNext();
      lastCount = currentCount;
      delay(70);  // 防抖 
    }

    if (currentCount < lastCount) {
      // 编码器向后旋转
      menu.selectPrev();
      lastCount = currentCount;
      delay(70);  // 防抖
    }

    // 处理按钮输入
    if (keyLib.singlePress(BTN_SELECT)) {
      menu.select();
      delay(200);  // 防抖
    }

    if (keyLib.longPress(BTN_SELECT, 700)) {
      menu.back();
      delay(200);  // 防抖     
    }
    
    // 更新菜单动画
    menu.update();
}



void AnimationCallback(uint8_t animationType) {
  menu.setSliderAnimationForm(animationType);
}

void BuzzCallback() {
  menu.TypeNum = 1;
}

void DelayCallBack() {
  menu.TypeNum = 2;
}

void StepCallBack() {
  menu.TypeNum = 2;
}