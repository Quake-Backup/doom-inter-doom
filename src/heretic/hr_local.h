//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2016-2022 Julian Nechaevsky
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


#pragma once

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "doomtype.h"
#include "id_lang.h"    // all important printed strings
#include "info.h"       // header generated by multigen utility
#include "w_wad.h"      // WAD file access
#include "m_fixed.h"    // fixed_t
#include "tables.h"     // angle_t 
#include "d_event.h"    // events
#include "d_mode.h"     // gamemode/mission
#include "d_ticcmd.h"   // ticcmd_t
#include "d_loop.h"
#include "v_patch.h"
#include "rd_menu_control.h"
#include "rd_text.h"
#include "z_zone.h"


#define HERETIC_VERSION       130
#define HERETIC_VERSION_TEXT  "v1.3"

// If rangecheck is undefined, most parameter validation
// debugging code will not be compiled:
// #define RANGECHECK

/*
===============================================================================

						map level types

===============================================================================
*/

enum  // lump order in a map wad
{
    ML_LABEL,
    ML_THINGS,
    ML_LINEDEFS,
    ML_SIDEDEFS,
    ML_VERTEXES,
    ML_SEGS,
    ML_SSECTORS,
    ML_NODES,
    ML_SECTORS,
    ML_REJECT,
    ML_BLOCKMAP
};

// A single Vertex.
typedef PACKED_STRUCT (
{
    short x;
    short y;
}) mapvertex_t;

// A SideDef, defining the visual appearance of a wall,
// by setting textures and offsets.
typedef PACKED_STRUCT (
{
    short textureoffset;
    short rowoffset;
    char  toptexture[8];
    char  bottomtexture[8];
    char  midtexture[8];
    // Front sector, towards viewer.
    short sector;
}) mapsidedef_t;

// A LineDef, as used for editing, and as input
// to the BSP builder.
typedef PACKED_STRUCT (
{
    unsigned short v1;
    unsigned short v2;
    unsigned short flags;
    short special;
    short tag;
    // sidenum[1] will be -1 if one sided
    unsigned short sidenum[2];
}) maplinedef_t;

// [crispy] allow loading of Hexen-format maps
// taken from chocolate-doom/src/hexen/xddefs.h:63-75
typedef PACKED_STRUCT (
{
    short v1;
    short v2;
    short flags;
    byte special;
    byte arg1;
    byte arg2;
    byte arg3;
    byte arg4;
    byte arg5;
    short sidenum[2];
}) maplinedef_hexen_t;

#define	ML_BLOCKING	        1  // Solid, is an obstacle.
#define	ML_BLOCKMONSTERS    2  // Blocks monsters only.
#define	ML_TWOSIDED         4  // Backside will not be present at all if not two sided

// if a texture is pegged, the texture will have the end exposed to air held
// constant at the top or bottom of the texture (stairs or pulled down things)
// and will move with a height change of one of the neighbor sectors
// Unpegged textures allways have the first row of the texture at the top
// pixel of the line for both top and bottom textures (windows)

#define	ML_DONTPEGTOP       8       // upper texture unpegged
#define	ML_DONTPEGBOTTOM    16      // lower texture unpegged
#define ML_SECRET           32      // don't map as two sided: IT'S A SECRET!
#define ML_SOUNDBLOCK       64      // don't let sound cross two of these
#define	ML_DONTDRAW         128     // don't draw on the automap
#define	ML_MAPPED           256     // set if allready drawn in automap

// Sector definition, from editing.
typedef	PACKED_STRUCT (
{
    short floorheight;
    short ceilingheight;
    char  floorpic[8];
    char  ceilingpic[8];
    short lightlevel;
    short special;
    short tag;
}) mapsector_t;

// SubSector, as generated by BSP.
typedef PACKED_STRUCT (
{
    unsigned short numsegs;
    // Index of first one, segs are stored sequentially.
    unsigned short firstseg;
}) mapsubsector_t;

// [crispy] allow loading of maps with DeePBSP nodes
// taken from prboom-plus/src/doomdata.h:163-166
typedef PACKED_STRUCT (
{
    unsigned short numsegs;
    int firstseg;
}) mapsubsector_deepbsp_t;

