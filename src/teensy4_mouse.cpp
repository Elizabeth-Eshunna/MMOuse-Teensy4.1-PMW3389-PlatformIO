
/*
Work In Progress

    Teensy4_mouse.ino - minimal firmware for an 8khz mouse written in arduino.
    Requires a teensy4.1 MCU and a mouse with the PMW3389 sensor.
    Use the Serial + Keyboard + Mouse + Joystick USB type.
    (part of the code inspired by https://github.com/SunjunKim/PMW3360, https://github.com/mrjohnk/PMW3360DM-T2QU and https://github.com/Trip93/teensy4_mouse)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <Adafruit_MCP23X17.h>
#include <Adafruit_BNO08x.h>

#include "BNO085_IMU.h"
#include "PMW_3389_sensor.h"
#include "srom_3389.h"
#include "mouse_buttons.h"


// PMW SPI settings and timing are defined in include/PMW_3389_sensor.h


// Button names are defined in src/mcp_buttons.cpp



static elapsedMicros burst_timer  = 0;
static elapsedMicros polling_rate = 0;
static elapsedMillis scroll_timer = 0;
static elapsedMillis button_timer = 0;

static int16_t  x                       = 0;
static int16_t  y                       = 0;
static uint8_t  scroll                  = 0;
static int8_t   buttons                 = 0;
static int8_t   buttons_old             = 0;
static uint16_t cpi[6]                  = {400, 800, 1600, 2000, 3200, 6400};
static uint8_t  cpi_index               = 5;
static bool     cpi_pressed             = false;
static uint16_t set_rate                = 125;
// Use the formula 1000000 / set_rate = polling rate to get the polling rate
// Note: this timer is seperate from the USB timer but it effectivly limits the polling rate
// Note: at lower polling rates (1000Hz and lower) it is recommended to use 16 bit x and y values to not hit the 8 bit limit
// USB_mouse_move is by default limited to 8 bit values see my github for a fix
// Note: polling rate can be "properly" limited by setting a divider (8000 / divider = polling rate or 1000 / divider = polling rate depending on interface)
// This divider can be set in the teensy core files but not sure yet if you can adjust it somewhere during runtime (usb_desc.h -> MOUSE_INTERVAL)

// MCP button handling moved to include/mcp_buttons.h / src/mcp_buttons.cpp
// PMW3389 low-level implementations moved to src/PMW_3389_sensor.cpp

/*
  Burst report according to PMW3360 datasheet
  BYTE[00] = Motion
  BYTE[01] = Observation
  BYTE[02] = Delta_X_L
  BYTE[03] = Delta_X_H
  BYTE[04] = Delta_Y_L
  BYTE[05] = Delta_Y_H
  BYTE[06] = SQUAL
  BYTE[07] = Raw_Data_Sum
  BYTE[08] = Maximum_Raw_Data
  BYTE[09] = Minimum_Raw_Data
  BYTE[10] = Shutter_Upper
  BYTE[11] = Shutter_Lower
*/

// Activates motion burst from the PMW3360
void read_burst_start()
{
  ACTIVATE_CS_PMW;
  SPI.transfer(REG_Motion_Burst);
  burst_timer = 0;
}

// Updates mouse x and y axis
void read_burst_end()
{
  int8_t burst[12] = {0};
  //read out the data send by the PMW3360 only the first 6 are nessecary so last 6 can be skipped
  SPI.transfer(burst, 12);
  DEACTIVATE_CS_PMW;

  x += ((burst[3] << 8) | burst[2]);
  y += ((burst[5] << 8) | burst[4]);
}

// Updates the scroll movement
void update_scroll()
{
  // Important to put some time between samples
  if (scroll_timer > 0)
  {
    //get new sample to compare with the old sample 0b0000NNOO
    (!digitalReadFast(MS_1)) ? scroll |= 0b00000100 : scroll &= 0b11111011;
    (!digitalReadFast(MS_0)) ? scroll |= 0b00001000 : scroll &= 0b11110111;

    // Grey code coming from the encoder
    if      ((scroll >> 2         ) == (scroll  & 0b00000011))                                                     buttons &= 0b00111111;              // No scrolling if old sample is equal to new sample
    else if ((scroll == 0b00000100) || (scroll == 0b00001101) || (scroll == 0b00001011) || (scroll == 0b00000010)) buttons |= 0b10000000 & 0b10111111; // Scroll up   00 -> 01 -> 11 -> 10 -> 00
    else if ((scroll == 0b00001000) || (scroll == 0b00001110) || (scroll == 0b00000111) || (scroll == 0b00000001)) buttons |= 0b01000000 & 0b01111111; // Scroll down 00 -> 10 -> 11 -> 01 -> 00

    //save new sample so it can be used in the next loop
    scroll       >>= 2;
    scroll_timer   = 0;
  }
}

