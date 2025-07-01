#include "TFT_Menu.h"
#include <algorithm> // For std::max and std::min
#include <math.h>    // For fmod

// TypeNum is a member of MenuSystem, not a global extern variable.
// The original comment "extern int TypeNum;" was misleading.
// It's declared as a public member in TFT_Menu.h and accessed via the MenuSystem object.

//------------------------------------MenuItem Class Implementation------------------------------------//
/**
 * @brief Constructor for a MenuItem.
 * @param _label The text label for the menu item.
 * @param _callback A pointer to a function to execute when this item is selected. Can be NULL if it's a submenu parent.
 */
MenuItem::MenuItem(String _label, void (*_callback)()) {
  label = _label;
  callback = _callback;
  subMenu = NULL;
  subMenuSize = 0;
}

/**
 * @brief Sets a submenu for this menu item.
 * @param _subMenu Pointer to an array of MenuItem objects representing the submenu.
 * @param _size The number of items in the submenu.
 */
void MenuItem::setSubMenu(MenuItem* _subMenu, uint8_t _size) {
  subMenu = _subMenu;
  subMenuSize = _size;
}

/**
 * @brief Gets the label of the menu item.
 * @return The label string.
 */
String MenuItem::getLabel() { return label; }

/**
 * @brief Gets the callback function associated with this menu item.
 * @return A pointer to the callback function.
 */
void (*MenuItem::getCallback())() { return callback; }

/**
 * @brief Gets the submenu associated with this menu item.
 * @return A pointer to the MenuItem array of the submenu, or NULL if none.
 */
MenuItem* MenuItem::getSubMenu() { return subMenu; }

/**
 * @brief Gets the size of the submenu.
 * @return The number of items in the submenu.
 */
uint8_t MenuItem::getSubMenuSize() { return subMenuSize; }

/**
 * @brief Checks if this menu item has a submenu.
 * @return True if it has a submenu, false otherwise.
 */
bool MenuItem::hasSubMenu() { return subMenu != NULL; }

//------------------------------------MenuSystem Class Implementation------------------------------------//
/**
 * @brief Constructor for the MenuSystem.
 * @param _tft Pointer to an initialized TFT_eSPI object.
 * @param _buzzer Pointer to an initialized Buzzer object.
 */
MenuSystem::MenuSystem(TFT_eSPI* _tft, Buzzer* _buzzer) {
  tft = _tft;
  buzzer = _buzzer ; 
  screenWidth = tft->width();
  screenHeight = tft->height();

  currentMenu = NULL; // Current menu item array
  currentMenuSize = 0; // Number of items in the current menu
  selectedIndex = 0; // Index of the currently selected item
  startIndex = 0; // Starting index of visible menu items
  menuLevel = 0; // Current menu depth
  lastTitle = ""; // Last drawn title string

  titleBottomMargin = 10; // Margin below the title area
  lastSelectedRect.valid = false; // Last selected item rectangle (now used for slider clearing)

  //----------------Default Colors----------------//
  backgroundColor = TFT_BLACK;
  menuBgColor = TFT_BLACK; 
  highlightColor = TFT_WHITE;
  textColor = TFT_WHITE;
  selectedTextColor = TFT_BLACK;
  titleColor = TFT_WHITE;
  borderColor = TFT_DARKGREY;

  //----------------Default Font Sizes----------------//
  menuFontSize = 1; // Font size for menu items
  titleFontSize = 2; // Font size for the title

  //----------------Animation Parameters Initialization----------------//
  animationActive = false;
  lastAnimTime = 0;
  animDuration = 200; // ms
  animInterval = 15;  // ms (Target ~60fps)
  _animationForm = 1; // Default animation form is Precise Control mode

  // Slider animation state initialization
  // These initial values will be updated after calculateLayoutParameters(), but set defaults first
  sliderAnim.x_cur = menuItemsXOffset;
  sliderAnim.y_cur = menuItemsAreaY;
  sliderAnim.w_cur = screenWidth * 0.8; 
  sliderAnim.h_cur = actualMenuItemHeight; 

  sliderAnim.x_tgt = sliderAnim.x_cur;
  sliderAnim.y_tgt = sliderAnim.y_cur;
  sliderAnim.w_tgt = sliderAnim.w_cur;
  sliderAnim.h_tgt = sliderAnim.h_cur;

  sliderAnim.x_vel = sliderAnim.y_vel = sliderAnim.w_vel = sliderAnim.h_vel = 0.0f;
  sliderAnim.x_err = sliderAnim.y_err = sliderAnim.w_err = sliderAnim.h_err = 0.0f;

  // Title decorator animation state initialization
  titleDecoratorAnimationActive = false;
  lastTitleDecoratorAnimTime = 0;
  titleDecoratorAnim.x_cur = 0.0f; 
  titleDecoratorAnim.x_tgt = 0.0f;
  titleDecoratorAnim.x_vel = 0.0f;
  titleDecoratorAnim.x_err = 0.0f;
  currentTitleDecoratorX = 0; // Set in calculateLayoutParameters

  //----------------Anti-Flicker Initialization----------------//
  lastSelectedIndex = -1;
  lastStartIndex = -1;
  needFullRedraw = true;

  //----------------Slider Display Mode and Custom Parameters Initialization----------------//
  _sliderDisplayMode = SLIDER_DISPLAY_FOLLOW_SELECTION; // Default slider follows selection
  _useCustomSliderPosition = false;
  _useCustomSliderSize = false;
  _customSliderX = -1; _customSliderY = -1; _customSliderWidth = -1; _customSliderHeight = -1;

  calculateLayoutParameters(); // Calculate layout parameters
  // Ensure title decorator is in static position initially
  currentTitleDecoratorX = titleDecoratorX; 
  titleDecoratorAnim.x_cur = titleDecoratorX;
  titleDecoratorAnim.x_tgt = titleDecoratorX;
}

/**
 * @brief Destructor for the MenuSystem.
 */
MenuSystem::~MenuSystem() {
  // Destructor implementation (if any dynamic memory allocation was used)
}

/**
 * @brief Initializes the buzzer.
 */
void MenuSystem::buzzer_begin(){
  buzzer->begin();
  buzzer->setVolume(buzz_vol);
}

/**
 * @brief Calculates all layout parameters based on screen dimensions and font sizes.
 *        Should be called after screen size or font size changes.
 */