// [crispy] allow loading of maps with ZDBSP nodes
// taken from prboom-plus/src/doomdata.h:168-170
typedef PACKED_STRUCT (
{
    unsigned int numsegs;
}) mapsubsector_zdbsp_t;

// LineSeg, generated by splitting LineDefs
// using partition lines selected by BSP builder.
typedef PACKED_STRUCT (
{
    unsigned short v1;
    unsigned short v2;
    short angle;		
    unsigned short linedef;
    short side;
    short offset;
}) mapseg_t;

// [crispy] allow loading of maps with DeePBSP nodes
// taken from prboom-plus/src/doomdata.h:183-190
typedef PACKED_STRUCT (
{
    int v1;
    int v2;
    unsigned short angle;
    unsigned short linedef;
    short side;
    unsigned short offset;
}) mapseg_deepbsp_t;

// [crispy] allow loading of maps with ZDBSP nodes
// taken from prboom-plus/src/doomdata.h:192-196
typedef PACKED_STRUCT (
{
    unsigned int v1, v2;
    unsigned short linedef;
    unsigned char side;
}) mapseg_zdbsp_t;

// Indicate a leaf.
#define NF_SUBSECTOR    0x80000000
#define NO_INDEX        ((unsigned short)-1)

typedef PACKED_STRUCT (
{
    // Partition line from (x,y) to x+dx,y+dy)
    short x;
    short y;
    short dx;
    short dy;

    // Bounding box for each child,
    // clip against view frustum.
    short bbox[2][4];

    // If NF_SUBSECTOR its a subsector,
    // else it's a node of another subtree.
    unsigned short children[2];

}) mapnode_t;

// [crispy] allow loading of maps with DeePBSP nodes
// taken from prboom-plus/src/doomdata.h:216-225
typedef PACKED_STRUCT (
{
    short x;
    short y;
    short dx;
    short dy;
    short bbox[2][4];
    int children[2];
}) mapnode_deepbsp_t;

// [crispy] allow loading of maps with ZDBSP nodes
// taken from prboom-plus/src/doomdata.h:227-136
typedef PACKED_STRUCT (
{
    short x;
    short y;
    short dx;
    short dy;
    short bbox[2][4];
    int children[2];
}) mapnode_zdbsp_t;

// Thing definition, position, orientation and type,
// plus skill/visibility flags and attributes.
typedef PACKED_STRUCT (
{
    short x;
    short y;
    short angle;
    short type;
    short options;
}) mapthing_t;

// [crispy] allow loading of Hexen-format maps
// taken from chocolate-doom/src/hexen/xddefs.h:134-149
typedef struct
{
    short tid;
    short x;
    short y;
    short height;
    short angle;
    short type;
    short options;
    byte special;
    byte arg1;
    byte arg2;
    byte arg3;
    byte arg4;
    byte arg5;
} PACKEDATTR mapthing_hexen_t;

#define	MTF_EASY        1
#define	MTF_NORMAL      2
#define	MTF_HARD        4
#define	MTF_AMBUSH      8

/*
===============================================================================

						texture definition

===============================================================================
*/

// Each texture is composed of one or more patches, with patches being lumps
// stored in the WAD. The lumps are referenced by number, and patched into the
// rectangular texture space using origin and possibly other attributes.

typedef PACKED_STRUCT (
{
    short originx;
    short originy;
    short patch;
    short stepdir;
    short colormap;
}) mappatch_t;

// A Heretic wall texture is a list of patches 
// which are to be combined in a predefined order.

typedef PACKED_STRUCT (
{
    char       name[8];
    int	       masked;	
    short      width;
    short      height;
    int        obsolete;
    short      patchcount;
    mappatch_t patches[1];
}) maptexture_t;

/*
===============================================================================

						GLOBAL TYPES

===============================================================================
*/

#define NUMARTIFCTS     28
#define MAXPLAYERS      4

#define	BT_ATTACK       1
#define	BT_USE          2
#define	BT_CHANGE       4       // if true, the next 3 bits hold weapon num
#define	BT_WEAPONMASK   (8+16+32)
#define	BT_WEAPONSHIFT  3

