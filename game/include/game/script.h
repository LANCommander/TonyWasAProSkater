/* The level trigger file's two bytecodes (pre-QScript), with Neversoft's names.

   The names are the THPS2 source's #defines, recovered from a SlickEdit tag database of its tree
   (sk2.vtg, in github.com/krystalgamer/spidey-decomp thps2-stuff); THPS1 uses the same values. The
   C_ and V_ names are verbatim; the trigger commands, node and object types were unprefixed
   defines (SENDPULSE, BADDY, PLATFORM, ...) and get TRG_/NODE_/OBJ_ here. Where THPS1's handler
   differs from what the THPS2 name says, the comment gives what THPS1 does. MAIN.SYM (the demo)
   has none of them: they were macros.

   Both are streams of 16-bit words in the .TRG file (trig.cpp; tools/trg_dump.py disassembles one
   and reads the names below, so keep them one `NAME = 0x...,` per line):

   - object scripts, run by CBaddy::ParseScript for a NODE_BADDY's script-only baddy or platform: a
     word with BASE_COMMAND set is a command (CBaddy/CPlatform/CScriptOnlyBaddy::ExecuteCommand),
     one with BASE_VARIABLE set (and not BASE_COMMAND) sets a variable (SetVariable); any other word
     is skipped. Where an operand may be a variable, a word with BASE_VARIABLE set names one (read
     through GetVariable, which can consume operands of its own); GetScriptValue operands also
     treat a word with 0x8000 set as a literal. The script ends at C_DONE.
   - trigger command lists, run by ExecuteCommandList for a NODE_COMMANDPOINT, a NODE_RESTART and
     the NODE_AUTOEXEC nodes (NODE_AUTOEXEC2 in a two-player game); they end at TRG_ENDCOMMANDLIST.

   Operands are words unless noted; "aligned" ones are 32-bit, at the next 4-byte boundary, and
   strings are NUL-terminated and padded to a word boundary (SkipString). */
#ifndef GAME_SCRIPT_H
#define GAME_SCRIPT_H

#define BASE_COMMAND 0x4000
#define BASE_VARIABLE 0x2000

/* trigger file node types (the first word of each node; OffsetList); TRIGGERFILEVERSION 2 */
enum ETrigNodeType {
	NODE_BADDY = 1,             /* object type, seed, links, flags, pos, angles, script */
	NODE_CRATE = 2,             /* links, checksum; THPS1: a trigger running the empty list */
	NODE_POINT = 3,             /* links, pos */
	NODE_AUTOEXEC = 4,          /* command list, run when the level loads */
	NODE_POWRUP = 5,            /* powerup type, links, pos, ... */
	NODE_COMMANDPOINT = 6,      /* links, checksum, command list */
	NODE_SEEDABLEBADDY = 7,     /* as NODE_BADDY, created inactive */
	NODE_RESTART = 8,           /* links, pos, angles, name, command list */
	NODE_BARREL = 9,            /* as NODE_CRATE */
	NODE_RAILPOINT = 10,        /* links, pos */
	NODE_RAILDEF = 11,          /* links, pos */
	NODE_TRICKOB = 12,          /* links (CTrickOb) */
	NODE_CAMPT = 13,            /* links, pos (CamPt_New) */
	NODE_GOALOB = 14,           /* links (CTrickOb) */
	NODE_AUTOEXEC2 = 15,        /* command list, run instead of NODE_AUTOEXEC in two-player */
	NODE_TERMINATOR = 255,      /* the last node */
	NODE_LIGHT = 500,           /* pos, light params; created at load */
	NODE_OFFLIGHT = 501,        /* pos, light params; created by a pulse */
	NODE_SCRIPTPOINT = 1000,    /* links, pos */
	NODE_CAMERAPATH = 1001,     /* links, pos */
};

/* a NODE_BADDY's flag bytes (GetFlag; the list ends at 0xff) */
enum ETrigCreateFlag {
	CREATEDATSTART = 1,         /* created when the level loads (else by a pulse) */
	ACTIVEWHENCREATED = 2,      /* not suspended when created */
	SUSPENDEDWHENCREATED = 3,
	MANUALSUSPENDCONTROL = 4,   /* clears mCBodyFlags 2 */
	RADIALSUSPENDCONTROL = 5,
};