void MenuSystem::calculateLayoutParameters() {
    screenWidth = tft->width();
    screenHeight = tft->height();
    
    // Title area calculation
    tft->setTextSize(titleFontSize);
    int16_t titleFontActualHeight = tft->fontHeight(); // More accurate than 8*size for some fonts
    uint8_t titlePaddingY = std::max(5, titleFontActualHeight / 3); 
    actualTitleAreaHeight = titleFontActualHeight + 2 * titlePaddingY;
    titleTextX = screenWidth * 0.08; 
    titleTextY = titlePaddingY + (actualTitleAreaHeight - titleFontActualHeight) / 2;

    titleDecoratorW = screenWidth * 0.05; // Title decorator width
    titleDecoratorH = titleDecoratorW;
    titleDecoratorX = std::max(2, (int)(titleTextX / 2 - titleDecoratorW / 2)); 
    titleDecoratorY = titleTextY + titleFontActualHeight / 2 - titleDecoratorH / 2;
    
    // Menu item calculation
    tft->setTextSize(menuFontSize);
    int16_t menuItemFontActualHeight = tft->fontHeight();
    uint8_t menuItemVerticalPaddingInside = std::max(4, menuItemFontActualHeight / 2);
    actualMenuItemHeight = menuItemFontActualHeight + 2 * menuItemVerticalPaddingInside;
    actualMenuItemSpacing = std::max(5, actualMenuItemHeight / 4); 

    menuItemsAreaY = actualTitleAreaHeight + titleBottomMargin; // Y coordinate for menu items area
    menuItemsXOffset = 0; // X offset for menu items
    menuItemTextXPadding = screenWidth * 0.04; // X padding for menu item text
    menuItemTextYOffset = menuItemVerticalPaddingInside; // Y padding for menu item text

    menuItemCornerRadius = std::max(4, actualMenuItemHeight / 4);
    menuItemBorderOffset = std::max(1, (int)(menuItemCornerRadius / 4)); 
    menuItemArrowWidth = std::max(8, (int)(menuItemFontActualHeight * 0.6)); 
    menuItemArrowMarginX = std::max(3, (int)(screenWidth * 0.04)); 
    menuItemDefaultMaxWidth = screenWidth - menuItemsXOffset - (screenWidth * 0.05); // Max width for an item

    itemDecoratorW = screenWidth * 0.03;
    itemDecoratorH = std::max(1,(int)(itemDecoratorW / 5)); 
    itemDecoratorX = std::max(2, (int)(menuItemsXOffset / 2 - itemDecoratorW / 2)); 

    // Calculation for maximum displayable menu items
    int16_t usableMenuHeight = screenHeight - actualTitleAreaHeight;
    if ((actualMenuItemHeight + actualMenuItemSpacing) > 0) {
        actualMaxDisplayItems = usableMenuHeight / (actualMenuItemHeight + actualMenuItemSpacing);
    } else {
        actualMaxDisplayItems = 1; // Avoid division by zero, allow at least one item to be displayed
    }
    if (actualMaxDisplayItems == 0 && currentMenuSize > 0) actualMaxDisplayItems = 1;

    // Scrollbar calculation
    scrollbarW = std::max(3, (int)(screenWidth * 0.02));
    uint8_t scrollbarMarginFromEdge = std::max(3, (int)(screenWidth * 0.015));

    scrollbarX = screenWidth - scrollbarW - scrollbarMarginFromEdge;
    scrollbarY = menuItemsAreaY + scrollbarMarginFromEdge; 
    scrollbarH = screenHeight - menuItemsAreaY - 2 * scrollbarMarginFromEdge; 

    // Dynamic scrollbar thumb height calculation (with new ratio limits)
    float thumbRatio = (float)actualMaxDisplayItems / currentMenuSize;
    scrollbarThumbMinHeight = std::max( 
        (int)(scrollbarH * 0.08),  // Absolute minimum height: 8% of scrollbar height
        std::min(
            (int)(scrollbarH * thumbRatio), // Theoretical calculated value
            (int)(scrollbarH * 0.5)         // Max cap at 50% of height
        )
    );

    // Added: Scrollbar boundary protection
    if(scrollbarH < scrollbarThumbMinHeight){
        scrollbarThumbMinHeight = std::max(scrollbarH / 2, 10); // Ensure visibility in extreme cases
    }

    needFullRedraw = true;
}

/**
 * @brief Calculates the width of a given text string using the specified font size.
 * @param text The string to measure.
 * @param fontSize The font size to use for measurement.
 * @return The width of the text in pixels.
 */
int MenuSystem::calculateTextWidth(String text, uint8_t fontSize) {
  tft->setTextSize(fontSize);
  return tft->textWidth(text);
}

/**
 * @brief Calculates the width required for a menu item, including text, padding, and arrow.
 * @param index The index of the menu item.
 * @return The calculated width of the menu item.
 */
int MenuSystem::calculateItemWidth(uint8_t index) {
    if (!currentMenu || index >= currentMenuSize) return menuItemsXOffset + 50; 
    // If no menu items or index out of bounds, return a default width

    String itemText = currentMenu[index].getLabel();
    int textActualWidth = calculateTextWidth(itemText, menuFontSize);
    
    int desiredWidth = textActualWidth + 2 * menuItemTextXPadding; // Calculation: text width + left/right padding

    if (currentMenu[index].hasSubMenu()) {
        desiredWidth += menuItemArrowWidth + menuItemTextXPadding / 2; // Calculation: text width + padding + arrow width + half padding
    }
    
    int minItemWidth = screenWidth * 0.3; 
    return constrain(desiredWidth, minItemWidth, menuItemDefaultMaxWidth); // Return value constrained between min and max width
}

/**
 * @brief Animates a single float value from its current to target, updating velocity and error.
 * @param current Pointer to the current value.
 * @param target Pointer to the target value.
 * @param velocity Pointer to the velocity term.
 * @param error Pointer to the error term (for integral controller).
 * @param deltaTime Time elapsed since the last update in seconds.
 * @return True if the animation is complete (current is very close to target), false otherwise.
 */
bool MenuSystem::animateSingleValue(float *current, float *target, float *velocity, float *error, float deltaTime) {
  if (abs(*current - *target) < 0.5f) { // If the difference between current and target is less than 0.5
    *current = *target; // Set current position to target position
    *error = 0; // Clear error (primarily for _animationForm == 1)
    *velocity = 0.0f; // Velocity should also be zero when animation completes
    return true; // Animation complete
  }

  if (_animationForm == 1) { // Precise Control mode (Integral Controller)
    float step = (*target - *current) * (float)animInterval / animDuration; // Calculate step
    *current += step; // Update current position

    *error += (*target - *current); // Accumulate total error
    *current += *error / ((float)animDuration / animInterval); // Current position = Current position + Total error / (Total animation time / Animation interval)
    *error = fmod(*error, (float)animDuration / animInterval); // Calculate remainder of error to prevent it from growing too large
  } 
  else if (_animationForm == 2) { // Underdamped mode
    if (deltaTime <= 0.0f) { // If deltaTime is very small or zero, do not update to prevent division by zero or similar issues
        return false; // Time has not advanced, animation state remains unchanged
    }
    // Underdamped spring-mass-damper model parameters
    const float zeta = 0.5f; // Damping ratio
    const float omega_n = 15.0f; // Natural frequency
    float displacement = *current - *target;
    float acceleration = -2.0f * zeta * omega_n * (*velocity) - omega_n * omega_n * displacement;
    *velocity += acceleration * deltaTime;
    *current += (*velocity) * deltaTime;
  }
  return false; // Animation is still in progress
}

