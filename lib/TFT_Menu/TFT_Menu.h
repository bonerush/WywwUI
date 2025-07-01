#ifndef TFT_MENU_H
#define TFT_MENU_H

#include <Arduino.h>
#include <TFT_eSPI.h> // Ensure TFT_eSPI library is installed and configured
#include "Buzzer.h"   // Ensure you have defined the Buzzer class

/**
 * @brief Structure to store rectangle information for menu items.
 *        (Legacy from older versions; slider animation primarily uses AnimationState and RectF now).
 */
struct MenuItemRect {
  int x, y, width, height;
  bool valid;
};

/**
 * @brief Structure to store animation's current and target states,
 *        along with velocity and error terms.
 */
struct AnimationState {
  float x_cur, y_cur, w_cur, h_cur; // Current position and dimensions
  float x_tgt, y_tgt, w_tgt, h_tgt; // Target position and dimensions
  float x_vel, y_vel, w_vel, h_vel; // Velocity (for underdamped animation)
  float x_err, y_err, w_err, h_err; // Error (for integral controller animation)
};

/**
 * @brief Structure for floating-point rectangles, useful for animation calculations.
 *        Primarily used to define target rectangles.
 */
struct RectF {
  float x, y, width, height;
};

/**
 * @brief Enum defining slider display modes.
 */
enum SliderDisplayMode {
  SLIDER_DISPLAY_FOLLOW_SELECTION, // Slider follows the selected item, scrolling the view.
  SLIDER_DISPLAY_FIXED_TOP         // Selected item is forced to the top of the menu area; slider is fixed.
};

//------------------------------------MenuItem Class------------------------------------//
/**
 * @brief Represents a single item within the menu system.
 *        Can have a callback function or a submenu.
 */
class MenuItem {
public:
  /**
   * @brief Constructor for a MenuItem.
   * @param _label The text label for the menu item.
   * @param _callback A pointer to a function to execute when this item is selected. Can be NULL if it's a submenu parent.
   */
  MenuItem(String _label, void (*_callback)());

  /**
   * @brief Sets a submenu for this menu item.
   * @param _subMenu Pointer to an array of MenuItem objects representing the submenu.
   * @param _size The number of items in the submenu.
   */
  void setSubMenu(MenuItem* _subMenu, uint8_t _size);

  /**
   * @brief Gets the label of the menu item.
   * @return The label string.
   */
  String getLabel();

  /**
   * @brief Gets the callback function associated with this menu item.
   * @return A pointer to the callback function.
   */
  void (*getCallback())();

  /**
   * @brief Gets the submenu associated with this menu item.
   * @return A pointer to the MenuItem array of the submenu, or NULL if none.
   */
  MenuItem* getSubMenu();

  /**
   * @brief Gets the size of the submenu.
   * @return The number of items in the submenu.
   */
  uint8_t getSubMenuSize();

  /**
   * @brief Checks if this menu item has a submenu.
   * @return True if it has a submenu, false otherwise.
   */
  bool hasSubMenu();

private:
  String label;
  void (*callback)();
  MenuItem* subMenu;
  uint8_t subMenuSize;
};

//------------------------------------MenuSystem Class------------------------------------//
/**
 * @brief Manages the display and navigation of a hierarchical menu system on a TFT display.
 */
class MenuSystem {
public:
  /**
   * @brief Constructor for the MenuSystem.
   * @param _tft Pointer to an initialized TFT_eSPI object.
   * @param _buzzer Pointer to an initialized Buzzer object.
   */
  MenuSystem(TFT_eSPI* _tft, Buzzer* _buzzer);

  /**
   * @brief Destructor for the MenuSystem.
   */
  ~MenuSystem();

  uint8_t TypeNum = 0; // Public flag for custom animation types triggered by callbacks
  uint16_t screenWidth;
  uint16_t screenHeight;
  
  /**
   * @brief Initializes the buzzer.
   */
  void buzzer_begin();

  // Menu Operations
  /**
   * @brief Sets the root menu for the system.
   * @param menu Pointer to the array of MenuItem objects for the root menu.
   * @param size The number of items in the root menu.
   */
  void setRootMenu(MenuItem* menu, uint8_t size);

  /**
   * @brief Moves the selection to the next menu item.
   */
  void selectNext();

  /**
   * @brief Moves the selection to the previous menu item.
   */
  void selectPrev();

  /**
   * @brief Confirms the selection of the current menu item, executing its callback or entering its submenu.
   */
  void select();

  /**
   * @brief Returns to the previous menu level.
   */
  void back();

  /**
   * @brief Updates the menu display, including animations.
   *        This function should be called frequently in the main loop.
   */
  void update();

  // Style Settings
  void setBackgroundColor(uint16_t color);
  void setMenuBgColor(uint16_t color);
  void setHighlightColor(uint16_t color);
  void setTextColor(uint16_t color);
  void setSelectedTextColor(uint16_t color);
  void setTitleColor(uint16_t color);
  void setBorderColor(uint16_t color);
  void setMenuFontSize(uint8_t size);
  void setTitleFontSize(uint8_t size);