/* NODE_BADDY object types (Trig_CreateObject) */
enum ETrigObjectType {
	OBJ_SCRIPTONLYBADDY = 203,  /* CScriptOnlyBaddy */
	OBJ_CAR_TAXI = 213,         /* CCar (no script) */
	OBJ_CAR_POLICE = 214,
	OBJ_CAR_BUS = 215,          /* THPS1: the van, "c_van" */
	OBJ_CAR_CABLE = 216,
	OBJ_PLATFORM = 402,         /* CPlatform */
};

/* object script commands (BASE_COMMAND set) */
enum EScriptCommand {
	/* flow (CBaddy) */
	C_DONE = 0x4100,            /* end of script; the parse stops (and mDoScript clears) here */
	C_GOTO = 0x4101,            /* label */
	C_GOTO_BREAK = 0x4102,      /* label; jumps there and stops for this frame */
	C_LABEL = 0x4104,           /* label; records the position after it */
	C_READ_LABELS = 0x4105,     /* records every C_LABEL from here to C_DONE (for forward gotos) */
	C_SET_SCRIPT_NODE = 0x4106, /* node (var); continues in another NODE_BADDY's script */
	C_BREAK = 0x4107,           /* stops for this frame, carrying on after it on the next */
	C_ADD = 0x4110,             /* variable (and its operands), value (var); variable += value */
	C_SUB = 0x4111,             /* as C_ADD, variable -= value */
	C_IF_GT = 0x4112,           /* a (var), b (var); if a > b ... C_ENDIF */
	C_IF_LT = 0x4113,           /* a (var), b (var) */
	C_IF_EQ = 0x4114,           /* a (var), b (var) */
	C_IF_FLAG_SET = 0x4115,     /* flags; if every one is set in mSpecificFlags */
	C_IF_FLAG_CLEAR = 0x4116,   /* flags; if none is set */
	C_ENDIF = 0x4120,           /* (SkipToEndif's target; a no-op when run) */
	C_4180 = 0x4180,            /* (no THPS2 name) no operands, does nothing */

	/* the object (CBaddy, and CPlatform's moves) */
	C_INIT_ITEM = 0x4200,       /* model name (string) */
	C_CYCLE_ANIM = 0x4201,      /* anim, direction */
	C_RUN_ANIM = 0x4202,        /* anim */
	C_DISPLAY_ON = 0x4203,
	C_DISPLAY_OFF = 0x4204,
	C_DIE_QUIETLY = 0x4205,     /* (a platform also stops its script for this frame) */
	C_MOVE_TO_ABSOLUTE = 0x4220, /* x, y, z (aligned); CPlatform */
	C_MOVE_TO_NODE = 0x4221,    /* node (var); CPlatform */
	C_MOVE_TO_RELATIVE = 0x4222, /* dx, dy, dz (aligned); CPlatform */
	C_ZERO_VELOCITY = 0x4226,
	C_FOLLOW_COMPLEX_SURFACE = 0x4227, /* interval; 0 turns GLOBAL_FLAG_FOLLOW_COMPLEX_SURFACE off */
	C_DO_PATTERN = 0x4240,      /* THPS1: StopScript, and stops for this frame */
	C_WAIT = 0x4280,            /* frames (var); stops, resuming after that many frames */
	C_WAIT_FOR_SIGNAL = 0x4281, /* stops each frame until the object is signalled */