/**
 * @brief Updates the slider animation state.
 * @param type Flag to select between standard slider animation (0) or window animation (1).
 */
void MenuSystem::updateAnimation(bool type) {
  if (!animationActive) return; // If no animation is active, return directly
  
  unsigned long currentTime = millis(); // Get current time
  if (currentTime - lastAnimTime >= animInterval) { // If interval since last animation time is greater than or equal to animation interval
    float deltaTime = (currentTime - lastAnimTime) / 1000.0f; // Calculate time difference in seconds
    if (deltaTime > 0.1f) deltaTime = 0.1f; // Cap max step to 100ms to prevent large jumps after lag
    if (deltaTime <= 0.0f) deltaTime = (float)animInterval / 1000.0f; // Prevent zero or negative deltaTime

    lastAnimTime = currentTime; // Update last animation time

    int oldX = round(sliderAnim.x_cur);
    int oldY = round(sliderAnim.y_cur);
    int oldWidth = round(sliderAnim.w_cur);
    int oldHeight = round(sliderAnim.h_cur);

    bool xDone = animateSingleValue(&sliderAnim.x_cur, &sliderAnim.x_tgt, &sliderAnim.x_vel, &sliderAnim.x_err, deltaTime);
    bool yDone = animateSingleValue(&sliderAnim.y_cur, &sliderAnim.y_tgt, &sliderAnim.y_vel, &sliderAnim.y_err, deltaTime);
    bool wDone = animateSingleValue(&sliderAnim.w_cur, &sliderAnim.w_tgt, &sliderAnim.w_vel, &sliderAnim.h_err, deltaTime); 
    bool hDone = animateSingleValue(&sliderAnim.h_cur, &sliderAnim.h_tgt, &sliderAnim.h_vel, &sliderAnim.h_err, deltaTime);
    
    // Only redraw if slider position or size has actually changed
    if (round(sliderAnim.x_cur) != oldX || round(sliderAnim.y_cur) != oldY || 
        round(sliderAnim.w_cur) != oldWidth || round(sliderAnim.h_cur) != oldHeight) {
      
      // 1. Clear the background of the previous slider position
      if (lastSelectedRect.valid) {
         tft->fillRect(lastSelectedRect.x, lastSelectedRect.y, 
                       lastSelectedRect.width, lastSelectedRect.height, 
                       backgroundColor);
      }
      
      // 2. Redraw the previously selected item, now displayed as unselected
      // Ensure it's within the visible range and different from the current selected item (to avoid redrawing in place during animation)
      if (lastSelectedIndex != -1 && lastSelectedIndex != selectedIndex &&
          lastSelectedIndex >= startIndex && lastSelectedIndex < (startIndex + actualMaxDisplayItems)) {
          drawMenuItem(lastSelectedIndex, false); 
      }
      
      // 3. Draw the slider at its current animated position, including the selected item's content
      if(type == 0){drawAnimatedSlider(); }
      else{drawAnimatedWindow();}
      
      
      // 4. Update lastSelectedRect for clearing in the next frame
      lastSelectedRect.x = round(sliderAnim.x_cur) - menuItemBorderOffset;
      lastSelectedRect.y = round(sliderAnim.y_cur) - menuItemBorderOffset;
      lastSelectedRect.width = round(sliderAnim.w_cur) + 2 * menuItemBorderOffset;
      lastSelectedRect.height = round(sliderAnim.h_cur) + 2 * menuItemBorderOffset;
      lastSelectedRect.valid = true;
    }
    
    if (xDone && yDone && wDone && hDone) { // If all animations are complete
      animationActive = false;
      needFullRedraw = true; // Force a full redraw after animation ends to ensure a clean screen state
    }
  }
}

/**
 * @brief Draws the animated slider at its current animation position, including the selected item's content.
 *        This function is responsible for drawing all content of the selected item.
 */
void MenuSystem::drawAnimatedSlider() {
  if (currentMenuSize == 0) return; // Do not draw slider if no menu items

  int animX = round(sliderAnim.x_cur);
  int animY = round(sliderAnim.y_cur);
  int animWidth = round(sliderAnim.w_cur);
  int animHeight = round(sliderAnim.h_cur);

  // Draw background rounded rectangle with highlight color
  tft->fillRoundRect(animX, animY, animWidth, animHeight, menuItemCornerRadius, highlightColor);
  
  // Draw border rounded rectangle with border color
  tft->drawRoundRect(animX - menuItemBorderOffset, animY - menuItemBorderOffset, 
                     animWidth + 2 * menuItemBorderOffset, animHeight + 2 * menuItemBorderOffset, 
                     menuItemCornerRadius + menuItemBorderOffset, borderColor);
  
  // Draw selected item's text, decorator, and arrow (these now move with the slider)
  String itemText = currentMenu[selectedIndex].getLabel();
  uint16_t currentTextColor = selectedTextColor; // Text color for selected item

  tft->setTextColor(currentTextColor); // Set text color
  tft->setTextSize(menuFontSize);      // Set text size

  // Calculate text Y coordinate to center it within the slider
  int textDrawY = animY + menuItemTextYOffset + (animHeight - 2 * menuItemTextYOffset - tft->fontHeight()) / 2; 

  tft->setCursor(animX + itemDecoratorW + menuItemTextXPadding, textDrawY); // Set text cursor position
  tft->print(itemText); // Draw text

  // Draw decorator
  tft->fillRect(animX + 2, animY + animHeight / 2 - itemDecoratorH / 2, itemDecoratorW, itemDecoratorH, currentTextColor);

  // Draw arrow if it has a submenu
  if (currentMenu[selectedIndex].hasSubMenu()) {
    int arrowBaseX = animX + animWidth - menuItemArrowMarginX;
    int arrowTipX = arrowBaseX - menuItemArrowWidth / 2; // X coordinate of the arrow tip
    int arrowCenterY = animY + animHeight / 2; // Y coordinate of the arrow center
    
    tft->fillTriangle(arrowBaseX, arrowCenterY, 
                      arrowTipX, arrowCenterY - menuItemArrowWidth / 2, 
                      arrowTipX, arrowCenterY + menuItemArrowWidth / 2, 
                      currentTextColor);
  }
}

