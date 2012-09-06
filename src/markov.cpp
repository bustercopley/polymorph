#include "markov.h"
#include "random.h"

unsigned markov (rng_t & rng, unsigned last, unsigned last_but_one) {
  unsigned result;
  do result = rng.get () & 7; /* PLEASE */

  // Certain transitions are not allowed:
  while (result == last_but_one || 1 & last ["\001\002\004\270\270\270\300\370"] >> result);
  return result;
}
