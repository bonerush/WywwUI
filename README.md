# TFT_Menu: A Dynamic Menu System for TFT Displays

TFT_Menu is a lightweight and highly customizable menu system designed for TFT displays, specifically built upon the `TFT_eSPI` library. It provides a smooth, animated, and intuitive user interface for embedded projects, supporting hierarchical menus with various visual feedback options.

## Features

*   **Hierarchical Menu Navigation:** Easily define and navigate through multi-level menus and submenus.
*   **Smooth Animations:**
    *   **Slider Animation:** Visually highlights the selected menu item with smooth transitions.
    *   **Title Decorator Animation:** Adds a subtle animation to the menu title when navigating between levels.
    *   Supports two animation forms: `Precise Control` (integral controller based) and `Underdamped` (spring-mass-damper model).
*   **Customizable Styling:** Full control over colors (background, menu background, highlight, text, title, border) and font sizes for both menu items and titles.
*   **Two Slider Display Modes:**
    *   `SLIDER_DISPLAY_FOLLOW_SELECTION` (Default): The slider moves with the selected item, scrolling the list when necessary.
    *   `SLIDER_DISPLAY_FIXED_TOP`: The selected item is always forced to the top of the visible menu area, with the slider fixed at that position.
*   **Custom Slider Targets:** For `SLIDER_DISPLAY_FIXED_TOP` mode, you can define custom X, Y, Width, and Height for the slider.
*   **Anti-Flicker Optimization:** Intelligent partial screen updates and redraw logic to minimize flickering during menu operations and animations.
*   **Buzzer Feedback:** Integrates with a `Buzzer` class for audible feedback on navigation and selection.
*   **Automatic Layout Calculation:** Dynamically calculates menu item heights, spacing, and scrollbar dimensions based on screen size and font settings.
*   **Operation Ban Flag:** Prevents user input during active animations or specific operations.

## Dependencies

*   [**TFT_eSPI Library**](https://github.com/Bodmer/TFT_eSPI): A highly optimized graphics library for ESP32 and ESP8266 processors with TFT displays.
*   **Buzzer.h**: A custom `Buzzer` class (not included in this repository, you'll need to provide your own simple implementation for basic beep functionality). A minimal example is provided below.

## Installation

1.  **Download:** Download the `TFT_Menu` repository as a ZIP file.
2.  **Arduino IDE:** Open your Arduino IDE.
3.  **Add Library:** Go to `Sketch > Include Library > Add .ZIP Library...` and select the downloaded ZIP file.
4.  **Buzzer.h:** Ensure you have a `Buzzer.h` and `Buzzer.cpp` (or equivalent) in your project directory or as a library.