/**
 * @brief Draws an animated window, typically used for special callback animations.
 */
void MenuSystem::drawAnimatedWindow() {
  if (currentMenuSize == 0) return; // Do not draw window if no menu items

  int animX = round(sliderAnim.x_cur);
  int animY = round(sliderAnim.y_cur);
  int animWidth = round(sliderAnim.w_cur);
  int animHeight = round(sliderAnim.h_cur);

  // Draw background rounded rectangle with highlight color
  tft->fillRoundRect(animX, animY, animWidth, animHeight, menuItemCornerRadius, highlightColor);
  
  // Draw border rounded rectangle with border color
  tft->drawRoundRect(animX - menuItemBorderOffset, animY - menuItemBorderOffset, 
                     animWidth + 2 * menuItemBorderOffset, animHeight + 2 * menuItemBorderOffset, 
                     menuItemCornerRadius + menuItemBorderOffset, borderColor);
}

/**
 * @brief Initiates a new slider animation to a specified target rectangle.
 * @param targetX Target X coordinate.
 * @param targetY Target Y coordinate.
 * @param targetWidth Target width.
 * @param targetHeight Target height.
 */
void MenuSystem::startAnimation(int targetX, int targetY, int targetWidth, int targetHeight) {
  sliderAnim.x_tgt = targetX;
  sliderAnim.y_tgt = targetY;
  sliderAnim.w_tgt = targetWidth;
  sliderAnim.h_tgt = targetHeight;
  
  animationActive = true;
  lastAnimTime = millis();
}

/**
 * @brief Sets the root menu for the system.
 * @param menu Pointer to the array of MenuItem objects for the root menu.
 * @param size The number of items in the root menu.
 */
void MenuSystem::setRootMenu(MenuItem* menu, uint8_t size) {
  currentMenu = menu;
  currentMenuSize = size;
  selectedIndex = 0;
  startIndex = 0;
  menuLevel = 0;
  
  calculateLayoutParameters(); // Recalculate layout parameters

  if (currentMenuSize > 0) {
    RectF initialRect = calculateSliderTargetRect(0); // Calculate initial slider position for the first menu item
    sliderAnim.x_cur = initialRect.x;
    sliderAnim.y_cur = initialRect.y;
    sliderAnim.w_cur = initialRect.width;
    sliderAnim.h_cur = initialRect.height;

    sliderAnim.x_tgt = initialRect.x; // Target also set to current position, no animation
    sliderAnim.y_tgt = initialRect.y;
    sliderAnim.w_tgt = initialRect.w_cur;
    sliderAnim.h_tgt = initialRect.h_cur;
  } else { // If menu is empty, slider returns to default position
    sliderAnim.x_cur = menuItemsXOffset;
    sliderAnim.y_cur = menuItemsAreaY;
    sliderAnim.w_cur = menuItemDefaultMaxWidth * 0.8;
    sliderAnim.h_cur = actualMenuItemHeight;

    sliderAnim.x_tgt = sliderAnim.x_cur;
    sliderAnim.y_tgt = sliderAnim.y_cur;
    sliderAnim.w_tgt = sliderAnim.w_cur;
    sliderAnim.h_tgt = sliderAnim.h_cur;
  }
  
  needFullRedraw = true;
  lastSelectedIndex = -1;
  lastStartIndex = -1;
  lastTitle = "";
  lastSelectedRect.valid = false;
  titleDecoratorAnimationActive = false; // Reset title animation when setting root menu
  currentTitleDecoratorX = titleDecoratorX; // Ensure decorator returns to static position
  titleDecoratorAnim.x_cur = titleDecoratorX; // Ensure animation start position is also correct
  titleDecoratorAnim.x_tgt = titleDecoratorX;
}

/**
 * @brief Draws the menu title.
 * @param forceRedraw If true, forces a redraw of the entire title area.
 * @param forceTextRedraw If true, forces a redraw of the title text and background, even if text hasn't changed.
 */
void MenuSystem::drawTitle(bool forceRedraw, bool forceTextRedraw) {
  String currentTitleStr;
  if (menuLevel == 0) {
    currentTitleStr = "Root Menu"; // Default root menu title
  } else {
    // Ensure history index is valid
    if (menuLevel -1 < 10 && selectedIndexHistory[menuLevel-1] < menuSizeHistory[menuLevel-1]) {
         currentTitleStr = menuHistory[menuLevel-1][selectedIndexHistory[menuLevel-1]].getLabel();
    } else {
        currentTitleStr = "ERROR"; // Should not happen
    }
  }
                      
  // Only clear and redraw title text area if title text changed, force redraw,
  // full redraw needed, or force text redraw is true.
  if (lastTitle != currentTitleStr || forceRedraw || needFullRedraw || forceTextRedraw) {
    tft->fillRect(0, 0, screenWidth, actualTitleAreaHeight, menuBgColor); // Clear entire title area
    
    tft->setTextColor(titleColor);
    tft->setTextSize(titleFontSize);
    tft->setCursor(titleTextX, titleTextY);
    tft->print(currentTitleStr);
    lastTitle = currentTitleStr; // Update lastTitle only when text is actually redrawn
  }

  // Title decorator element: always draw at current animated position
  tft->fillRoundRect(currentTitleDecoratorX, titleDecoratorY, titleDecoratorW, titleDecoratorH, titleDecoratorW/3, highlightColor);
}

/**
 * @brief Updates rectangle information for a menu item. (Might be deprecated or unused in current animation logic).
 * @param index The index of the menu item.
 * @param selected True if the item is selected.
 * @param rect Reference to the MenuItemRect structure to update.
 */
void MenuSystem::updateRectInfo(uint8_t index, bool selected, MenuItemRect &rect) {
  if (index >= currentMenuSize) {
    rect.valid = false;
    return;
  }
  int itemY = menuItemsAreaY + (index - startIndex) * (actualMenuItemHeight + actualMenuItemSpacing);
  int itemW = calculateItemWidth(index);
  
  rect.x = menuItemsXOffset - menuItemBorderOffset;
  rect.y = itemY - menuItemBorderOffset;
  rect.width = itemW + 2 * menuItemBorderOffset;
  rect.height = actualMenuItemHeight + 2 * menuItemBorderOffset;
  rect.valid = true;
}

/**
 * @brief Clears a specified rectangular area on the screen. (Might be deprecated or unused in current animation logic).
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param width Width of the area.
 * @param height Height of the area.
 */
