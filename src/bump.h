// -*- C++ -*-

#ifndef bump_h
#define bump_h

//  r ^                        r = bump (t)
//    |
// v1 +--------------XXXXXX----------------
//    |           X  |    |  X
//    |         X    |    |    X
//    |        X     |    |     X
//    |      X       |    |       X
// v0 +XXXX----------+----+----------XXXX-->
//        t0        t1    t2        t3      t

struct bump_t {
  float v0, v1, t0, t1, t2, t3;

  float operator () (float t) const {
    float r;
    if (t < t0 || t3 < t) {
      r = 0;
    }
    else if (t1 < t && t < t2) {
      r = 1;
    }
    else {
      float s;
      if (t < t2) {
        s = (t - t0) / (t1 - t0);
      }
      else {
        s = (t3 - t) / (t3 - t2);
      }
      r = s * s * (3 - 2 * s);
    }
    return v0 + r * (v1 - v0);
  }
};

#endif
