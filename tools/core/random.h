/* random.h - random(), random(min, max) and randomSeed() of an Arduino core,
 * the same numbers on every target: a linear congruential generator (the
 * constants of Numerical Recipes). Included by the stand-in cores. The C
 * library has a random(void) of its own: the Arduino forms are C++ (core.h)
 * and call these. */
static unsigned long core_random_state = 1;

void randomSeed(unsigned long seed)
{
  if (seed != 0)
    core_random_state = seed;
}

static unsigned long core_random_next(void)
{
  core_random_state = core_random_state * 1664525UL + 1013904223UL;
  return (core_random_state >> 1) & 0x7FFFFFFFUL;
}

long core_random(long howbig)
{
  if (howbig <= 0)
    return 0;
  return (long)(core_random_next() % (unsigned long)howbig);
}

long core_random_range(long howsmall, long howbig)
{
  if (howsmall >= howbig)
    return howsmall;
  return core_random(howbig - howsmall) + howsmall;
}