  /**
   * @brief Draws the entire menu system.
   * @param forceRedraw If true, forces a complete redraw of the screen.
   */
  void drawMenu(bool forceRedraw);

  // Slider Animation Control API
  /**
   * @brief Sets the slider display mode.
   * @param mode The desired display mode (SLIDER_DISPLAY_FOLLOW_SELECTION or SLIDER_DISPLAY_FIXED_TOP).
   */
  void setSliderDisplayMode(SliderDisplayMode mode);

  /**
   * @brief Sets custom target position and size for the slider.
   *        These values override default calculations only in SLIDER_DISPLAY_FIXED_TOP mode.
   * @param x Custom X coordinate, -1 to use default.
   * @param y Custom Y coordinate, -1 to use default.
   * @param width Custom width, -1 to use default.
   * @param height Custom height, -1 to use default.
   */
  void setCustomSliderTarget(int x = -1, int y = -1, int width = -1, int height = -1); 

  /**
   * @brief Sets the slider animation form.
   * @param form 1 for Precise Control (integral controller), 2 for Underdamped.
   */
  void setSliderAnimationForm(uint8_t form);

  /**
   * @brief Sets the duration of slider animations in milliseconds.
   * @param duration Animation duration in ms.
   */
  void setSliderAnimationDuration(uint16_t duration);

  /**
   * @brief Sets the update interval for slider animations in milliseconds.
   *        A smaller interval results in smoother animation but higher CPU usage.
   * @param interval Animation update interval in ms.
   */
  void setSliderAnimationInterval(uint16_t interval);
 
  // Get Current State
  /**
   * @brief Gets the current menu level.
   * @return The current menu level (0 for root menu).
   */
  uint8_t getCurrentLevel();

  /**
   * @brief Gets the index of the currently selected menu item.
   * @return The selected item's index.
   */
  uint8_t getSelectedIndex();

private:
  TFT_eSPI* tft;
  Buzzer* buzzer;
  uint8_t buzz_vol = 5; // Buzzer volume

  MenuItem* currentMenu;      // Array of current menu items
  uint8_t currentMenuSize;    // Number of items in the current menu
  uint8_t selectedIndex;      // Index of the currently selected item
  uint8_t startIndex;         // Starting index of visible menu items (for scrolling)
  uint8_t menuLevel;          // Current menu depth

  // Menu history, used for navigating back to parent menus
  MenuItem* menuHistory[10];
  uint8_t menuSizeHistory[10];
  uint8_t selectedIndexHistory[10];

  // Layout Parameters (calculated based on screen size and font settings)
  int16_t actualTitleAreaHeight;
  uint8_t titleTextX;
  uint8_t titleTextY;
  uint8_t titleDecoratorW;
  uint8_t titleDecoratorH;
  uint8_t titleDecoratorX;
  uint8_t titleDecoratorY;
  uint8_t titleBottomMargin;

  int16_t actualMenuItemHeight;
  uint8_t actualMenuItemSpacing;
  uint8_t menuItemsAreaY;
  uint8_t menuItemsXOffset;
  uint8_t menuItemTextXPadding;
  uint8_t menuItemTextYOffset;
  uint8_t menuItemCornerRadius;
  uint8_t menuItemBorderOffset;
  uint8_t menuItemArrowWidth;
  uint8_t menuItemArrowMarginX;
  uint8_t menuItemDefaultMaxWidth;
  uint8_t itemDecoratorW;
  uint8_t itemDecoratorH;
  uint8_t itemDecoratorX;

  uint8_t actualMaxDisplayItems; // Maximum number of menu items that can be displayed on screen

  uint8_t scrollbarW;
  uint8_t scrollbarX;
  uint8_t scrollbarY;
  uint8_t scrollbarH;
  uint8_t scrollbarThumbMinHeight;

  // Color Settings
  uint16_t backgroundColor;
  uint16_t menuBgColor;
  uint16_t highlightColor;
  uint16_t textColor;
  uint16_t selectedTextColor;
  uint16_t titleColor;
  uint16_t borderColor;

  // Font Sizes
  uint8_t menuFontSize;
  uint8_t titleFontSize;

  // Animation Parameters
  AnimationState sliderAnim; // Slider animation state
  bool animationActive;      // Flag indicating if slider animation is active
  unsigned long lastAnimTime; // Last slider animation update time
  uint16_t animDuration;     // Slider animation duration (ms)
  uint16_t animInterval;     // Slider animation update interval (ms)
  uint8_t _animationForm;    // Slider animation type (1: Precise Control, 2: Underdamped)
  bool type = false;         // Flag to determine if it's a window animation or item animation
  bool BanOperation = false; // Flag to ban user operations during specific animations/states