void MenuSystem::clearMenuItem(int x, int y, int width, int height) {
  tft->fillRect(x, y, width, height, backgroundColor); // Use overall background color
}

/**
 * @brief Draws the list of menu items. Only non-selected items are drawn by this function.
 * @param forceRedraw If true, forces a complete redraw of the visible menu items.
 */
void MenuSystem::drawMenuItems(bool forceRedraw) {
  if (animationActive && !forceRedraw) return; // If slider animation is active, drawing is handled by updateAnimation

  bool fullRedrawNeeded = needFullRedraw || forceRedraw || (lastStartIndex != startIndex);

  if (fullRedrawNeeded) {
    tft->fillRect(0, actualTitleAreaHeight, screenWidth, screenHeight - actualTitleAreaHeight, backgroundColor);
    
    uint8_t maxItemsToDraw = startIndex + actualMaxDisplayItems;
    uint8_t endIndex = (maxItemsToDraw < currentMenuSize) ? maxItemsToDraw : currentMenuSize;
    
    for (uint8_t i = startIndex; i < endIndex; i++) {
      // Here, we only draw non-selected items. The selected item will be drawn by drawAnimatedSlider.
      if (i != selectedIndex) {
        drawMenuItem(i, false); 
      } else {
        // If it's the selected item, ensure its background is cleared so drawAnimatedSlider can draw it
        int itemY = menuItemsAreaY + (i - startIndex) * (actualMenuItemHeight + actualMenuItemSpacing);
        int itemW = calculateItemWidth(i);
        tft->fillRect(menuItemsXOffset - menuItemBorderOffset, itemY - menuItemBorderOffset, 
                     itemW + 2 * menuItemBorderOffset, actualMenuItemHeight + 2 * menuItemBorderOffset, 
                     backgroundColor);
      }
    }
    drawScrollbar();
    lastStartIndex = startIndex;
  } else if (lastSelectedIndex != selectedIndex) {
    // If not a full redraw, and selected item changed
    // Clear the area of the old selected item (which was previously the slider)
    if (lastSelectedRect.valid) {
        tft->fillRect(lastSelectedRect.x, lastSelectedRect.y, 
                       lastSelectedRect.width, lastSelectedRect.height, 
                       backgroundColor);
    }

    // Redraw the previously selected item, now displayed as unselected
    if (lastSelectedIndex >= startIndex && lastSelectedIndex < startIndex + actualMaxDisplayItems) {
      drawMenuItem(lastSelectedIndex, false); 
    }
    // The new selected item will be drawn by drawAnimatedSlider
  }
  
  lastSelectedIndex = selectedIndex;
  needFullRedraw = false; // Reset flag after drawing
}

/**
 * @brief Draws a single menu item. This function is primarily responsible for drawing non-selected items.
 *        Selected items are drawn by `drawAnimatedSlider`.
 * @param index The index of the menu item to draw.
 * @param isSelected True if the item is currently selected.
 */
void MenuSystem::drawMenuItem(uint8_t index, bool isSelected) {
  if (index >= currentMenuSize) return; // Bounds check

  // If this item is the selected item, its drawing (including content) is entirely handled by drawAnimatedSlider.
  // This function only draws non-selected items.
  if (isSelected) return; 
  
  int itemY = menuItemsAreaY + (index - startIndex) * (actualMenuItemHeight + actualMenuItemSpacing); // Menu item Y coordinate
  int itemW = calculateItemWidth(index); // Menu item width

  // Draw item background (always use menu background color, slider handles highlighting)
  tft->fillRoundRect(menuItemsXOffset, itemY, itemW, actualMenuItemHeight, menuItemCornerRadius, menuBgColor);
  
  uint16_t currentTxtColor = textColor; // Text color for non-selected items
  
  // Draw decorator
  tft->fillRect(menuItemsXOffset + 2, itemY + actualMenuItemHeight / 2 - itemDecoratorH / 2, itemDecoratorW, itemDecoratorH, currentTxtColor);

  // Draw item text
  tft->setTextColor(currentTxtColor);
  tft->setTextSize(menuFontSize);
  int textDrawY = itemY + menuItemTextYOffset + (actualMenuItemHeight - 2 * menuItemTextYOffset - tft->fontHeight()) / 2; 
  tft->setCursor(menuItemsXOffset + itemDecoratorW + menuItemTextXPadding, textDrawY);
  tft->print(currentMenu[index].getLabel());
  
  // If it has a submenu, draw the arrow
  if (currentMenu[index].hasSubMenu()) {
    int arrowBaseX = menuItemsXOffset + itemW - menuItemArrowMarginX;
    int arrowTipX = arrowBaseX - menuItemArrowWidth / 2; // X coordinate of the arrow tip
    int arrowCenterY = itemY + actualMenuItemHeight / 2; // Y coordinate of the arrow center
    
    tft->fillTriangle(arrowBaseX, arrowCenterY, 
                      arrowTipX, arrowCenterY - menuItemArrowWidth / 2, 
                      arrowTipX, arrowCenterY + menuItemArrowWidth / 2, 
                      currentTxtColor);
  }
}

/**
 * @brief Draws the scrollbar if the menu items exceed the visible display area.
 */
void MenuSystem::drawScrollbar() {
  if (currentMenuSize <= actualMaxDisplayItems || actualMaxDisplayItems == 0) return;
  
  // Draw scrollbar background/track
  tft->fillRect(scrollbarX, scrollbarY, scrollbarW, scrollbarH, TFT_DARKGREY); 
  
  // Calculate thumb properties
  int thumbHeight = std::max((int)scrollbarThumbMinHeight, (int)(scrollbarH * actualMaxDisplayItems / currentMenuSize));
  int thumbMaxY = scrollbarH - thumbHeight; // Maximum top position for the thumb
  int thumbY = scrollbarY + (thumbMaxY * startIndex / (currentMenuSize - actualMaxDisplayItems));
  
  tft->fillRect(scrollbarX, thumbY, scrollbarW, thumbHeight, TFT_WHITE); // Thumb color
}

/**
 * @brief Placeholder for memory optimization routines.
 *        In more complex systems, this might involve dynamic allocation/deallocation.
 */
void MenuSystem::optimizeMemoryUsage() {
  // Placeholder - in a more complex system, this might involve dynamic allocation/deallocation of menu items.
  // For this array-based history, there's not much to do beyond the history array boundaries.
}

/**
 * @brief Draws the entire menu system.
 * @param forceRedraw If true, forces a complete redraw of the screen.
 */
