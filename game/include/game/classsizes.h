/* The sizes of the classes that C sources allocate themselves (`__nw__6CClassUi(SIZEOF_SPRITE)`
   and then the constructor), for the PS1 and for a 64-bit build, where their pointers are 8 bytes.
   Plain C; each class's header checks its size against the one here. */
#ifndef GAME_CLASSSIZES_H
#define GAME_CLASSSIZES_H

#if __SIZEOF_POINTER__ == 8
#define SIZEOF_CBRUCE       3992
#define SIZEOF_CLIGHT       120
#define SIZEOF_CPOWERUP     392
#define SIZEOF_CRAIL        64
#define SIZEOF_CTRICKOB     40
#define SIZEOF_PREMANAGER   392
#define SIZEOF_PADBUFFER    288
#define SIZEOF_SLICEDIMAGE  64
#define SIZEOF_SPRITE       32
#define SIZEOF_SVIDEOINFO   216
#define SIZEOF_VERTICALMENU 944
#else
#define SIZEOF_CBRUCE       3612
#define SIZEOF_CLIGHT       100
#define SIZEOF_CPOWERUP     356
#define SIZEOF_CRAIL        44
#define SIZEOF_CTRICKOB     24
#define SIZEOF_PREMANAGER   324
#define SIZEOF_PADBUFFER    284
#define SIZEOF_SLICEDIMAGE  52
#define SIZEOF_SPRITE       24
#define SIZEOF_SVIDEOINFO   192
#define SIZEOF_VERTICALMENU 820
#endif

/* in a class's header, after the class */
#define CHECK_CLASS_SIZE(cls, size) typedef char cls##_is_##size[sizeof(cls) == (size) ? 1 : -1]

#endif
