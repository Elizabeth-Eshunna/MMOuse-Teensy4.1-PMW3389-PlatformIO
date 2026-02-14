#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

#include "mouse_buttons.h"

// Define MCP objects
Adafruit_MCP23X17 mcp1;
Adafruit_MCP23X17 mcp2;

// Button name array (moved from teensy4_mouse.cpp)
const char* const buttonNames[32] PROGMEM = {
"FAR_EDGE"            ,
"NEAR_EDGE"           ,
"RIGHT_EDGE"          ,
"LEFT_TIP"            ,
"RIGHT_TIP"           ,
"LEFT_PUSH_TRIGGER"   ,
"RIGHT_PUSH_TRIGGER"  ,
"LEFT_PULL_TRIGGER"   ,
"RIGHT_PULL_TRIGGER"  ,
"FAR_WING"            ,
"MID_WING"            ,
"NEAR_WING"           ,
"FAR_DPI"             ,
"MID_DPI"             ,
"NEAR_DPI"            ,
"GRID_TOP_LEFT"       ,
"GRID_LEFT"           ,
"GRID_BOT_LEFT"       ,
"GRID_BOT"            ,
"GRID_BOT_RIGHT"      ,
"GRID_RIGHT"          ,
"GRID_FAR_TOP"        ,
"GRID_FAR_BOT"        ,
"UNUSED_23"           ,
"UNUSED_24"           ,
"UNUSED_25"           ,
"UNUSED_26"           ,
"UNUSED_27"           ,
"UNUSED_28"           ,
"UNUSED_29"           ,
"UNUSED_30"           , 
"UNUSED_31"
};

static bool g_shift_active = false; // Tracks the state of our G-Shift modifier key

void mcp_init()
{
  Serial.println("MCP23xxx Button Test!");
  if (!mcp1.begin_I2C(0x20)) {
    Serial.println("Error with MCP1.");
    while (1);
  }
  if (!mcp2.begin_I2C(0x22)) {
    Serial.println("Error with MCP2.");
    while (1);
  }

  for (uint8_t i = 0; i < 16; i++) {
    mcp1.pinMode(i, INPUT_PULLUP);
  }
  for (uint8_t i = 0; i < 16; i++) {
    mcp2.pinMode(i, INPUT_PULLUP);
  }
  Serial.println("MCP INIT DONE");
}

bool mcp_get_g_shift_active()
{
  return g_shift_active;
}