// Updates the clickable buttons on the mouse
void update_buttons()
{
  // Update main mouse buttons at highest possible speed
  if (!digitalReadFast(M1_NO) && digitalReadFast(M1_NC)) buttons |= 0b00000001;
  if (!digitalReadFast(M1_NC) && digitalReadFast(M1_NO)) buttons &= 0b11111110;
  if (!digitalReadFast(M2_NO) && digitalReadFast(M2_NC)) buttons |= 0b00000010;
  if (!digitalReadFast(M2_NC) && digitalReadFast(M2_NO)) buttons &= 0b11111101;
  if (!digitalReadFast(M4_NO) && digitalReadFast(M4_NC)) buttons |= 0b00001000;
  if (!digitalReadFast(M4_NC) && digitalReadFast(M4_NO)) buttons &= 0b11110111;
  if (!digitalReadFast(M5_NO) && digitalReadFast(M5_NC)) buttons |= 0b00010000;
  if (!digitalReadFast(M5_NC) && digitalReadFast(M5_NO)) buttons &= 0b11101111;

  // Some browser functionallity bugs out when update rate of middle mouse button exceeds a 1000hz (page drag with middle mouse button)
  // No point updating the cpi button beyond a 1000hz
  if (button_timer > 0)
  {
    button_timer = 0;
    if (!digitalReadFast(M3_NO) && digitalReadFast(M3_NC)) buttons |= 0b00000100;
    if (!digitalReadFast(M3_NC) && digitalReadFast(M3_NO)) buttons &= 0b11111011;
    if (!digitalReadFast(MC_NO) && digitalReadFast(MC_NC)) buttons |= 0b00100000;
    if (!digitalReadFast(MC_NC) && digitalReadFast(MC_NO)) buttons &= 0b11011111;
  }
}

// updates cpi avoid using this function within SPI transactions like motion burst
void update_cpi()
{
  // Update cpi on release
  if (  buttons & 0b00100000) cpi_pressed = true;
  if (!(buttons & 0b00100000) && cpi_pressed)
  {
    cpi_pressed = false;
    cpi_index < 5 ? cpi_index++ : cpi_index = 0;
    SPI.beginTransaction(SPI_SETTINGS_PMW);
    set_CPI             (cpi[cpi_index]);
    //start motion burst mode again
    write_reg_PMW       (REG_Motion_Burst, 0x00);
    SPI.endTransaction  ();
  }
}

// Passes the button, scroll and mouse movment information to the USB bus for the next poll
void update_usb()
{
  int8_t scroll           = (buttons & 0b10000000) ? scroll =  1 : (buttons & 0b01000000) ? scroll = -1 : scroll = 0;
  usb_mouse_buttons_state =  buttons;
  buttons_old             =  buttons;
  // This function is blocking if its buffers fill up and blocks until the next USB poll (125uS at USB 2.0 HIGH speed and 1mS below that speed)
  // usb_mouse_move only accepts 8 bit data by default it still works with 8 bit values but at lower polling speeds (1000Hz and lower)
  // maximum control speed is limited due to the integers overflowing. Recommended to make this function 16 bit for lower polling speeds (see my github)
  usb_mouse_move(x, y, scroll, 0);
  x = 0;
  y = 0;
}





// Sets up all the buttons and the sensor
void setup()
{
  // Mouse buttons
  pinMode(M1_NO, INPUT_PULLUP);
  pinMode(M1_NC, INPUT_PULLUP);
  pinMode(M2_NO, INPUT_PULLUP);
  pinMode(M2_NC, INPUT_PULLUP);
  pinMode(M3_NO, INPUT_PULLUP);
  pinMode(M3_NC, INPUT_PULLUP);
  pinMode(M4_NO, INPUT_PULLUP);
  pinMode(M4_NC, INPUT_PULLUP);
  pinMode(M4_NO, INPUT_PULLUP);
  pinMode(M4_NC, INPUT_PULLUP);
  pinMode(M5_NO, INPUT_PULLUP);
  pinMode(M5_NC, INPUT_PULLUP);
  pinMode(MC_NO, INPUT_PULLUP);
  pinMode(MC_NC, INPUT_PULLUP);

  // Scroll wheel
  pinMode(MS_0, INPUT_PULLUP);
  pinMode(MS_1, INPUT_PULLUP);

  Serial.begin(115200);
  //while(!Serial); // Optional: wait for serial monitor to open

  // Initialize MCP23017 expanders and configure pins
  mcp_init();


  Serial.println("Starting PMW");
  while (!begin_PMW()) Serial.println("Problem starting PMW");
  Serial.println("PMW succesfully started");

  Serial.println(BURST_READ_TIME);

  SPI.beginTransaction(SPI_SETTINGS_PMW);

  //set cpi then start burst mode after
  write_reg_PMW(REG_Lift_Config , 0b00000011);
  set_CPI      (6400);
  write_reg_PMW(REG_Motion_Burst, 0x00);
  SPI.endTransaction();
}

// Gathers mouse data and sends it over the USB
void loop()
{
  // MERGED: Call the function to read buttons from the MCP expanders
  update_mcp_buttons();

  //check if there is new motion data from interrupt pin
  if (!digitalReadFast(PMW_MOTION))
  {
    // If there is enough time left or if no motion data is read yet read the motion register
    while (polling_rate < (set_rate - BURST_READ_TIME) || (!x && !y))
    {
      SPI.beginTransaction(SPI_SETTINGS_PMW);
      read_burst_start();
      // Burst data only available after 35 microseconds update other things in the mean time
      while (burst_timer < TSRAD_MOTION_BURST)
      {
        update_buttons();
        update_scroll();
      }
      read_burst_end();
      SPI.endTransaction();
    }
    update_usb();
    polling_rate = 0;
  }
  // If there is no motion data only update the buttons and scroll wheel
  else
  {
    update_cpi();
    update_buttons();
    update_scroll();
    if (buttons != buttons_old && (polling_rate >= set_rate))
    {
      update_usb();
      polling_rate = 0;
    }
  }
}