	/* effects, sound and the world (CBaddy) */
	C_PLAY_SFX = 0x4290,        /* sound */
	C_PLAY_POSITIONAL_SFX = 0x4291, /* sound; at the object */
	C_SPARK = 0x4292,           /* count; a cone of sparks along the object's angles */
	C_SET_FMV_CHECKSUM = 0x4293, /* texture checksum (aligned); the in-level TV's texture */
	C_START_FMV = 0x4294,       /* the TV video, sound from the object and its links */
	C_STOP_FMV = 0x4295,
	C_MIDI_FADE_IN = 0x4296,    /* rate */
	C_MIDI_FADE_OUT = 0x4297,   /* rate */
	C_SHAKE_CAMERA = 0x4298,    /* size (var): 0 small, 1 medium, else big; from the object */
	C_SET_FMV_TRACK = 0x4299,   /* track */
	C_EXPLODE_NODE = 0x429a,    /* node (var), kind, (kind 2: value, 2 words); a stub in THPS1 */
	C_SMOKEJET_ON = 0x429b,     /* 6 words; CScriptOnlyBaddy; protected */
	C_FLASH_SCREEN = 0x429c,    /* 5 words; Flash_Screen */
	C_START_FRAMEBUFFER_FMV = 0x429d, /* sound from the object, starts copying the frame buffer */
	C_SET_WATER_LEVEL = 0x429e, /* to the object's height (its creator's, in two-player) */
	C_SMOKEJET_OFF = 0x429f,    /* CScriptOnlyBaddy */
	C_MAKE_POWERUP = 0x42a0,    /* type (var), a, b, gravity; at the object */
	C_FIREJET_ON = 0x42a1,      /* 6 words; CScriptOnlyBaddy: C_SMOKEJET_ON's jet, another texture */
	C_SET_WATER_DAMAGE = 0x42a2, /* damage */
	C_PLAY_XA = 0x42a3,         /* THPS1: no operands, does nothing */
	C_EXPLODE = 0x42a6,         /* kind, ...; C_EXPLODE_NODE at the object (a stub) */
	C_TEXT_MESSAGE = 0x42b0,    /* string; THPS1 skips it */
	C_SEND_SIGNAL_TO_LINKS = 0x42b1, /* node (var) */
	C_SEND_PULSE_TO_LINKS = 0x42b2, /* node (var) */
	C_SEND_SIGNAL_TO_NODE = 0x42b3, /* node (var); the baddy/object there */
	C_SEND_PULSE_TO_NODE = 0x42b4, /* node (var) */
	C_GOALCOUNTER = 0x42c0,     /* goals; the panel's goal counter, in a career game */

	/* CPlatform (C_WAIT_UNTIL_STOOD_ON is a no-op on other objects) */
	C_WAIT_UNTIL_STOOD_ON = 0x4300, /* stops each frame until a skater stands on it */
	C_SHATTER = 0x4301,         /* dies and shatters, stops for this frame */
	C_BOUNDING_BOX_CHECK_ON = 0x4302, /* x, y, z; THPS1: sets mSpin and mSpecificFlags 4 */
	C_DAMAGE_BRUCE = 0x4303,    /* amount (mBruceHealthAdjust) */
	C_SCALE = 0x4304,           /* percent, frames; all three axes */
	C_WATER_CLIP = 0x4305,      /* on; the Item flag 0x8 (also V_ALLOW_WATER_CLIP) */
	C_SCALE_X = 0x4306,         /* percent, frames */
	C_SCALE_Y = 0x4307,
	C_SCALE_Z = 0x4308,
	C_IS_BOUNCY = 0x4309,       /* bounce type (mType); 0 makes it a normal platform again */

	/* CScriptOnlyBaddy (the looping sounds also CPlatform) */
	C_MAKE_RAIN = 0x4503,       /* droplets (GetScriptValue); falling around the skater */
	C_MAKE_ROCK = 0x4504,       /* THPS1: 4 ignored words, then C_SMOKEJET_ON's 6 */
	C_SNAP_TO_BRUCE = 0x4506,   /* moves to the skater */
	C_PLAY_LOOPING_SFX = 0x4507, /* sound; replaces the looping sound */
	C_PLAY_LOOPING_POSITIONAL_SFX = 0x4508, /* sound, update interval */
	C_STOP_LOOPING_SFX = 0x4509,
	C_BUBBLES = 0x4700,         /* on; mSpecificFlags 0x100 */
};

/* mSpecificFlags bits named in THPS2 (others: 4 C_BOUNDING_BOX_CHECK_ON, 0x10 C_IS_BOUNCY,
   0x100 C_BUBBLES) */
#define GLOBAL_FLAG_HAS_TARGET_POSITION 0x01
#define GLOBAL_FLAG_FOLLOW_COMPLEX_SURFACE 0x02

/* object script variables (BASE_VARIABLE set): set as script statements by SetVariable (operands
   as noted) and read as operands by GetVariable (no operands unless noted) */