void update_mcp_buttons() {
    static uint32_t previous_states = 0xFFFFFFFF;
    static uint32_t g_shift_press_mask = 0; // Remembers the G-Shift state for each key at the moment it was pressed.
    uint32_t current_states = 0;

    // Read the current state of all 32 buttons efficiently
    uint16_t states_mcp1 = mcp1.readGPIOAB();
    uint16_t states_mcp2 = mcp2.readGPIOAB();
    current_states = states_mcp1 | (uint32_t)states_mcp2 << 16;

    // --- First, handle the G-Shift key itself to update the global state ---
    // This ensures its state is known before processing other keys in the same loop.
    bool g_shift_current_state = !((current_states >> RIGHT_EDGE) & 1); // is G-Shift currently pressed?
    if (g_shift_current_state) {
        if (!g_shift_active) {
            g_shift_active = true;
            Serial.println("G-SHIFT ON");
        }
    } else {
        if (g_shift_active) {
            g_shift_active = false;
            Serial.println("G-SHIFT OFF");
        }
    }

    // --- Now, iterate through all buttons to handle their events ---
    for (uint8_t i = 0; i < 32; i++) {
        // Skip the G-Shift button itself, as we've already handled its state change.
        if (i == RIGHT_EDGE) continue;

        bool is_pressed = !((current_states >> i) & 1);
        bool was_pressed = !((previous_states >> i) & 1);

        // --- Key Press Event (was released, is now pressed) ---
        if (is_pressed && !was_pressed) {
            Serial.print(buttonNames[i]);
            Serial.println(" pressed");

            // Store the G-Shift state FOR THIS KEY at the moment of pressing.
            if (g_shift_active) {
                g_shift_press_mask |= (1UL << i);
            } else {
                g_shift_press_mask &= ~(1UL << i);
            }

            // A single switch handles all press actions.
            // The G-Shift logic is handled by the ternary operator: (condition ? value_if_true : value_if_false)
            switch (i) {
                // --- MACROS ---
                case FAR_EDGE: Keyboard.press(MODIFIERKEY_CTRL); Keyboard.press(KEY_C); break;
                case NEAR_EDGE: Keyboard.press(MODIFIERKEY_CTRL); Keyboard.press(KEY_V); break;

                // --- MODIFIER KEYS (excluding G-Shift) ---
                case RIGHT_TIP: Keyboard.press(MODIFIERKEY_CTRL); break;
                case LEFT_PUSH_TRIGGER: Keyboard.press(MODIFIERKEY_ALT); break;
                case RIGHT_PUSH_TRIGGER: Keyboard.press(MODIFIERKEY_SHIFT); break;

                // --- SINGLE KEY PRESSES ---
                case LEFT_TIP: Keyboard.press(KEY_ENTER); break;
                case LEFT_PULL_TRIGGER: Keyboard.press(KEY_BACKSPACE); break;
                case RIGHT_PULL_TRIGGER: Keyboard.press(KEY_DELETE); break;
                case NEAR_DPI: Keyboard.press(KEY_PRINTSCREEN); break;
                case GRID_TOP_LEFT: Keyboard.press(KEY_ENTER); break;

                // --- Layered Keys ---
                case FAR_WING:      Keyboard.press(g_shift_active ? KEY_1 : KEY_A); break;
                case MID_WING:      Keyboard.press(g_shift_active ? KEY_2 : KEY_B); break;
                case NEAR_WING:     Keyboard.press(g_shift_active ? KEY_3 : KEY_C); break;
                case FAR_DPI:       Keyboard.press(g_shift_active ? KEY_4 : KEY_D); break;
                case MID_DPI:       Keyboard.press(g_shift_active ? KEY_5 : KEY_E); break;
                case GRID_LEFT:     Keyboard.press(g_shift_active ? KEY_6 : KEY_F); break;
                case GRID_BOT_LEFT: Keyboard.press(g_shift_active ? KEY_7 : KEY_G); break;
                case GRID_BOT:      Keyboard.press(g_shift_active ? KEY_8 : KEY_H); break;
                case GRID_BOT_RIGHT:Keyboard.press(g_shift_active ? KEY_9 : KEY_I); break;
                case GRID_RIGHT:    Keyboard.press(g_shift_active ? KEY_0 : KEY_J); break;
                case GRID_FAR_TOP:  Keyboard.press(g_shift_active ? KEY_MINUS : KEY_K); break;
                case GRID_FAR_BOT:  Keyboard.press(g_shift_active ? KEY_EQUAL : KEY_L); break;
            }
        }
        // --- Key Release Event (was pressed, is now released) ---
        else if (!is_pressed && was_pressed) {
            Serial.print(buttonNames[i]);
            Serial.println(" released");

            // Check which layer the key was on WHEN IT WAS PRESSED by consulting our mask.
            bool was_g_shifted_on_press = (g_shift_press_mask >> i) & 1;

            // A single switch handles all release actions.
            // The release action is based on the saved state, not the current one.
            switch (i) {
                // --- MACROS (release in reverse order) ---
                case FAR_EDGE: Keyboard.release(KEY_C); Keyboard.release(MODIFIERKEY_CTRL); break;
                case NEAR_EDGE: Keyboard.release(KEY_V); Keyboard.release(MODIFIERKEY_CTRL); break;

                // --- MODIFIER KEYS (excluding G-Shift) ---
                case RIGHT_TIP: Keyboard.release(MODIFIERKEY_CTRL); break;
                case LEFT_PUSH_TRIGGER: Keyboard.release(MODIFIERKEY_ALT); break;
                case RIGHT_PUSH_TRIGGER: Keyboard.release(MODIFIERKEY_SHIFT); break;

                // --- SINGLE KEY PRESSES ---
                case LEFT_TIP: Keyboard.release(KEY_ENTER); break;
                case LEFT_PULL_TRIGGER: Keyboard.release(KEY_BACKSPACE); break;
                case RIGHT_PULL_TRIGGER: Keyboard.release(KEY_DELETE); break;
                case NEAR_DPI: Keyboard.release(KEY_PRINTSCREEN); break;
                case GRID_TOP_LEFT: Keyboard.release(KEY_ENTER); break;
                
                // --- Layered Keys ---
                case FAR_WING:      Keyboard.release(was_g_shifted_on_press ? KEY_1 : KEY_A); break;
                case MID_WING:      Keyboard.release(was_g_shifted_on_press ? KEY_2 : KEY_B); break;
                case NEAR_WING:     Keyboard.release(was_g_shifted_on_press ? KEY_3 : KEY_C); break;
                case FAR_DPI:       Keyboard.release(was_g_shifted_on_press ? KEY_4 : KEY_D); break;
                case MID_DPI:       Keyboard.release(was_g_shifted_on_press ? KEY_5 : KEY_E); break;
                case GRID_LEFT:     Keyboard.release(was_g_shifted_on_press ? KEY_6 : KEY_F); break;
                case GRID_BOT_LEFT: Keyboard.release(was_g_shifted_on_press ? KEY_7 : KEY_G); break;
                case GRID_BOT:      Keyboard.release(was_g_shifted_on_press ? KEY_8 : KEY_H); break;
                case GRID_BOT_RIGHT:Keyboard.release(was_g_shifted_on_press ? KEY_9 : KEY_I); break;
                case GRID_RIGHT:    Keyboard.release(was_g_shifted_on_press ? KEY_0 : KEY_J); break;
                case GRID_FAR_TOP:  Keyboard.release(was_g_shifted_on_press ? KEY_MINUS : KEY_K); break;
                case GRID_FAR_BOT:  Keyboard.release(was_g_shifted_on_press ? KEY_EQUAL : KEY_L); break;
            }
        }
    }

    // Save the current states for the next loop iteration
    previous_states = current_states;
}
