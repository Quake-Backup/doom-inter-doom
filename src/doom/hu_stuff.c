//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2016-2019 Julian Nechaevsky
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:  Heads-up displays
//



#include <ctype.h>
#include <time.h>

#include "doomdef.h"
#include "doomkeys.h"
#include "z_zone.h"
#include "deh_main.h"
#include "i_input.h"
#include "i_swap.h"
#include "i_video.h"
#include "hu_stuff.h"
#include "hu_lib.h"
#include "m_controls.h"
#include "m_misc.h"
#include "w_wad.h"
#include "s_sound.h"
#include "doomstat.h"
#include "st_stuff.h" // [JN] ST_HEIGHT
#include "v_video.h"  // [JN] V_DrawPatch

// Data.
#include "rd_lang.h"
#include "sounds.h"

#include "crispy.h"
#include "jn.h"


//
// Locally used constants, shortcuts.
//

// DOOM 1 map names
#define HU_TITLE        (mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLE_RUS    (mapnames_rus[(gameepisode-1)*9+gamemap-1])

// DOOM 2 map names
#define HU_TITLE2       (mapnames_commercial[gamemap-1])
#define HU_TITLE2_RUS   (mapnames_commercial_rus[gamemap-1])

// Plutonia map names
#define HU_TITLEP       (mapnames_commercial[gamemap-1 + 32])
#define HU_TITLEP_RUS   (mapnames_commercial_rus[gamemap-1 + 32])

// TNT map names
#define HU_TITLET       (mapnames_commercial[gamemap-1 + 64])
#define HU_TITLET_RUS   (mapnames_commercial_rus[gamemap-1 + 64])

// No Rest for the Living map names
#define HU_TITLEN       (mapnames_commercial[gamemap-1 + 96])
#define HU_TITLEN_RUS   (mapnames_commercial_rus[gamemap-1 + 96])

// Atari Jaguar map names
#define HU_TITLEJ       (mapnames_commercial[gamemap-1 + 105])
#define HU_TITLEJ_RUS   (mapnames_commercial_rus[gamemap-1 + 105])

#define HU_TITLE_CHEX   (mapnames_chex[(gameepisode-1)*9+gamemap-1])
#define HU_TITLEHEIGHT  1
#define HU_TITLEX       0
// [JN] Initially HU_TITLEY is 167. 
// Moved a bit higher for possible non-standard font compatibility,
// and for preventing text shadows being dropped on status bar.
#define HU_TITLEY       (165 - SHORT(hu_font[0]->height))
// [JN] Jaguar: draw level name slightly higher
#define HU_TITLEY_JAG   (158 - SHORT(hu_font[0]->height))

#define HU_INPUTTOGGLE  't'
#define HU_INPUTX       HU_MSGX
#define HU_INPUTY       (HU_MSGY + HU_MSGHEIGHT*(SHORT(hu_font[0]->height) +1))
#define HU_INPUTWIDTH   64
#define HU_INPUTHEIGHT  1

#define HU_COORDX       (ORIGWIDTH - 8 * hu_font['A'-HU_FONTSTART]->width)


char *chat_macros[10] =
{
    HUSTR_CHATMACRO0,
    HUSTR_CHATMACRO1,
    HUSTR_CHATMACRO2,
    HUSTR_CHATMACRO3,
    HUSTR_CHATMACRO4,
    HUSTR_CHATMACRO5,
    HUSTR_CHATMACRO6,
    HUSTR_CHATMACRO7,
    HUSTR_CHATMACRO8,
    HUSTR_CHATMACRO9
};

char *chat_macros_rus[10] =
{
    HUSTR_CHATMACRO0_RUS,
    HUSTR_CHATMACRO1_RUS,
    HUSTR_CHATMACRO2_RUS,
    HUSTR_CHATMACRO3_RUS,
    HUSTR_CHATMACRO4_RUS,
    HUSTR_CHATMACRO5_RUS,
    HUSTR_CHATMACRO6_RUS,
    HUSTR_CHATMACRO7_RUS,
    HUSTR_CHATMACRO8_RUS,
    HUSTR_CHATMACRO9_RUS
};

char* player_names[] =
{
    HUSTR_PLRGREEN,
    HUSTR_PLRINDIGO,
    HUSTR_PLRBROWN,
    HUSTR_PLRRED
};

char* player_names_rus[] =
{
    HUSTR_PLRGREEN_RUS,
    HUSTR_PLRINDIGO_RUS,
    HUSTR_PLRBROWN_RUS,
    HUSTR_PLRRED_RUS
};