#define BT_SPECIAL      128     // game events, not really buttons
#define	BTS_SAVEMASK    (4+8+16)
#define	BTS_SAVESHIFT   2
#define	BT_SPECIALMASK  3
#define	BTS_PAUSE       1       // pause the game
#define	BTS_SAVEGAME    2       // save the game at each console
// savegame slot numbers occupy the second byte of buttons

typedef enum
{
    ga_nothing,
    ga_loadlevel,
    ga_newgame,
    ga_loadgame,
    ga_savegame,
    ga_playdemo,
    ga_completed,
    ga_victory,
    ga_worlddone,
    ga_screenshot
} gameaction_t;

/*
===============================================================================

							MAPOBJ DATA

===============================================================================
*/

// think_t is a function pointer to a routine to handle an actor
typedef void (*think_t) ();

typedef struct thinker_s
{
    struct thinker_s *prev, *next;
    think_t function;
} thinker_t;

typedef union
{
    int i;
    struct mobj_s *m;
} specialval_t;

struct player_s;

typedef struct mobj_s
{
    thinker_t thinker;          // thinker links

// info for drawing
    fixed_t x, y, z;
    struct mobj_s *snext, *sprev;       // links in sector (if needed)
    angle_t angle;
    spritenum_t sprite;         // used to find patch_t and flip value
    int frame;                  // might be ord with FF_FULLBRIGHT

// interaction info
    struct mobj_s *bnext, *bprev;       // links in blocks (if needed)
    struct subsector_s *subsector;
    fixed_t floorz, ceilingz;   // closest together of contacted secs
    fixed_t dropoffz;           // killough 11/98: the lowest floor over all contacted Sectors.
    fixed_t radius, height;     // for movement checking
    fixed_t momx, momy, momz;   // momentums

    int validcount;             // if == validcount, already checked

    mobjtype_t type;
    mobjinfo_t *info;           // &mobjinfo[mobj->type]
    int tics;                   // state tic counter
    state_t *state;
    int damage;                 // For missiles
    int flags;
    int flags2;                 // Heretic flags
    int intflags;               // killough 9/15/98: internal flags
    specialval_t special1;      // Special info
    specialval_t special2;      // Special info
    int health;
    int movedir;                // 0-7
    int movecount;              // when 0, select a new dir
    struct mobj_s *target;      // thing being chased/attacked (or NULL)
    // also the originator for missiles
    int reactiontime;           // if non 0, don't attack yet
    // used by player to freeze a bit after
    // teleporting
    int threshold;              // if >0, the target will be chased
    short gear;                 // killough 11/98: used in torque simulation
    int geartics;               // [JN] Duration of torque sumulation.
    // no matter what (even if shot)
    struct player_s *player;    // only valid if type == MT_PLAYER
    int lastlook;               // player number last looked for

    mapthing_t spawnpoint;      // for nightmare respawn

    // [AM] If true, ok to interpolate this tic.
    boolean interp;

    // [AM] Previous position of mobj before think.
    //      Used to interpolate between positions.
    fixed_t oldx;
    fixed_t oldy;
    fixed_t oldz;
    angle_t oldangle;

} mobj_t;

// killough 11/98:
// For torque simulation:

void P_ApplyTorque(mobj_t *mo); // killough 9/12/98 
#define OVERDRIVE 6
#define MAXGEAR (OVERDRIVE+16)

// each sector has a degenmobj_t in it's center for sound origin purposes
typedef struct
{
    thinker_t thinker;          // not used for anything
    fixed_t x, y, z;
} degenmobj_t;

//
// frame flags
//
#define	FF_FULLBRIGHT	0x8000  // flag in thing->frame
#define FF_FRAMEMASK	0x7fff

// --- mobj.flags ---

#define MF_SPECIAL      1      // call P_SpecialThing when touched
#define MF_SOLID        2
#define MF_SHOOTABLE    4
#define MF_NOSECTOR     8      // don't use the sector links (invisible but touchable)
#define MF_NOBLOCKMAP   16     // don't use the blocklinks (inert but displayable)
#define MF_AMBUSH	    32
#define MF_JUSTHIT      64     // try to attack right back
#define MF_JUSTATTACKED 128    // take at least one step before attacking
#define MF_SPAWNCEILING 256    // hang from ceiling instead of floor
#define MF_NOGRAVITY    512    // don't apply gravity every tic