enum EScriptVariable {
	V_HEALTH = 0x2100,          /* set: value */
	V_COLLISION_RADIUS = 0x2101, /* set only: value (mRMinor) */
	V_PATTERN = 0x2114,         /* set only: value */
	V_SUBPATTERN = 0x2115,      /* set only: value */
	V_REGISTER = 0x2120,        /* register (low byte); set: + value (GetScriptValue) */
	V_ANIM_SPEED = 0x2121,      /* set only: value */
	V_GROUND_TO_ORIGIN = 0x2122, /* set only: value */
	V_APPLY_GRAVITY = 0x2123,   /* CPlatform set only: on (low byte) */
	V_MODEL = 0x2124,           /* CPlatform set only: model index */
	V_ATTRIBUTE = 0x2125,       /* set only: index, value (mBaddyAttributes) */
	V_ANGULAR_VELOCITY = 0x2127, /* CPlatform set only: x, y, z (GetScriptValue) */
	V_ANGULAR_ACCELERATION = 0x2128, /* CPlatform set only: x, y, z (GetScriptValue) */
	V_RANDOM = 0x2129,          /* get only: range; Rnd(range) */
	V_MY_NODE = 0x212a,         /* get only */
	V_LINKED_NODE = 0x212b,     /* get only: node (var); its first link, or 0 */
	V_INPUT_SIGNAL = 0x212c,    /* times signalled; set: value (low byte) */
	V_INPUT_HIT = 0x212d,       /* set: value (low byte) */
	V_VERTICAL_COMPLIANCE = 0x212e, /* set: value */
	V_MODEL_CHECKSUM = 0x212f,  /* set only: checksum (aligned); also CPlatform */
	V_ALLOW_WATER_CLIP = 0x2131, /* set only: on; the Item flag 0x8 */
	V_BRUCE_XZ_DIST = 0x2132,   /* get only: XZ distance to the skater, at most 0x1fff */
	V_BRUCE_LINEOFSIGHT = 0x2133, /* get only: from the object's eye to the skater */
	V_VELOCITY = 0x2134,        /* CPlatform set only: x, y, z (GetScriptValue) */
	V_DEAD_CHECKSUM = 0x2135,   /* set only: checksum (aligned) */
	V_VIDEO_FRAME = 0x2136,     /* get only: the TV video's current frame */
	V_ANGLES = 0x2137,          /* CPlatform set only: x, y, z (GetScriptValue) */
	V_MY_X_POS = 0x2140,        /* set: value (GetScriptValue) */
	V_MY_Y_POS = 0x2141,
	V_MY_Z_POS = 0x2142,
	V_BRUCE_X_POS = 0x2150,     /* get only */
	V_BRUCE_Y_POS = 0x2151,
	V_BRUCE_Z_POS = 0x2152,
	V_NOT_TROD_UPON_TIMER = 0x2200, /* CPlatform get only */
};

/* trigger command list commands (ExecuteCommandList; SkipCommand skips one, but has no case for
   some: 0x96, 0x97, 0x9d, 0x9f, 0xa1 and 0xa2 skip as if they had no operands) */