char        chat_char; // remove later.
static      player_t* plr;
patch_t*    hu_font[HU_FONTSIZE];
patch_t*    hu_font_small_eng[HU_FONTSIZE]; // [JN] Small, unchangeable English font (FNTSE)
patch_t*    hu_font_small_rus[HU_FONTSIZE]; // [JN] Small, unchangeable Russian font (FNTSR)
patch_t*    hu_font_big_eng[HU_FONTSIZE2];  // [JN] Big, unchangeable English font (FNTBE)
patch_t*    hu_font_big_rus[HU_FONTSIZE2];  // [JN] Big, unchangeable Russian font (FNTBR)
patch_t*    hu_font_gray[HU_FONTSIZE];  // [JN] Small gray STCFG font, used for local time widget

static      hu_textline_t w_title;
static      hu_textline_t w_kills;
static      hu_textline_t w_items;
static      hu_textline_t w_scrts;
static      hu_textline_t w_skill;
static      hu_textline_t w_ltime;
boolean     chat_on;
static      hu_itext_t w_chat;

static boolean always_off = false;
static char	    chat_dest[MAXPLAYERS];
static hu_itext_t w_inputbuffer[MAXPLAYERS];

static boolean  message_on;
boolean         message_dontfuckwithme;
static boolean  message_nottobefuckedwith;

static hu_stext_t w_message;
static int message_counter;

// [JN] Local time widget
static boolean  message_on_time;
static hu_stext_t w_message_time;

// [JN] FPS counter
static boolean  message_on_fps;
static hu_stext_t w_message_fps;

extern int showMessages;

static boolean headsupactive = false;


//
// Builtin map names.
// The actual names can be found in DStrings.h.
//

char* mapnames[] = // DOOM shareware/registered/retail (Ultimate) names.
{
    HUSTR_E1M1,
    HUSTR_E1M2,
    HUSTR_E1M3,
    HUSTR_E1M4,
    HUSTR_E1M5,
    HUSTR_E1M6,
    HUSTR_E1M7,
    HUSTR_E1M8,
    HUSTR_E1M9,

    HUSTR_E2M1,
    HUSTR_E2M2,
    HUSTR_E2M3,
    HUSTR_E2M4,
    HUSTR_E2M5,
    HUSTR_E2M6,
    HUSTR_E2M7,
    HUSTR_E2M8,
    HUSTR_E2M9,

    HUSTR_E3M1,
    HUSTR_E3M2,
    HUSTR_E3M3,
    HUSTR_E3M4,
    HUSTR_E3M5,
    HUSTR_E3M6,
    HUSTR_E3M7,
    HUSTR_E3M8,
    HUSTR_E3M9,

    HUSTR_E4M1,
    HUSTR_E4M2,
    HUSTR_E4M3,
    HUSTR_E4M4,
    HUSTR_E4M5,
    HUSTR_E4M6,
    HUSTR_E4M7,
    HUSTR_E4M8,
    HUSTR_E4M9,

    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL"
};

char* mapnames_rus[] =
{
    HUSTR_E1M1_RUS,
    HUSTR_E1M2_RUS,
    HUSTR_E1M3_RUS,
    HUSTR_E1M4_RUS,
    HUSTR_E1M5_RUS,
    HUSTR_E1M6_RUS,
    HUSTR_E1M7_RUS,
    HUSTR_E1M8_RUS,
    HUSTR_E1M9_RUS,

    HUSTR_E2M1_RUS,
    HUSTR_E2M2_RUS,
    HUSTR_E2M3_RUS,
    HUSTR_E2M4_RUS,
    HUSTR_E2M5_RUS,
    HUSTR_E2M6_RUS,
    HUSTR_E2M7_RUS,
    HUSTR_E2M8_RUS,
    HUSTR_E2M9_RUS,

    HUSTR_E3M1_RUS,
    HUSTR_E3M2_RUS,
    HUSTR_E3M3_RUS,
    HUSTR_E3M4_RUS,
    HUSTR_E3M5_RUS,
    HUSTR_E3M6_RUS,
    HUSTR_E3M7_RUS,
    HUSTR_E3M8_RUS,
    HUSTR_E3M9_RUS,

    HUSTR_E4M1_RUS,
    HUSTR_E4M2_RUS,
    HUSTR_E4M3_RUS,
    HUSTR_E4M4_RUS,
    HUSTR_E4M5_RUS,
    HUSTR_E4M6_RUS,
    HUSTR_E4M7_RUS,
    HUSTR_E4M8_RUS,
    HUSTR_E4M9_RUS,

    // [JN] "НОВЫЙ УРОВЕНЬ"
    "YJDSQ EHJDTYM",
    "YJDSQ EHJDTYM",
    "YJDSQ EHJDTYM",
    "YJDSQ EHJDTYM",
    "YJDSQ EHJDTYM",
    "YJDSQ EHJDTYM",
    "YJDSQ EHJDTYM",
    "YJDSQ EHJDTYM",
    "YJDSQ EHJDTYM"
};


