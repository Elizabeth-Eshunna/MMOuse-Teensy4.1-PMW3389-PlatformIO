#define MS_0       29
#define MS_1       30
#define MC_NO      14
#define MC_NC      15

#define M1_NO      33
#define M1_NC      34
#define M2_NO      35
#define M2_NC      36
#define M3_NO      37
#define M3_NC      38
#define M4_NO      39
#define M4_NC      40
#define M5_NO      31
#define M5_NC      32


#define FAR_EDGE              0
#define NEAR_EDGE             1
#define RIGHT_EDGE            2 // This will be our G-Shift key
#define LEFT_TIP              3
#define RIGHT_TIP             4
#define LEFT_PUSH_TRIGGER     5
#define RIGHT_PUSH_TRIGGER    6
#define LEFT_PULL_TRIGGER     7
#define RIGHT_PULL_TRIGGER    8
#define FAR_WING              9
#define MID_WING              10
#define NEAR_WING             11
#define FAR_DPI               12
#define MID_DPI               13
#define NEAR_DPI              14
#define GRID_TOP_LEFT         15

// MCP2 (I2C Address 0x22)
#define GRID_LEFT             16
#define GRID_BOT_LEFT         17
#define GRID_BOT              18
#define GRID_BOT_RIGHT        19
#define GRID_RIGHT            20
#define GRID_FAR_TOP          21
#define GRID_FAR_BOT          22
// Unassigned buttons below (23-31)

//#define FAR_EDGE 0            //#define BUTTONa0 0
//#define NEAR_EDGE 1            //#define BUTTONa1 1
//#define RIGHT_EDGE 2            //#define BUTTONa2 2
//
//#define LEFT_TIP 3         //#define BUTTONa3 3
//#define RIGHT_TIP 4            //#define BUTTONa4 4
//
//#define LEFT_PUSH_TRIGGER 5            //#define BUTTONa5 5
//#define RIGHT_PUSH_TRIGGER 6            //#define BUTTONa6 6
//
//#define LEFT_PULL_TRIGGER 7            //#define BUTTONa7 7
//#define RIGHT_PULL_TRIGGER 8           //#define BUTTONa8 8
//
//#define FAR_WING 9            //#define BUTTONa9 9
//#define MID_WING 10           //#define BUTTONa10 10
//#define NEAR_WING 11            //#define BUTTONa11 11
//
//#define FAR_DPI 12            //#define BUTTONa12 12
//#define MID_DPI 13            //#define BUTTONa13 13
//#define NEAR_DPI 14            //#define BUTTONa14 14
//
//#define GRID_TOP_LEFT 15            //#define BUTTONa15 15       
//#define GRID_LEFT 16            //#define BUTTONb0 16
//#define GRID_BOT_LEFT 17            //#define BUTTONb1 17
//#define GRID_BOT 18            //#define BUTTONb2 18
//#define GRID_BOT_RIGHT 19            //#define BUTTONb3 19
//#define GRID_RIGHT 20            //#define BUTTONb4 20
//#define GRID_FAR_TOP 21            //#define BUTTONb5 21
//#define GRID_FAR_BOT 22            //#define BUTTONb6 22
            //#define BUTTONb7 23
            //#define BUTTONb8 24
            //#define BUTTONb9 25
            //#define BUTTONb10 26
            //#define BUTTONb11 27
            //#define BUTTONb12 28
            //#define BUTTONb13 29
            //#define BUTTONb14 30
            //#define BUTTONb15 31

extern const char* const buttonNames[32];
#pragma once

#include <Arduino.h>

void mcp_init();
void update_mcp_buttons();
bool mcp_get_g_shift_active();

// Button name array defined in src/mcp_buttons.cpp
extern const char* const buttonNames[32];