enum ETrigCommand {
	TRG_SETCHEATRESTARTS = 0x02, /* strings, ended by an empty one; the cheat menu's restarts */
	TRG_SENDPULSE = 0x03,       /* to the links (limited by TRG_SETINITIALPULSES) */
	TRG_SENDACTIVATE = 0x04,    /* unsuspends the linked objects */
	TRG_SENDSUSPEND = 0x05,     /* suspends the linked objects */
	TRG_SENDSIGNAL = 0x0a,      /* to the links */
	TRG_SENDKILL = 0x0b,        /* to the links */
	TRG_SENDKILLLOUDLY = 0x0c,  /* to the links; THPS1: hits them (KillInList mode 1) */
	TRG_SENDVISIBLE = 0x0d,     /* VISIBLE (1) or INVISIBLE (0); to the links */
	TRG_WATEREFFECTON = 0x66,   /* DAT_800d1f14 = 1 */
	TRG_WATEREFFECTOFF = 0x67,  /* DAT_800d1f14 = 0 */
	TRG_SETFOGGINGPARAMS = 0x68, /* hither, dpq min, range */
	TRG_PLAYSOUND = 0x69,       /* sound */
	TRG_STOPSOUND = 0x6a,       /* sound */
	TRG_DEBUGTEXT = 0x77,       /* string; skipped */
	TRG_SPOOLIN = 0x7e,         /* file name; Spool_PSX */
	TRG_SPOOLOUT = 0x7f,        /* file name */
	TRG_SPOOLENV = 0x80,        /* file name; Spool_PSX mode 1 */
	TRG_SPOOLLOCK = 0x81,       /* Spool_Sync */
	TRG_SETCAMANGLE = 0x82,     /* angle, rate */
	TRG_BACKGROUNDON = 0x83,    /* background */
	TRG_BACKGROUNDOFF = 0x84,   /* background */
	TRG_KILLEVERYTHINGINBOX = 0x85, /* boxes (aligned min/max xyz), ended by a 0xff word */
	TRG_SETINITIALPULSES = 0x86, /* count; once per command point (0xffff: unlimited) */
	TRG_SETCAMDISTXZ = 0x87,    /* distance, rate */
	TRG_ALLOWXA = 0x88,
	TRG_DISALLOWXA = 0x89,
	TRG_SEEKXA = 0x8a,          /* 1 word; THPS1 ignores it */
	TRG_PLAYXA = 0x8b,          /* 2 words; THPS1 ignores it */
	TRG_SETRESTART = 0x8c,      /* restart name; a new one clears the messages and saves */
	TRG_SETVISIBILITYINBOX = 0x8d, /* VISIBLE/INVISIBLE, INSIDEBOX/OUTSIDEBOX, then boxes */
	TRG_SETOBJFILE = 0x8e,      /* file name; the platforms' models */
	TRG_SETCAMDISTY = 0x8f,     /* distance, rate */
	TRG_SETCAMOFFSETX = 0x90,   /* offset, rate */
	TRG_SETCAMOFFSETY = 0x91,
	TRG_SETCAMOFFSETZ = 0x92,
	TRG_SETGAMELEVEL = 0x93,    /* level */
	TRG_IFPULSECOUNT = 0x94,    /* count; skips to TRG_ENDIF unless this point has had that many */
	TRG_ENDIF = 0x95,
	TRG_MIDICHANNELMUTE = 0x96, /* channels */
	TRG_SETDUALBUFFERSIZE = 0x97, /* kilobytes (a stub in THPS1) */
	TRG_KILLBRUCE = 0x98,       /* puts the skater at the linked node, or the nearest 'H' restart,
	                               or restarts */
	TRG_SETCAMCOLIJSIDE = 0x99, /* value */
	TRG_SETCAMCOLIJBACK = 0x9a,
	TRG_MIDIFADEIN = 0x9b,      /* rate */
	TRG_MIDIFADEOUT = 0x9c,     /* rate */
	TRG_SETREVERBTYPE = 0x9d,   /* type */
	TRG_ENDLEVEL = 0x9e,        /* Game_Over */
	TRG_SPOOLMIDI = 0x9f,       /* file name */
	TRG_SETCAMMODE = 0xa0,      /* mode */
	TRG_SETCAMCOMPLEXMODE = 0xa1, /* mode */
	TRG_LOADAI = 0xa2,          /* string; THPS1 ignores it */
	TRG_IGNOREBRUCEINPUT = 0xa3, /* frames */
	TRG_SETCAMCOLIJANGLESIDE = 0xa4, /* angle */
	TRG_SETCAMCOLIJANGLEBACK = 0xa5,
	TRG_SETOTPUSHBACK = 0xa6,   /* value */
	TRG_SETCAMZOOM = 0xa7,      /* zoom, rate */
	TRG_SETCAMPITCHDAMP = 0xa8, /* value */
	TRG_SETOTPUSHBACK2 = 0xa9,  /* value */
	TRG_SETSUSPENDDISTANCE = 0xaa, /* distance */
	TRG_BACKGROUNDCREATE = 0xab, /* checksum (aligned), angular velocity x, y, z */
	TRG_SETCAMYDAMP = 0xac,     /* value */
	TRG_SETCAMFOCUSEQUALSTRIPOD = 0xad,
	TRG_SETDROPDAMAGEON = 0xae, /* a, b (CBruce::m8A4) */
	TRG_SETDROPDAMAGEOFF = 0xaf,
	TRG_SETRESTARTNOMESSAGE = 0xb0, /* restart name; TRG_SETRESTART without clearing messages */
	TRG_IGNOREBRUCEINPUTFREEZE = 0xb1, /* frames */
	TRG_SETRESTART2 = 0xb2,     /* restart name; only in a two-player game */
	TRG_SETFADECOLOR = 0xc8,    /* high, low */
	TRG_GAPPOLYHIT = 0xc9,      /* checksum (aligned), gap type */
	TRG_SETSKYCOLOR = 0xca,     /* high, low */
	TRG_ENDCOMMANDLIST = 0xffff,
};

#endif