char* mapnames_chex[] = // Chex Quest names.
{
    HUSTR_E1M1,
    HUSTR_E1M2,
    HUSTR_E1M3,
    HUSTR_E1M4,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,

    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,

    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,

    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,
    HUSTR_E1M5,

    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL"
};


// List of names for levels in commercial IWADs
// (doom2.wad, plutonia.wad, tnt.wad).  These are stored in a
// single large array; WADs like pl2.wad have a MAP33, and rely on
// the layout in the Vanilla executable, where it is possible to
// overflow the end of one array into the next.

char* mapnames_commercial[] =
{
    // DOOM 2 map names.
    HUSTR_1,
    HUSTR_2,
    HUSTR_3,
    HUSTR_4,
    HUSTR_5,
    HUSTR_6,
    HUSTR_7,
    HUSTR_8,
    HUSTR_9,
    HUSTR_10,
    HUSTR_11,

    HUSTR_12,
    HUSTR_13,
    HUSTR_14,
    HUSTR_15,
    HUSTR_16,
    HUSTR_17,
    HUSTR_18,
    HUSTR_19,
    HUSTR_20,

    HUSTR_21,
    HUSTR_22,
    HUSTR_23,
    HUSTR_24,
    HUSTR_25,
    HUSTR_26,
    HUSTR_27,
    HUSTR_28,
    HUSTR_29,
    HUSTR_30,
    HUSTR_31,
    HUSTR_32,

    // Plutonia WAD map names.
    PHUSTR_1,
    PHUSTR_2,
    PHUSTR_3,
    PHUSTR_4,
    PHUSTR_5,
    PHUSTR_6,
    PHUSTR_7,
    PHUSTR_8,
    PHUSTR_9,
    PHUSTR_10,
    PHUSTR_11,

    PHUSTR_12,
    PHUSTR_13,
    PHUSTR_14,
    PHUSTR_15,
    PHUSTR_16,
    PHUSTR_17,
    PHUSTR_18,
    PHUSTR_19,
    PHUSTR_20,

    PHUSTR_21,
    PHUSTR_22,
    PHUSTR_23,
    PHUSTR_24,
    PHUSTR_25,
    PHUSTR_26,
    PHUSTR_27,
    PHUSTR_28,
    PHUSTR_29,
    PHUSTR_30,
    PHUSTR_31,
    PHUSTR_32,

    // TNT WAD map names.
    THUSTR_1,
    THUSTR_2,
    THUSTR_3,
    THUSTR_4,
    THUSTR_5,
    THUSTR_6,
    THUSTR_7,
    THUSTR_8,
    THUSTR_9,
    THUSTR_10,
    THUSTR_11,

    THUSTR_12,
    THUSTR_13,
    THUSTR_14,
    THUSTR_15,
    THUSTR_16,
    THUSTR_17,
    THUSTR_18,
    THUSTR_19,
    THUSTR_20,

    THUSTR_21,
    THUSTR_22,
    THUSTR_23,
    THUSTR_24,
    THUSTR_25,
    THUSTR_26,
    THUSTR_27,
    THUSTR_28,
    THUSTR_29,
    THUSTR_30,
    THUSTR_31,
    THUSTR_32,

    // Нет покоя для живых.
    NHUSTR_1,
    NHUSTR_2,
    NHUSTR_3,
    NHUSTR_4,
    NHUSTR_5,
    NHUSTR_6,
    NHUSTR_7,
    NHUSTR_8,
    NHUSTR_9,

    // [JN] Atari Jaguar
    JHUSTR_1,
    JHUSTR_2,
    JHUSTR_3,
    JHUSTR_4,
    JHUSTR_5,
    JHUSTR_6,
    JHUSTR_7,
    JHUSTR_8,
    JHUSTR_9,
    JHUSTR_10,
    JHUSTR_11,
    JHUSTR_12,
    JHUSTR_13,
    JHUSTR_14,
    JHUSTR_15,
    JHUSTR_16,
    JHUSTR_17,
    JHUSTR_18,
    JHUSTR_19,
    JHUSTR_20,
    JHUSTR_21,
    JHUSTR_22,
    JHUSTR_23,
    JHUSTR_24,
    JHUSTR_25,
    JHUSTR_26
};