void MenuSystem::drawMenu(bool forceRedraw) {
  if (forceRedraw || needFullRedraw) { // Only perform full redraw when necessary
    tft->fillScreen(backgroundColor); // Clear screen
    needFullRedraw = true; // Ensure all components redraw
  }
  // drawTitle now automatically draws the decorator based on animation state
  drawTitle(needFullRedraw || forceRedraw); 
  drawMenuItems(needFullRedraw || forceRedraw); // Draw menu items (only non-selected)
  if(type == 0) drawAnimatedSlider(); // Update slider animation
  else drawAnimatedWindow(); // Update window animation
}

/**
 * @brief Updates the title decorator animation state.
 */
void MenuSystem::updateTitleDecoratorAnimation() {
  if (!titleDecoratorAnimationActive) return;

  unsigned long currentTime = millis();
  if (currentTime - lastTitleDecoratorAnimTime >= animInterval) { // Use same interval as slider animation
    float deltaTime = (currentTime - lastTitleDecoratorAnimTime) / 1000.0f;
    if (deltaTime > 0.1f) deltaTime = 0.1f;
    if (deltaTime <= 0.0f) deltaTime = (float)animInterval / 1000.0f;

    lastTitleDecoratorAnimTime = currentTime;

    bool xDone = animateSingleValue(&titleDecoratorAnim.x_cur, &titleDecoratorAnim.x_tgt,
                                    &titleDecoratorAnim.x_vel, &titleDecoratorAnim.x_err, deltaTime);
    currentTitleDecoratorX = round(titleDecoratorAnim.x_cur);

    // Force redraw of the entire title area to ensure no artifacts
    drawTitle(false, true); // forceTextRedraw = true ensures title text and background are redrawn

    if (xDone) {
      titleDecoratorAnimationActive = false;
      needFullRedraw = true; // Force a full redraw after animation ends to ensure a clean screen state
    }
  }
}

/**
 * @brief Moves the selection to the next menu item.
 */
void MenuSystem::selectNext() {
  if (currentMenuSize == 0 || selectedIndex >= currentMenuSize - 1) return;
  if(BanOperation == true) return; // If operation is banned, return immediately
  selectedIndex++; // Select next menu item
  buzzer->beep(20,1000,buzz_vol); 

  // During scroll operations, smooth animation is usually not desired; jump to new position immediately
  if (_sliderDisplayMode == SLIDER_DISPLAY_FIXED_TOP) {
    startIndex = selectedIndex; // Force selected item to the top
    needFullRedraw = true; // Force full redraw to update scroll position
    // Immediately set slider current position to target position, avoiding animation during scroll
    RectF targetRect = calculateSliderTargetRect(selectedIndex);
    sliderAnim.x_cur = targetRect.x;
    sliderAnim.y_cur = targetRect.y;
    sliderAnim.w_cur = targetRect.width; 
    sliderAnim.h_cur = targetRect.height; 
    animationActive = false; // Disable animation as it's an immediate scroll
    return;
  }

  // Scrolling logic for SLIDER_DISPLAY_FOLLOW_SELECTION mode
  if (selectedIndex >= startIndex + actualMaxDisplayItems) {
    startIndex = selectedIndex - actualMaxDisplayItems + 1;
    needFullRedraw = true; // Force full redraw to update scroll position
    // Immediately set slider current position to target position, avoiding animation during scroll
    RectF targetRect = calculateSliderTargetRect(selectedIndex);
    sliderAnim.x_cur = targetRect.x;
    sliderAnim.y_cur = targetRect.y;
    sliderAnim.w_cur = targetRect.width; 
    sliderAnim.h_cur = targetRect.height; 
    animationActive = false; // Disable animation as it's an immediate scroll
    return;
  }
  
  // If no scrolling, initiate animation
  RectF targetRect = calculateSliderTargetRect(selectedIndex);
  startAnimation(targetRect.x, targetRect.y, targetRect.width, targetRect.height);
}

/**
 * @brief Moves the selection to the previous menu item.
 */
void MenuSystem::selectPrev() {
  if (currentMenuSize == 0 || selectedIndex == 0) return;
  if(BanOperation == true) return; // If operation is banned, return immediately
  selectedIndex--;
  buzzer->beep(20,1000,buzz_vol); 

  // During scroll operations, smooth animation is usually not desired; jump to new position immediately
  if (_sliderDisplayMode == SLIDER_DISPLAY_FIXED_TOP) {
    startIndex = selectedIndex; // Force selected item to the top
    needFullRedraw = true; // Force full redraw to update scroll position
    // Immediately set slider current position to target position
    RectF targetRect = calculateSliderTargetRect(selectedIndex);
    sliderAnim.x_cur = targetRect.x;
    sliderAnim.y_cur = targetRect.y;
    sliderAnim.w_cur = targetRect.width; 
    sliderAnim.h_cur = targetRect.height; 
    animationActive = false; // Disable animation
    return;
  }

  // Scrolling logic for SLIDER_DISPLAY_FOLLOW_SELECTION mode
  if (selectedIndex < startIndex) {
    startIndex = selectedIndex;
    needFullRedraw = true; // Force full redraw to update scroll position
    // Immediately set slider current position to target position
    RectF targetRect = calculateSliderTargetRect(selectedIndex);
    sliderAnim.x_cur = targetRect.x;
    sliderAnim.y_cur = targetRect.y;
    sliderAnim.w_cur = targetRect.width; 
    sliderAnim.h_cur = targetRect.height; 
    animationActive = false; // Disable animation
    return;
  }

  // If no scrolling, initiate animation
  RectF targetRect = calculateSliderTargetRect(selectedIndex);
  startAnimation(targetRect.x, targetRect.y, targetRect.width, targetRect.height);
}

/**
 * @brief Confirms the selection of the current menu item, executing its callback or entering its submenu.
 */