// movement flags
#define MF_DROPOFF  1024       // allow jumps from high places
#define MF_PICKUP   2048       // for players to pick up items
#define MF_NOCLIP   4096       // player cheat
#define MF_SLIDE    8192       // keep info about sliding along walls
#define MF_FLOAT    16384      // allow moves to any height, no gravity
#define MF_TELEPORT 32768      // don't cross lines or look at heights
#define MF_MISSILE  65536      // don't hit same species, explode on block
#define MF_DROPPED  131072     // dropped by a demon, not level spawned
#define MF_SHADOW   262144     // use translucent draw (shadow demons / invis)
#define MF_NOBLOOD  524288     // don't bleed when shot (use puff)
#define MF_CORPSE   1048576    // don't stop moving halfway off a step
#define MF_INFLOAT  2097152    // floating to a height for a move, don't auto float to target's height

#define MF_COUNTKILL 4194304   // count towards intermission kill total
#define MF_COUNTITEM 8388608   // count towards intermission item total

#define MF_SKULLFLY   16777216 // skull in flight
#define MF_NOTDMATCH  33554432 // don't spawn in death match (key cards)

#define MF_TRANSLATION    67108864      // if 0x4 0x8 or 0xc, use a translation
#define MF_TRANSSHIFT     26            // table for player colormaps
#define MF_EXTRATRANS     134217728     // [JN] Extra translucency
#define MF_COUNTEXTRAKILL 268435456     // [JN] Resurrected monster is counted by extra counter.

// --- mobj.flags2 ---

#define MF2_LOGRAV          0x00000001  // alternate gravity setting
#define MF2_WINDTHRUST      0x00000002  // gets pushed around by the wind specials
#define MF2_FLOORBOUNCE     0x00000004  // bounces off the floor
#define MF2_THRUGHOST       0x00000008  // missile will pass through ghosts
#define MF2_FLY             0x00000010  // fly mode is active
#define MF2_FOOTCLIP        0x00000020  // if feet are allowed to be clipped
#define MF2_SPAWNFLOAT      0x00000040  // spawn random float z
#define MF2_NOTELEPORT      0x00000080  // does not teleport
#define MF2_RIP             0x00000100  // missile rips through solid targets
#define MF2_PUSHABLE        0x00000200  // can be pushed by other moving mobjs
#define MF2_SLIDE           0x00000400  // slides against walls
#define MF2_ONMOBJ          0x00000800  // mobj is resting on top of another mobj
#define MF2_PASSMOBJ        0x00001000  // Enable z block checking.  If on,
                                        // this flag will allow the mobj to
                                        // pass over/under other mobjs.
#define MF2_CANNOTPUSH      0x00002000  // cannot push other pushable mobjs
#define MF2_FEETARECLIPPED  0x00004000  // a mobj's feet are now being cut
#define MF2_BOSS            0x00008000  // mobj is a major boss
#define MF2_FIREDAMAGE      0x00010000  // does fire damage
#define MF2_NODMGTHRUST     0x00020000  // does not thrust target when damaging
#define MF2_TELESTOMP       0x00040000  // mobj can stomp another
#define MF2_FLOATBOB        0x00080000  // use float bobbing z movement
#define MF2_DONTDRAW        0X00100000  // don't generate a vissprite
#define MF2_FOOTCLIP2       0x00400000  // [JN] If feet are allowed to be clipped for 3 pixels.
#define MF2_FEETARECLIPPED2 0x00800000  // [JN] A mobj's feet are now being cut for 3 pixels.

// [JN] killough 9/15/98: Same, but internal flags, not intended for .deh
// (some degree of opaqueness is good, to avoid compatibility woes)

enum {
    MIF_FALLING = 1,      // Object is falling
    MIF_ARMED = 2,        // Object is armed (for MF_TOUCHY objects)
    MIF_LINEDONE = 4,     // Object has activated W1 or S1 linedef via DEH frame
};

//==============================================================================

typedef enum
{
    PST_LIVE,                   // playing
    PST_DEAD,                   // dead on the ground
    PST_REBORN                  // ready to restart
} playerstate_t;

// psprites are scaled shapes directly on the view screen
// coordinates are given for a 320*200 view screen