char* mapnames_commercial_rus[] =
{
    // DOOM 2 map names.
    HUSTR_1_RUS,
    HUSTR_2_RUS,
    HUSTR_3_RUS,
    HUSTR_4_RUS,
    HUSTR_5_RUS,
    HUSTR_6_RUS,
    HUSTR_7_RUS,
    HUSTR_8_RUS,
    HUSTR_9_RUS,
    HUSTR_10_RUS,
    HUSTR_11_RUS,

    HUSTR_12_RUS,
    HUSTR_13_RUS,
    HUSTR_14_RUS,
    HUSTR_15_RUS,
    HUSTR_16_RUS,
    HUSTR_17_RUS,
    HUSTR_18_RUS,
    HUSTR_19_RUS,
    HUSTR_20_RUS,

    HUSTR_21_RUS,
    HUSTR_22_RUS,
    HUSTR_23_RUS,
    HUSTR_24_RUS,
    HUSTR_25_RUS,
    HUSTR_26_RUS,
    HUSTR_27_RUS,
    HUSTR_28_RUS,
    HUSTR_29_RUS,
    HUSTR_30_RUS,
    HUSTR_31_RUS,
    HUSTR_32_RUS,

    // Plutonia WAD map names.
    PHUSTR_1_RUS,
    PHUSTR_2_RUS,
    PHUSTR_3_RUS,
    PHUSTR_4_RUS,
    PHUSTR_5_RUS,
    PHUSTR_6_RUS,
    PHUSTR_7_RUS,
    PHUSTR_8_RUS,
    PHUSTR_9_RUS,
    PHUSTR_10_RUS,
    PHUSTR_11_RUS,

    PHUSTR_12_RUS,
    PHUSTR_13_RUS,
    PHUSTR_14_RUS,
    PHUSTR_15_RUS,
    PHUSTR_16_RUS,
    PHUSTR_17_RUS,
    PHUSTR_18_RUS,
    PHUSTR_19_RUS,
    PHUSTR_20_RUS,

    PHUSTR_21_RUS,
    PHUSTR_22_RUS,
    PHUSTR_23_RUS,
    PHUSTR_24_RUS,
    PHUSTR_25_RUS,
    PHUSTR_26_RUS,
    PHUSTR_27_RUS,
    PHUSTR_28_RUS,
    PHUSTR_29_RUS,
    PHUSTR_30_RUS,
    PHUSTR_31_RUS,
    PHUSTR_32_RUS,

    // TNT WAD map names.
    THUSTR_1_RUS,
    THUSTR_2_RUS,
    THUSTR_3_RUS,
    THUSTR_4_RUS,
    THUSTR_5_RUS,
    THUSTR_6_RUS,
    THUSTR_7_RUS,
    THUSTR_8_RUS,
    THUSTR_9_RUS,
    THUSTR_10_RUS,
    THUSTR_11_RUS,

    THUSTR_12_RUS,
    THUSTR_13_RUS,
    THUSTR_14_RUS,
    THUSTR_15_RUS,
    THUSTR_16_RUS,
    THUSTR_17_RUS,
    THUSTR_18_RUS,
    THUSTR_19_RUS,
    THUSTR_20_RUS,

    THUSTR_21_RUS,
    THUSTR_22_RUS,
    THUSTR_23_RUS,
    THUSTR_24_RUS,
    THUSTR_25_RUS,
    THUSTR_26_RUS,
    THUSTR_27_RUS,
    THUSTR_28_RUS,
    THUSTR_29_RUS,
    THUSTR_30_RUS,
    THUSTR_31_RUS,
    THUSTR_32_RUS,

    // Нет покоя для живых.
    NHUSTR_1_RUS,
    NHUSTR_2_RUS,
    NHUSTR_3_RUS,
    NHUSTR_4_RUS,
    NHUSTR_5_RUS,
    NHUSTR_6_RUS,
    NHUSTR_7_RUS,
    NHUSTR_8_RUS,
    NHUSTR_9_RUS,

    // [JN] Atari Jaguar
    JHUSTR_1_RUS,
    JHUSTR_2_RUS,
    JHUSTR_3_RUS,
    JHUSTR_4_RUS,
    JHUSTR_5_RUS,
    JHUSTR_6_RUS,
    JHUSTR_7_RUS,
    JHUSTR_8_RUS,
    JHUSTR_9_RUS,
    JHUSTR_10_RUS,
    JHUSTR_11_RUS,
    JHUSTR_12_RUS,
    JHUSTR_13_RUS,
    JHUSTR_14_RUS,
    JHUSTR_15_RUS,
    JHUSTR_16_RUS,
    JHUSTR_17_RUS,
    JHUSTR_18_RUS,
    JHUSTR_19_RUS,
    JHUSTR_20_RUS,
    JHUSTR_21_RUS,
    JHUSTR_22_RUS,
    JHUSTR_23_RUS,
    JHUSTR_24_RUS,
    JHUSTR_25_RUS,
    JHUSTR_26_RUS
};