void MenuSystem::select() {
  if (!currentMenu || selectedIndex >= currentMenuSize) return;
  if(BanOperation == true) return; // If operation is banned, return immediately
  MenuItem selectedItem = currentMenu[selectedIndex];
  buzzer->beep(20,1000,buzz_vol);
   
  if (selectedItem.getCallback() != NULL) {
      selectedItem.getCallback()();
      //
      switch (TypeNum) 
      {
        case 0:break;
        case 1:{
            Serial.println("Scroll");
            type = 1; // Set flag for window animation

            uint16_t WinStartX = screenWidth*0.05;
            uint16_t WinStartY = screenHeight*0.25;
            uint16_t WinWidth = screenWidth*0.9;
            uint16_t WinHeight = screenHeight*0.5;

            RectF targetRect = {WinStartX, WinStartY, WinWidth, WinHeight};
            // Animate from current slider position to the new submenu's first item position

            if(animationActive == false){
                tft->fillRect(WinStartX*0.05, WinStartY*0.5, WinWidth*0.9,5, TFT_BLACK); 
            }
            
            startAnimation(targetRect.x, targetRect.y, targetRect.width, targetRect.height);

            TypeNum = 0;
            BanOperation = true; // Ban operations during this specific animation
            break;
        }
        case 2:{
            Serial.println("test");
            TypeNum = 0;
            BanOperation = true; // Ban operations during this specific animation
            break;
        }
      }
  }
  
  if (selectedItem.hasSubMenu()) {
    if (menuLevel < 9) { // Max 10 levels (0-9)
        menuHistory[menuLevel] = currentMenu; // Save current menu
        menuSizeHistory[menuLevel] = currentMenuSize; // Save current menu size
        selectedIndexHistory[menuLevel] = selectedIndex; // Save current index
        menuLevel++;
        
        currentMenu = selectedItem.getSubMenu();
        currentMenuSize = selectedItem.getSubMenuSize();
        selectedIndex = 0; // Select first item when entering a submenu
        
        // Set start index based on new menu and mode
        if (_sliderDisplayMode == SLIDER_DISPLAY_FIXED_TOP) {
            startIndex = 0; // Force first item of submenu to the top
        } else {
            startIndex = 0; // Default follow mode also starts from 0
        }
        
        if (currentMenuSize > 0) { // If submenu is not empty
            RectF targetRect = calculateSliderTargetRect(selectedIndex);
            // Animate from current slider position to the new submenu's first item position
            startAnimation(targetRect.x, targetRect.y, targetRect.width, targetRect.height);
        } else { // If submenu is empty, slider returns to default position
            sliderAnim.x_cur = menuItemsXOffset;
            sliderAnim.y_cur = menuItemsAreaY;
            sliderAnim.w_cur = menuItemDefaultMaxWidth * 0.8;
            sliderAnim.h_cur = actualMenuItemHeight;

            sliderAnim.x_tgt = sliderAnim.x_cur;
            sliderAnim.y_tgt = sliderAnim.y_cur;
            sliderAnim.w_tgt = sliderAnim.w_cur;
            sliderAnim.h_tgt = sliderAnim.h_cur;
        }
        animationActive = true; 
        needFullRedraw = true;
        lastSelectedIndex = -1; 
        lastStartIndex = -1;
        lastTitle = ""; // Force title redraw

        // Start title decorator animation (slide in from right)
        titleDecoratorAnim.x_cur = screenWidth; // Start from right side of screen
        titleDecoratorAnim.x_tgt = titleDecoratorX; // Target is normal position
        titleDecoratorAnim.x_vel = 0.0f;
        titleDecoratorAnim.x_err = 0.0f;
        currentTitleDecoratorX = screenWidth; // Initialize current drawing position
        titleDecoratorAnimationActive = true;
        lastTitleDecoratorAnimTime = millis();
    }
  }
}

/**
 * @brief Returns to the previous menu level.
 */
void MenuSystem::back() {
  if (menuLevel > 0) { // If current menu level is greater than 0
    if(type == 0){ // If not in a special window animation mode
        menuLevel--; // Go back to previous menu level
        buzzer->longBeep(100,1000,buzz_vol); // Long beep
        currentMenu = menuHistory[menuLevel]; // Get previous menu
        currentMenuSize = menuSizeHistory[menuLevel]; // Get previous menu size
        selectedIndex = selectedIndexHistory[menuLevel]; // Get previous menu's selected index

        // Set start index based on new menu and mode
        if (_sliderDisplayMode == SLIDER_DISPLAY_FIXED_TOP) {
            startIndex = selectedIndex; // Force selected item to the top
        } else {
            // Ensure selected item remains visible after returning
            if (selectedIndex >= startIndex + actualMaxDisplayItems) {
                startIndex = selectedIndex - actualMaxDisplayItems + 1;
            } else if (selectedIndex < startIndex) {
                startIndex = selectedIndex;
            }
            // Ensure startIndex does not go beyond the bottom of the menu
            if (startIndex > 0 && startIndex + actualMaxDisplayItems > currentMenuSize) {
                startIndex = std::max(0, (int)currentMenuSize - (int)actualMaxDisplayItems);
            }
        }

        if (currentMenuSize > 0) { // If current menu is not empty
        RectF targetRect = calculateSliderTargetRect(selectedIndex);
        // Animate from current slider position to the selected item's position in the previous menu
        startAnimation(targetRect.x, targetRect.y, targetRect.width, targetRect.height);
        } else { // If the returned menu is empty, slider returns to default position
        sliderAnim.x_cur = menuItemsXOffset;
        sliderAnim.y_cur = menuItemsAreaY;
        sliderAnim.w_cur = menuItemDefaultMaxWidth * 0.8;
        sliderAnim.h_cur = actualMenuItemHeight;

        sliderAnim.x_tgt = sliderAnim.x_cur;
        sliderAnim.y_tgt = sliderAnim.y_cur;
        sliderAnim.w_tgt = sliderAnim.w_cur;
        sliderAnim.h_tgt = sliderAnim.h_cur;
        }

        animationActive = true; 
        needFullRedraw = true;
        lastSelectedIndex = -1;
        lastStartIndex = -1; 
        lastTitle = ""; // Force title redraw

        // Start title decorator animation (slide in from left)
        titleDecoratorAnim.x_cur = -titleDecoratorW; // Start from left side of screen
        titleDecoratorAnim.x_tgt = titleDecoratorX; // Target is normal position
        titleDecoratorAnim.x_vel = 0.0f;
        titleDecoratorAnim.x_err = 0.0f;
        currentTitleDecoratorX = -titleDecoratorW; // Initialize current drawing position
        titleDecoratorAnimationActive = true;
        lastTitleDecoratorAnimTime = millis();
        }
    else{ // If currently in a special window animation mode, just exit that mode
        type = 0; // Revert to normal menu item animation
        buzzer->longBeep(100,1000,buzz_vol); // Long beep
        RectF targetRect = calculateSliderTargetRect(selectedIndex);
        startAnimation(targetRect.x, targetRect.y, targetRect.width, targetRect.height);
        BanOperation = false; // Re-enable operations
        }
    }
}

/**
 * @brief Updates the menu display, including animations.
 *        This function should be called frequently in the main loop.
 */
void MenuSystem::update() {
  // Prioritize animation updates
  if (animationActive) { // Slider animation
    if(type == 1){updateAnimation(1);} // Special window animation
    else{updateAnimation(0);} // Standard slider animation
  }
  if (titleDecoratorAnimationActive) { // Title decorator animation
    updateTitleDecoratorAnimation();
  }
  // Only perform full menu drawing when all animations are inactive and a full redraw is needed.
  // When animations are active, they are responsible for their own drawing.
  if (!animationActive && !titleDecoratorAnimationActive && needFullRedraw) { 
    drawMenu(true); // drawMenu will call drawTitle and drawMenuItems
                    // needFullRedraw will be reset in drawMenuItems
  }
  // Otherwise, no drawing operations are performed to save CPU cycles.
}