typedef enum
{
    ps_weapon,
    ps_flash,
    NUMPSPRITES
} psprnum_t;

typedef struct
{
    state_t *state;             // a NULL state means not active
    int tics;
    fixed_t sx, sy;
} pspdef_t;

typedef enum
{
    key_yellow,
    key_green,
    key_blue,
    NUMKEYS
} keytype_t;

typedef enum
{
    wp_staff,
    wp_goldwand,
    wp_crossbow,
    wp_blaster,
    wp_skullrod,
    wp_phoenixrod,
    wp_mace,
    wp_gauntlets,
    wp_beak,
    NUMWEAPONS,
    wp_nochange
} weapontype_t;

#define AMMO_GWND_WIMPY 10
#define AMMO_GWND_HEFTY 50
#define AMMO_CBOW_WIMPY 5
#define AMMO_CBOW_HEFTY 20
#define AMMO_BLSR_WIMPY 10
#define AMMO_BLSR_HEFTY 25
#define AMMO_SKRD_WIMPY 20
#define AMMO_SKRD_HEFTY 100
#define AMMO_PHRD_WIMPY 1
#define AMMO_PHRD_HEFTY 10
#define AMMO_MACE_WIMPY 20
#define AMMO_MACE_HEFTY 100

typedef enum
{
    am_goldwand,
    am_crossbow,
    am_blaster,
    am_skullrod,
    am_phoenixrod,
    am_mace,
    NUMAMMO,
    am_noammo                   // staff, gauntlets
} ammotype_t;

typedef struct
{
    ammotype_t ammo;
    int upstate;
    int downstate;
    int readystate;
    int atkstate;
    int holdatkstate;
    int flashstate;
} weaponinfo_t;

typedef enum
{
    arti_none,
    arti_invulnerability,
    arti_invisibility,
    arti_health,
    arti_superhealth,
    arti_tomeofpower,
    arti_torch,
    arti_firebomb,
    arti_egg,
    arti_fly,
    arti_teleport,
    NUMARTIFACTS
} artitype_t;

typedef enum
{
    pw_None,
    pw_invulnerability,
    pw_invisibility,
    pw_allmap,
    pw_infrared,
    pw_weaponlevel2,
    pw_flight,
    pw_shield,
    pw_health2,
    NUMPOWERS
} powertype_t;

#define	INVULNTICS    (30*TICRATE)
#define	INVISTICS     (60*TICRATE)
#define	INFRATICS    (120*TICRATE)
#define	IRONTICS      (60*TICRATE)
#define WPNLEV2TICS   (40*TICRATE)
#define FLIGHTTICS    (60*TICRATE)
#define CHICKENTICS   (40*TICRATE)

#define BLINKTHRESHOLD (4*32)

#define NUMINVENTORYSLOTS 14

typedef struct
{
    int type;
    int count;
} inventory_t;

/*
================
=
= player_t
=
================
*/

#define CF_NOCLIP       1
#define CF_GODMODE      2
#define CF_NOMOMENTUM   4   // not really a cheat, just a debug aid