void HU_Init(void)
{
    int     i;
    int     j;
    int     o, p;
    int     q, r;
    
    int     g;
    char    buffer[9];

    // load the heads-up font
    j = HU_FONTSTART;
    o = HU_FONTSTART;
    p = HU_FONTSTART;
    q = HU_FONTSTART2;
    r = HU_FONTSTART2;
    g = HU_FONTSTART_GRAY;

    // [JN] Standard STCFN font
    for (i=0;i<HU_FONTSIZE;i++)
    {
        DEH_snprintf(buffer, 9, "STCFN%.3d", j++);
        hu_font[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }

    // [JN] Small, unchangeable English font (FNTSE)
    for (i=0;i<HU_FONTSIZE;i++)
    {
        DEH_snprintf(buffer, 9, "FNTSE%.3d", o++);
        hu_font_small_eng[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }

    // [JN] Small, unchangeable Russian font (FNTSR)
    for (i=0;i<HU_FONTSIZE;i++)
    {
        DEH_snprintf(buffer, 9, "FNTSR%.3d", p++);
        hu_font_small_rus[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }

    // [JN] Big, unchangeable English font (FNTBE)
    for (i=0;i<HU_FONTSIZE2;i++)
    {
        DEH_snprintf(buffer, 9, "FNTBE%.3d", q++);
        hu_font_big_eng[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }

    // [JN] Big, unchangeable Russian font (FNTBR)
    for (i=0;i<HU_FONTSIZE2;i++)
    {
        DEH_snprintf(buffer, 9, "FNTBR%.3d", r++);
        hu_font_big_rus[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }    

    // [JN] Small gray STCFG font, used for local time widget and FPS counter
    for (i=0;i<HU_FONTSIZE_GRAY;i++)
    {
        DEH_snprintf(buffer, 9, "STCFG%.3d", g++);
        hu_font_gray[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }
}


void HU_Stop(void) 
{
    headsupactive = false;
}


void HU_Start(void)
{
    int     i;
    char*   s;

    if (headsupactive)
    HU_Stop();

    plr = &players[consoleplayer];
    message_on = false;
    message_on_time = true; // [JN] Local time widget
    message_on_fps = true;  // [JN] FPS counter
    message_dontfuckwithme = false;
    message_nottobefuckedwith = false;
    chat_on = false;

    // create the message widget
    HUlib_initSText(&w_message, HU_MSGX, HU_MSGY, HU_MSGHEIGHT, 
                    english_language ? hu_font : hu_font_small_rus,
                    HU_FONTSTART, &message_on);

    // [JN] Create the local time widget
#ifdef WIDESCREEN
    HUlib_initSText(&w_message_time, 400, 10, HU_MSGHEIGHT, hu_font_gray, HU_FONTSTART, &message_on_time);
    HUlib_initSText(&w_message_fps, 390, 20, HU_MSGHEIGHT, hu_font_gray, HU_FONTSTART, &message_on_fps);
#else
    HUlib_initSText(&w_message_time, 294, 10, HU_MSGHEIGHT, hu_font_gray, HU_FONTSTART, &message_on_time);
    HUlib_initSText(&w_message_fps, 294, 20, HU_MSGHEIGHT, hu_font_gray, HU_FONTSTART, &message_on_fps);
#endif

    // create the map title widget
    HUlib_initTextLine(&w_title, HU_TITLEX, (gamemission == jaguar ?
                                             HU_TITLEY_JAG :
                                             HU_TITLEY),
                                             english_language ? hu_font : hu_font_small_rus, 
                                             HU_FONTSTART);

    HUlib_initTextLine(&w_kills,
		       HU_TITLEX, (HU_MSGY+1) + 1 * 8,
		       english_language ? hu_font : hu_font_small_rus,
		       HU_FONTSTART);

    HUlib_initTextLine(&w_items,
		       HU_TITLEX, (HU_MSGY+1) + 2 * 8,
		       english_language ? hu_font : hu_font_small_rus,
		       HU_FONTSTART);

    HUlib_initTextLine(&w_scrts,
		       HU_TITLEX, (HU_MSGY+1) + 3 * 8,
		       english_language ? hu_font : hu_font_small_rus,
		       HU_FONTSTART);

    HUlib_initTextLine(&w_skill,
		       HU_TITLEX, (HU_MSGY+1) + 4 * 8,
		       english_language ? hu_font : hu_font_small_rus,
		       HU_FONTSTART);

    HUlib_initTextLine(&w_ltime,
		       HU_TITLEX, (HU_MSGY+1) + 6 * 8,
		       english_language ? hu_font : hu_font_small_rus,
		       HU_FONTSTART);

    switch ( logical_gamemission )
    {
        case doom:
        s = english_language ? HU_TITLE : HU_TITLE_RUS;
        break;

        case doom2:
        s = english_language ? HU_TITLE2 : HU_TITLE2_RUS;
        break;

        case pack_plut:
        s = english_language ? HU_TITLEP : HU_TITLEP_RUS;
        break;

        case pack_tnt:
        s = english_language ? HU_TITLET : HU_TITLET_RUS;
        break;

        case pack_nerve:
        if (gamemap <= 9)
        s = english_language ? HU_TITLEN : HU_TITLEN_RUS;
        else
        s = english_language ? HU_TITLE2 : HU_TITLE2_RUS;
        break;

        case jaguar:
        if (gamemap <= 26)
        s = english_language ? HU_TITLEJ : HU_TITLEJ_RUS;
        else
        s = english_language ? HU_TITLE2 : HU_TITLE2_RUS;
        break;
        
        default:
        s = english_language ? "Unknown level" : "ytbpdtcnysq ehjdtym"; // [JN] "Неизвестный уровень"
        break;
    }

    if (logical_gamemission == doom && gameversion == exe_chex)
    {
        s = HU_TITLE_CHEX;
    }

    // dehacked substitution to get modified level name
    s = DEH_String(s);

    while (*s)
    HUlib_addCharToTextLine(&w_title, *(s++));

    // create the chat widget
    HUlib_initIText(&w_chat, HU_INPUTX, HU_INPUTY, hu_font, HU_FONTSTART, &chat_on);

    // create the inputbuffer widgets
    for (i=0 ; i<MAXPLAYERS ; i++)
    HUlib_initIText(&w_inputbuffer[i], 0, 0, 0, 0, &always_off);

    headsupactive = true;
}


void HU_Drawer(void)
{
    HUlib_drawSText(&w_message);
    if (local_time)
    {
        // [JN] Draw local time widget
        HUlib_drawSText(&w_message_time);
    }
    if (show_fps && !vanillaparm)
    {
        // [JN] Draw FPS counter
        HUlib_drawSText(&w_message_fps);
    }
    HUlib_drawIText(&w_chat);

    if (automapactive)
    {
        static char str[32], *s;
        int time = leveltime / TICRATE;

        HUlib_drawTextLineUncolored(&w_title, false);

        // [from-crispy] Show level stats in automap
        if (!vanillaparm && automap_stats)
        {
            sprintf(str, english_language ?
                         "Kills: %d/%d" : "dhfub: %d*%d",
                         players[consoleplayer].killcount, totalkills);
            HUlib_clearTextLine(&w_kills);
            s = str;
            while (*s)
                HUlib_addCharToTextLine(&w_kills, *(s++));
            HUlib_drawTextLineUncolored(&w_kills, false);
    
            sprintf(str, english_language ?
                         "Items: %d/%d" : "ghtlvtns: %d*%d",
                         players[consoleplayer].itemcount, totalitems);
            HUlib_clearTextLine(&w_items);
            s = str;
            while (*s)
                HUlib_addCharToTextLine(&w_items, *(s++));
            HUlib_drawTextLineUncolored(&w_items, false);
    
            sprintf(str, english_language ?
                         "Secret: %d/%d" : "nfqybrb: %d*%d",
                         players[consoleplayer].secretcount, totalsecret);
            HUlib_clearTextLine(&w_scrts);
            s = str;
            while (*s)
                HUlib_addCharToTextLine(&w_scrts, *(s++));
            HUlib_drawTextLineUncolored(&w_scrts, false);
            sprintf(str, english_language ?
                         "Skill: %d" : "ckj;yjcnm: %d",
                         gameskill+1);
            HUlib_clearTextLine(&w_skill);
            s = str;
            while (*s)
                HUlib_addCharToTextLine(&w_skill, *(s++));
            HUlib_drawTextLineUncolored(&w_skill, false);
            sprintf(str, "%02d:%02d:%02d", time/3600, (time%3600)/60, time%60);
            HUlib_clearTextLine(&w_ltime);
            s = str;
            while (*s)
                HUlib_addCharToTextLine(&w_ltime, *(s++));
            HUlib_drawTextLineUncolored(&w_ltime, false);
        }
    }

    // [JN] Draw crosshair. 
    // Thanks to Fabian Greffrath for ORIGWIDTH, ORIGHEIGHT and ST_HEIGHT values,
    // thanks to Zodomaniac for proper health values!
    if (!vanillaparm && !automapactive && crosshair_draw)
    {
#ifdef WIDESCREEN
        if (crosshair_scale)
        {   // Scaled crosshair
            V_DrawPatch(ORIGWIDTH/2,
                       (ORIGHEIGHT+2)/2,
                W_CacheLumpName(DEH_String(!crosshair_health ?
                                           "XHAIRSR" :             // Red (only)
                                           plr->health >= 67 ?
                                           "XHAIRSG" :             // Green
                                           plr->health >= 34 ?
                                           "XHAIRSY" : "XHAIRSR"), // Yellow or Red
                                           PU_CACHE));
        }
        else
        {   // Unscaled crosshair
            V_DrawPatchUnscaled(SCREENWIDTH/2,
                               (SCREENHEIGHT+4)/2,
                W_CacheLumpName(DEH_String(!crosshair_health ? 
                                           "XHAIRUR" :              // Red (only)
                                           plr->health >= 67 ?
                                           "XHAIRUG" :              // Green
                                           plr->health >= 34 ?
                                           "XHAIRUY" : "XHAIRUR"),  // Yellow or Red
                                           PU_CACHE));
        }
#else
        if (crosshair_scale)
        {   // Scaled crosshair
            V_DrawPatch(ORIGWIDTH/2, 
                ((screenblocks <= 10) ? (ORIGHEIGHT-ST_HEIGHT+2)/2 : (ORIGHEIGHT+2)/2),
                W_CacheLumpName(DEH_String(!crosshair_health ?
                                           "XHAIRSR" :             // Red (only)
                                           plr->health >= 67 ?
                                           "XHAIRSG" :             // Green
                                           plr->health >= 34 ?
                                           "XHAIRSY" : "XHAIRSR"), // Yellow or Red
                                           PU_CACHE));
        }
        else
        {   // Unscaled crosshair
            V_DrawPatchUnscaled(SCREENWIDTH/2,
                ((screenblocks <= 10) ? (SCREENHEIGHT-ST_HEIGHT-26)/2 : (SCREENHEIGHT+4)/2),
                W_CacheLumpName(DEH_String(!crosshair_health ? 
                                           "XHAIRUR" :              // Red (only)
                                           plr->health >= 67 ?
                                           "XHAIRUG" :             // Green
                                           plr->health >= 34 ?
                                           "XHAIRUY" : "XHAIRUR"),  // Yellow or Red
                                           PU_CACHE));
        }
#endif
    }
}


void HU_Erase(void)
{
    HUlib_eraseSText(&w_message);
    if (local_time)
    {
        // [JN] Erase local time widget
        HUlib_eraseSText(&w_message_time);
    }
    if (show_fps && !vanillaparm)
    {
        // [JN] Erase FPS counter
        HUlib_eraseSText(&w_message_fps);
    }
    HUlib_eraseIText(&w_chat);
    HUlib_eraseTextLine(&w_title);
}


void HU_Ticker(void)
{
    int     i, rc;
    char    c;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    static char s[64];
    static char f[64];
    strftime(s, sizeof(s), "%H:%M", tm);

    // [JN] Start local time widget
    if (local_time)
    plr->message_time = (s);

    if (show_fps && !vanillaparm)
    {
        M_snprintf(f, sizeof(f), "! %d", real_fps); // [JN] 1 = FPS, see STCFG033 (doom-sysfont.wad)
        plr->message_fps = (f);
    }

    // tick down message counter if message is up
    if (message_counter && !--message_counter)
    {
        message_on = false;
        message_nottobefuckedwith = false;
    }

    if (showMessages || message_dontfuckwithme)
    {
        // display message if necessary
        if ((plr->message && !message_nottobefuckedwith) || (plr->message && message_dontfuckwithme))
        {
            HUlib_addMessageToSText(&w_message, 0, plr->message);
            plr->message = 0;
            message_on = true;
            message_counter = HU_MSGTIMEOUT;
            message_nottobefuckedwith = message_dontfuckwithme;
            message_dontfuckwithme = 0;
        }
    } // else message_on = false;

    // [JN] Handling local time widget
    if (plr->message_time)
    {
        HUlib_addMessageToSText(&w_message_time, 0, plr->message_time);
        plr->message_time = 0;
        message_on_time = true;
    }

    // [JN] Handling local time widget
    if (plr->message_fps)
    {
        HUlib_addMessageToSText(&w_message_fps, 0, plr->message_fps);
        plr->message_fps = 0;
        message_on_fps = true;
    }

    // check for incoming chat characters
    if (netgame)
    {
        for (i=0 ; i<MAXPLAYERS; i++)
        {
            if (!playeringame[i])
            continue;

            if (i != consoleplayer && (c = players[i].cmd.chatchar))
            {
                if (c <= HU_BROADCAST)
                {
                    chat_dest[i] = c;
                }
                else
                {
                    rc = HUlib_keyInIText(&w_inputbuffer[i], c);

                    if (rc && c == KEY_ENTER)
                    {
                        if (w_inputbuffer[i].l.len && (chat_dest[i] == consoleplayer+1 || chat_dest[i] == HU_BROADCAST))
                        {
                            HUlib_addMessageToSText(&w_message, DEH_String(english_language ? 
                                                                           player_names[i] :
                                                                           player_names_rus[i]),
                                                                           w_inputbuffer[i].l.l);

                            message_nottobefuckedwith = true;
                            message_on = true;
                            message_counter = HU_MSGTIMEOUT;

                            if ( gamemode == commercial )
                            S_StartSound(0, sfx_radio);
                            else
                            S_StartSound(0, sfx_tink);
                        }

                        HUlib_resetIText(&w_inputbuffer[i]);
                    }
                }

                players[i].cmd.chatchar = 0;
            }
        }
    }
}


#define QUEUESIZE 128

static char chatchars[QUEUESIZE];
static int  head = 0;
static int  tail = 0;


void HU_queueChatChar(char c)
{
    if (((head + 1) & (QUEUESIZE-1)) == tail)
    {
        plr->message = DEH_String(english_language ? HUSTR_MSGU : HUSTR_MSGU_RUS);
    }
    else
    {
        chatchars[head] = c;
        head = (head + 1) & (QUEUESIZE-1);
    }
}


char HU_dequeueChatChar(void)
{
    char c;

    if (head != tail)
    {
        c = chatchars[tail];
        tail = (tail + 1) & (QUEUESIZE-1);
    }
    else
    {
        c = 0;
    }

    return c;
}


static void StartChatInput(int dest)
{
    chat_on = true;
    HUlib_resetIText(&w_chat);
    HU_queueChatChar(HU_BROADCAST);

    I_StartTextInput(0, 8, SCREENWIDTH, 16);
}


static void StopChatInput(void)
{
    chat_on = false;
    I_StopTextInput();
}


boolean HU_Responder(event_t *ev)
{
    static char lastmessage[HU_MAXLINELENGTH+1];
    char*       macromessage;
    boolean     eatkey = false;

    static boolean  altdown = false;
    unsigned char   c;
    int             i;
    int             numplayers;

    static int      num_nobrainers = 0;

    numplayers = 0;
    for (i=0 ; i<MAXPLAYERS ; i++)
    numplayers += playeringame[i];

    if (ev->data1 == KEY_RSHIFT)
    {
        return false;
    }
    else if (ev->data1 == KEY_RALT || ev->data1 == KEY_LALT)
    {
        altdown = ev->type == ev_keydown;
        return false;
    }

    if (ev->type != ev_keydown)
    return false;

    if (!chat_on)
    {
        if (ev->data1 == key_message_refresh)
        {
            message_on = true;
            message_counter = HU_MSGTIMEOUT;
            eatkey = true;
        }
        else if (netgame && ev->data2 == key_multi_msg)
        {
            eatkey = true;
            StartChatInput(HU_BROADCAST);
        }
        else if (netgame && numplayers > 2)
        {
            for (i=0; i<MAXPLAYERS ; i++)
            {
                if (ev->data2 == key_multi_msgplayer[i])
                {
                    if (playeringame[i] && i!=consoleplayer)
                    {
                        eatkey = true;
                        StartChatInput(i + 1);
                        break;
                    }
                    else if (i == consoleplayer)
                    {
                        num_nobrainers++;
                        if (num_nobrainers < 3)
                            plr->message = DEH_String(english_language ?
                                                      HUSTR_TALKTOSELF1 : HUSTR_TALKTOSELF1_RUS);
                        else if (num_nobrainers < 6)
                            plr->message = DEH_String(english_language ?
                                                      HUSTR_TALKTOSELF2 : HUSTR_TALKTOSELF2_RUS);
                        else if (num_nobrainers < 9)
                            plr->message = DEH_String(english_language ?
                                                      HUSTR_TALKTOSELF3 : HUSTR_TALKTOSELF3_RUS);
                        else if (num_nobrainers < 32)
                            plr->message = DEH_String(english_language ?
                                                      HUSTR_TALKTOSELF4 : HUSTR_TALKTOSELF4_RUS);
                        else
                            plr->message = DEH_String(english_language ?
                                                      HUSTR_TALKTOSELF5 : HUSTR_TALKTOSELF5_RUS);
                    }
                }
            }
        }
    }
    else
    {
    // send a macro
    if (altdown)
    {
        c = ev->data1 - '0';
        if (c > 9)
        return false;
        // fprintf(stderr, "got here\n");
        macromessage = english_language ? chat_macros[c] : chat_macros_rus[c];

        // kill last message with a '\n'
        HU_queueChatChar(KEY_ENTER); // DEBUG!!!

        // send the macro message
        while (*macromessage)
        HU_queueChatChar(*macromessage++);
        HU_queueChatChar(KEY_ENTER);

        // leave chat mode and notify that it was sent
        StopChatInput();
        M_StringCopy(lastmessage, english_language ?
                                  chat_macros[c] : chat_macros_rus[c],
                                  sizeof(lastmessage));
        plr->message = lastmessage;
        eatkey = true;
    }
    else
    {
        c = ev->data3;

        eatkey = HUlib_keyInIText(&w_chat, c);
        if (eatkey)
        {
            // static unsigned char buf[20]; // DEBUG
            HU_queueChatChar(c);

            // M_snprintf(buf, sizeof(buf), "KEY: %d => %d", ev->data1, c);
            //        plr->message = buf;
        }
        if (c == KEY_ENTER)
        {
            StopChatInput();
            if (w_chat.l.len)
            {
                M_StringCopy(lastmessage, w_chat.l.l, sizeof(lastmessage));
                plr->message = lastmessage;
            }
        }
        else if (c == KEY_ESCAPE)
        {
            StopChatInput();
        }
    }
    }

    return eatkey;
}

