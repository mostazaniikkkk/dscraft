/* host stub */
#ifndef TEST_MAXMOD
#define TEST_MAXMOD
extern int sfxPlayed;
static inline void mmEffect(int s){ (void)s; sfxPlayed++; }
#endif