typedef struct player_s
{
    mobj_t *mo;
    playerstate_t playerstate;
    ticcmd_t cmd;

    fixed_t viewz;              // focal origin above r.z
    fixed_t viewheight;         // base height above floor for viewz
    fixed_t deltaviewheight;    // squat speed
    fixed_t bob;                // bounded/scaled total momentum

    int flyheight;
    int lookdir, oldlookdir;
    boolean centering;
    int health;                 // only used between levels, mo->health
    // is used during levels
    int armorpoints, armortype; // armor type is 0-2

    inventory_t inventory[NUMINVENTORYSLOTS];
    artitype_t readyArtifact;
    int artifactCount;
    int inventorySlotNum;
    int powers[NUMPOWERS];
    boolean keys[NUMKEYS];
    boolean backpack;
    signed int frags[MAXPLAYERS];       // kills of other players
    weapontype_t readyweapon;
    weapontype_t pendingweapon; // wp_nochange if not changing
    boolean weaponowned[NUMWEAPONS];
    int ammo[NUMAMMO];
    int maxammo[NUMAMMO];
    int attackdown, usedown;    // true if button down last tic
    int cheats;                 // bit flags

    int refire;                 // refired shots are less accurate

    int killcount, itemcount, secretcount;      // for intermission
    int extrakillcount;         // [JN] Resurrected monsters counter.
    char *message;              // hint messages
    int messageTics;            // counter for showing messages
    int yellowkeyTics;          // [JN] Counter for missing yellow key
    int greenkeyTics;           // [JN] Counter for missing green key
    int bluekeyTics;            // [JN] Counter for missing blue key
    MessageType_t messageType; // [JN] Colored message type
    int damagecount, bonuscount;        // for screen flashing
    int flamecount;             // for flame thrower duration
    mobj_t *attacker;           // who did damage (NULL for floors)
    int extralight;             // so gun flashes light up areas
    int fixedcolormap;          // can be set to REDCOLORMAP, etc
    int colormap;               // 0-3 for which color to draw player
    pspdef_t psprites[NUMPSPRITES];     // view sprites (gun, etc)
    boolean didsecret;          // true if secret level has been done
    int chickenTics;            // player is a chicken if > 0
    int chickenPeck;            // chicken peck countdown
    mobj_t *rain1;              // active rain maker 1
    mobj_t *rain2;              // active rain maker 2
    // [AM] Previous position of viewz before think.
    //      Used to interpolate between camera positions.
    angle_t oldviewz;
    // [crispy] squat down weapon sprite a bit after hitting the ground
    fixed_t	psp_dy, psp_dy_max;
} player_t;

/*
===============================================================================

					GLOBAL VARIABLES

===============================================================================
*/

#define TELEFOGHEIGHT (32*FRACUNIT)

extern boolean altpal;          // checkparm to use an alternate palette routine
extern boolean autostart;
extern boolean cdrom;           // true if cd-rom mode active ("-cdrom")
extern boolean deathmatch;      // only if started as net death
extern boolean DebugSound;      // debug flag for displaying sound info
extern boolean demoextend;      // allow demos to persist through exit/respawn
extern boolean demoplayback;
extern boolean demorecording;
extern boolean devparm;         // [JN] Game launched with -devparm
extern boolean lowres_turn;     // Used when recording Vanilla demos in netgames.
extern boolean netgame;         // only true if >1 player
extern boolean nodrawers;       // [crispy] for the demowarp feature
extern boolean nomonsters;      // checkparm of -nomonsters
extern boolean paused;
extern boolean playeringame[MAXPLAYERS];
extern boolean precache;        // if true, load all graphics at level load
extern boolean ravpic;          // checkparm of -ravpic
extern boolean realframe, skippsprinterp; // [JN] Interpolation for weapon bobbing
extern boolean respawnmonsters;
extern boolean respawnparm;     // checkparm of -respawn
extern boolean singledemo;      // quit after playing a demo from cmdline
extern boolean testcontrols;
extern boolean usergame;        // ok to save / end game

extern gameaction_t gameaction;
extern GameMode_t   gamemode;
extern gamestate_t  gamestate;

extern int artiskip;            // whether shift-enter skips an artifact
extern int bodyqueslot;
extern int consoleplayer;       // player taking events and displaying
extern int displayplayer;
extern int gameepisode;
extern int gamemap;
extern int GetWeaponAmmo[NUMWEAPONS];
extern int levelstarttic;       // gametic at level start
extern int mouseSensitivity;
extern int prevmap;
extern int show_messages;
extern int skytexture;
extern int startepisode;
extern int startmap;
extern int testcontrols_mousespeed;
extern int totalkills, totalitems, totalsecret; // for intermission
extern int totalleveltimes;     // [crispy] CPhipps - total time for all completed levels
extern int totaltimes;          // [crispy] CPhipps - total game time for completed levels so far
extern int viewangleoffset;     // ANG90 = left side, ANG270 = right

extern mapthing_t *deathmatch_p;
extern mapthing_t deathmatchstarts[10];
extern mapthing_t playerstarts[MAXPLAYERS];

extern player_t players[MAXPLAYERS];

extern skill_t gameskill;
extern skill_t startskill;

extern ticcmd_t *netcmds;

/*
================================================================================
=
= HR_MAIN
=
================================================================================
*/

extern void D_DoomMain (void);
extern void D_DoomLoop (void);