  // Title Decorator Animation Parameters
  AnimationState titleDecoratorAnim; // Title decorator animation state
  bool titleDecoratorAnimationActive; // Flag indicating if title decorator animation is active
  unsigned long lastTitleDecoratorAnimTime; // Last title decorator animation update time
  int currentTitleDecoratorX; // Current X coordinate for drawing the title decorator

  // Anti-Flicker Optimization
  int8_t lastSelectedIndex;
  int8_t lastStartIndex;
  String lastTitle;
  bool needFullRedraw; // Flag indicating if a full screen redraw is needed
  MenuItemRect lastSelectedRect; // Rectangle of the last drawn slider, used for clearing

  // Slider Display Mode and Custom Parameters
  SliderDisplayMode _sliderDisplayMode; // Slider display mode
  bool _useCustomSliderPosition; // Flag to use custom position
  bool _useCustomSliderSize;     // Flag to use custom size
  int _customSliderX, _customSliderY, _customSliderWidth, _customSliderHeight; // Custom values

  // Internal Helper Functions
  /**
   * @brief Calculates all layout parameters based on screen dimensions and font sizes.
   *        Should be called after screen size or font size changes.
   */
  void calculateLayoutParameters();

  /**
   * @brief Calculates the width of a given text string using the specified font size.
   * @param text The string to measure.
   * @param fontSize The font size to use for measurement.
   * @return The width of the text in pixels.
   */
  int calculateTextWidth(String text, uint8_t fontSize);

  /**
   * @brief Calculates the width required for a menu item, including text, padding, and arrow.
   * @param index The index of the menu item.
   * @return The calculated width of the menu item.
   */
  int calculateItemWidth(uint8_t index);

  // Animation related
  /**
   * @brief Animates a single float value from its current to target, updating velocity and error.
   * @param current Pointer to the current value.
   * @param target Pointer to the target value.
   * @param velocity Pointer to the velocity term.
   * @param error Pointer to the error term (for integral controller).
   * @param deltaTime Time elapsed since the last update in seconds.
   * @return True if the animation is complete (current is very close to target), false otherwise.
   */
  bool animateSingleValue(float *current, float *target, float *velocity, float *error, float deltaTime);

  /**
   * @brief Updates the slider animation state.
   * @param type Flag to select between standard slider animation (0) or window animation (1).
   */
  void updateAnimation(bool type);

  /**
   * @brief Initiates a new slider animation to a specified target rectangle.
   * @param targetX Target X coordinate.
   * @param targetY Target Y coordinate.
   * @param targetWidth Target width.
   * @param targetHeight Target height.
   */
  void startAnimation(int targetX, int targetY, int targetWidth, int targetHeight);

  /**
   * @brief Calculates the slider's target rectangle based on the current display mode and selected index.
   * @param index The index of the menu item for which to calculate the target rectangle.
   * @return A RectF structure representing the target position and size.
   */
  RectF calculateSliderTargetRect(uint8_t index);

  /**
   * @brief Updates the title decorator animation state.
   */
  void updateTitleDecoratorAnimation();

  // Drawing related
  /**
   * @brief Draws the menu title.
   * @param forceRedraw If true, forces a redraw of the entire title area.
   * @param forceTextRedraw If true, forces a redraw of the title text and background, even if text hasn't changed.
   */
  void drawTitle(bool forceRedraw, bool forceTextRedraw = false);

  /**
   * @brief Draws the list of menu items. Only non-selected items are drawn by this function.
   * @param forceRedraw If true, forces a complete redraw of the visible menu items.
   */
  void drawMenuItems(bool forceRedraw);

  /**
   * @brief Draws a single menu item. This function is primarily responsible for drawing non-selected items.
   *        Selected items are drawn by `drawAnimatedSlider`.
   * @param index The index of the menu item to draw.
   * @param isSelected True if the item is currently selected.
   */
  void drawMenuItem(uint8_t index, bool isSelected);

  /**
   * @brief Draws the animated slider at its current animation position, including the selected item's content.
   */
  void drawAnimatedSlider();

  /**
   * @brief Draws an animated window, typically used for special callback animations.
   */
  void drawAnimatedWindow();

  /**
   * @brief Draws the scrollbar if the menu items exceed the visible display area.
   */
  void drawScrollbar();

  /**
   * @brief Updates rectangle information for a menu item. (Might be deprecated or unused in current animation logic).
   * @param index The index of the menu item.
   * @param selected True if the item is selected.
   * @param rect Reference to the MenuItemRect structure to update.
   */
  void updateRectInfo(uint8_t index, bool selected, MenuItemRect &rect);

  /**
   * @brief Clears a specified rectangular area on the screen. (Might be deprecated or unused in current animation logic).
   * @param x X coordinate.
   * @param y Y coordinate.
   * @param width Width of the area.
   * @param height Height of the area.
   */
  void clearMenuItem(int x, int y, int width, int height);

  /**
   * @brief Placeholder for memory optimization routines.
   *        In more complex systems, this might involve dynamic allocation/deallocation.
   */
  void optimizeMemoryUsage();
};

#endif // TFT_MENU_H