//------------------------------------Style Setters------------------------------------//
void MenuSystem::setBackgroundColor(uint16_t color) {
  backgroundColor = color;
  needFullRedraw = true;
}

void MenuSystem::setMenuBgColor(uint16_t color) {
  menuBgColor = color;
  needFullRedraw = true;
}

void MenuSystem::setHighlightColor(uint16_t color) {
  highlightColor = color;
  needFullRedraw = true;
}

void MenuSystem::setTextColor(uint16_t color) {
  textColor = color;
  needFullRedraw = true;
}

void MenuSystem::setSelectedTextColor(uint16_t color) {
  selectedTextColor = color;
  needFullRedraw = true;
}

void MenuSystem::setTitleColor(uint16_t color) {
  titleColor = color;
  needFullRedraw = true;
}

void MenuSystem::setBorderColor(uint16_t color) {
  borderColor = color;
  needFullRedraw = true;
}

void MenuSystem::setMenuFontSize(uint8_t size) {
  if (size == 0) size = 1; // Prevent font size from being 0
  menuFontSize = size;
  calculateLayoutParameters(); // Recalculate layout parameters
  needFullRedraw = true;
}

void MenuSystem::setTitleFontSize(uint8_t size) {
  if (size == 0) size = 1; // Prevent font size from being 0
  titleFontSize = size;
  calculateLayoutParameters(); // Recalculate layout parameters
  needFullRedraw = true;
}

//------------------------------------Slider Animation Control API------------------------------------//

/**
 * @brief Sets the slider display mode.
 * @param mode The desired display mode (SLIDER_DISPLAY_FOLLOW_SELECTION or SLIDER_DISPLAY_FIXED_TOP).
 */
void MenuSystem::setSliderDisplayMode(SliderDisplayMode mode) {
  _sliderDisplayMode = mode;
  needFullRedraw = true; // Mode change may require redraw
  // Immediately update slider current position to target to avoid animation during mode switch
  if (currentMenuSize > 0) {
      RectF targetRect = calculateSliderTargetRect(selectedIndex);
      sliderAnim.x_cur = targetRect.x;
      sliderAnim.y_cur = targetRect.y;
      sliderAnim.w_cur = targetRect.width; 
      sliderAnim.h_cur = targetRect.height; 
      sliderAnim.x_tgt = targetRect.x;
      sliderAnim.y_tgt = targetRect.y;
      sliderAnim.w_tgt = targetRect.width; 
      sliderAnim.h_tgt = targetRect.height; 
  }
  animationActive = false; // Disable animation as it's an immediate mode switch
}

/**
 * @brief Sets custom target position and size for the slider.
 *        These values override default calculations only in SLIDER_DISPLAY_FIXED_TOP mode.
 * @param x Custom X coordinate, -1 to use default.
 * @param y Custom Y coordinate, -1 to use default.
 * @param width Custom width, -1 to use default.
 * @param height Custom height, -1 to use default.
 */
void MenuSystem::setCustomSliderTarget(int x, int y, int width, int height) {
  _customSliderX = x;
  _customSliderY = y;
  _customSliderWidth = width;
  _customSliderHeight = height;

  _useCustomSliderPosition = (x != -1 || y != -1);
  _useCustomSliderSize = (width != -1 || height != -1);

  // If currently in fixed top mode, immediately update slider target
  // Otherwise, these custom parameters will be ignored in calculateSliderTargetRect
  if (_sliderDisplayMode == SLIDER_DISPLAY_FIXED_TOP) {
      if (currentMenuSize > 0) {
          RectF targetRect = calculateSliderTargetRect(selectedIndex);
          startAnimation(targetRect.x, targetRect.y, targetRect.width, targetRect.height);
      }
  }
}

/**
 * @brief Sets the slider animation form.
 * @param form 1 for Precise Control (integral controller), 2 for Underdamped.
 */
void MenuSystem::setSliderAnimationForm(uint8_t form) {
  if (form == 1 || form == 2) {
    _animationForm = form;
  }
}

/**
 * @brief Sets the duration of slider animations in milliseconds.
 * @param duration Animation duration in ms.
 */
void MenuSystem::setSliderAnimationDuration(uint16_t duration) {
  animDuration = duration;
}

/**
 * @brief Sets the update interval for slider animations in milliseconds.
 *        A smaller interval results in smoother animation but higher CPU usage.
 * @param interval Animation update interval in ms.
 */
void MenuSystem::setSliderAnimationInterval(uint16_t interval) {
  animInterval = interval;
}

/**
 * @brief Calculates the slider's target rectangle based on the current display mode and selected index.
 * @param index The index of the menu item for which to calculate the target rectangle.
 * @return A RectF structure representing the target position and size.
 */
RectF MenuSystem::calculateSliderTargetRect(uint8_t index) {
  RectF targetRect;
  
  // Default values
  targetRect.x = menuItemsXOffset;
  targetRect.height = actualMenuItemHeight;

  if (_sliderDisplayMode == SLIDER_DISPLAY_FIXED_TOP) {
    // Force slider to the top of the menu area
    targetRect.y = menuItemsAreaY;
    targetRect.width = calculateItemWidth(index); // Width still follows selected item
  } else { // SLIDER_DISPLAY_FOLLOW_SELECTION (default mode)
    targetRect.y = menuItemsAreaY + (index - startIndex) * (actualMenuItemHeight + actualMenuItemSpacing);
    targetRect.width = calculateItemWidth(index);
  }
  
  // Apply user-defined size/position overrides
  // Only effective if _useCustomSliderPosition or _useCustomSliderSize is true, and corresponding value is not -1
  if (_useCustomSliderPosition) {
      if (_customSliderX != -1) targetRect.x = _customSliderX;
      if (_customSliderY != -1) targetRect.y = _customSliderY;
  }
  if (_useCustomSliderSize) {
      if (_customSliderWidth != -1) targetRect.width = _customSliderWidth;
      if (_customSliderHeight != -1) targetRect.height = _customSliderHeight;
  }

  return targetRect;
}

/**
 * @brief Gets the current menu level.
 * @return The current menu level (0 for root menu).
 */
uint8_t MenuSystem::getCurrentLevel() {
  return menuLevel;
}

/**
 * @brief Gets the index of the currently selected menu item.
 * @return The selected item's index.
 */
uint8_t MenuSystem::getSelectedIndex() {
  return selectedIndex;
}