/*
================================================================================
=
= AM_MAP
=
================================================================================
*/

typedef struct
{
    int64_t x,y;
} mpoint_t;

extern mpoint_t *markpoints;
extern int       markpointnum, markpointnum_max;

extern boolean automapactive;
extern const boolean AM_Responder (const event_t *ev);

extern void AM_clearMarks (void);
extern void AM_Drawer (void);
extern void AM_initMarksColor (const int color);
extern void AM_initPics (void);
extern void AM_initVariables (void);
extern void AM_Start (void);
extern void AM_Stop (void);
extern void AM_Ticker (void);

/*
================================================================================
=
= CT_CHAT (Chat mode)
=
================================================================================
*/

extern char *chat_macros[10];
extern const char CT_dequeueChatChar (void);
extern const boolean CT_Responder (event_t *ev);
extern boolean chatmodeon;
extern boolean ultimatemsg;

extern void CT_Drawer (void);
extern void CT_Init (void);
extern void CT_Ticker (void);


/* 
================================================================================
=
= F_FINALE (Finale screens)
=
================================================================================
*/

extern const boolean F_Responder (const event_t *event);

extern void F_Drawer (void);
extern void F_StartFinale (void);
extern void F_Ticker (void);


/*
================================================================================
=
= G_GAME
=
================================================================================
*/

extern boolean G_Responder(event_t * ev);
extern boolean timingdemo;

extern void G_DeathMatchSpawnPlayer(int playernum);
extern void G_DeferedInitNew(skill_t skill, int episode, int map);
extern void G_DeferedPlayDemo(char *demo);
extern void G_DoLoadGame(void);
extern void G_DoSelectiveGame(int option);
extern void G_ExitLevel(void);
extern void G_InitNew(skill_t skill, int episode, int map, int fast_monsters);
extern void G_LoadGame(char *name);
extern void G_PlayDemo(char *name);
extern void G_PlayerReborn (int player);
extern void G_RecordDemo(skill_t skill, int numplayers, int episode, int map, char *name);
extern void G_SaveGame(int slot, char *description);
extern void G_ScreenShot(void);
extern void G_SecretExitLevel(void);
extern void G_Ticker(void);
extern void G_TimeDemo(char *name);
extern void G_WorldDone(void);

/*
================================================================================
=
= IN_LUDE (INTERLUDE)
=
================================================================================
*/

extern void IN_Start (void);
extern void IN_Ticker (void);
extern void IN_Drawer (void);

/* 
================================================================================
=
= M_RANDOM
=
================================================================================
*/

// Most damage defined using HITDICE
#define HITDICE(a) ((1+(P_Random()&7))*a)

extern int rndindex;
extern const int M_Random (void);
extern const int P_Random (void);
extern const int P_SubRandom (void);

extern void M_ClearRandom (void);

/* 
================================================================================
=
= MN_MENU
=
================================================================================
*/

extern boolean MN_Responder (event_t *event);
extern boolean askforquit;

extern byte *ammo_widget_opacity_set;

extern void MN_Drawer (void);
extern void MN_Init (void);
extern void MN_Ticker (void);

/*
================================================================================
=
= SB_BAR (STATUS BAR)
=
================================================================================
*/

#define	SBARHEIGHT	    (42 << hires)  // status bar height at bottom of screen

#define STARTREDPALS	1
#define STARTBONUSPALS	9
#define NUMREDPALS		8
#define NUMBONUSPALS	4

extern boolean SB_Responder(event_t *event);

extern int ArtifactFlash;
extern int defdemotics, deftotaldemotics;
extern int SB_state;

extern byte    *CrosshairOpacity;
extern patch_t *CrosshairPatch;

extern int  CrosshairShowcaseTimeout;
extern void Crosshair_Colorize_inMenu (void);
extern void Crosshair_DefineDrawingFunc (void);
extern void Crosshair_DefineOpacity (void);
extern void Crosshair_DefinePatch (void);
extern void Crosshair_Draw (void);

extern void SB_DemoProgressBar (void);
extern void SB_DrawDemoTimer (const int time);
extern void SB_Drawer (void);
extern void SB_Init (void);
extern void SB_Ticker (void);

#include "sounds.h"
