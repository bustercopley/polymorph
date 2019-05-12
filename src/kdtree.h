// -*- C++ -*-

// Copyright 2012-2019 Richard Copley
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef kdtree_h
#define kdtree_h

#include "mswin.h"

#include "bounce.h"
#include "compiler.h"
#include "memory.h"
#include "partition.h"
#include "vector.h"
#include <cstdint>

//#define ENABLE_RANDOMIZE_COLLISION_ORDER

#ifdef ENABLE_RANDOMIZE_COLLISION_ORDER
#define RANDOMIZE_COLLISION_ORDER_ENABLED 1
#else
#define RANDOMIZE_COLLISION_ORDER_ENABLED 0
#endif

// Simplified kd-tree of fixed dimension 3,
// using a constant-depth implicit binary tree.

// The build phase is pretty much the classical algorithm, making use
// of the partition routine from quicksort. There are two search phases.
// First, for each point P in the tree, traverse the kd-tree discarding
// nodes not intersecting the bounding box of the sphere centre P, radius
// 2*max_radius. Second, for each of the wall planes, traverse the kd-tree
// discarding nodes whose minimum directed distance from the wall is
// greater than max_radius.

// For simplicity and efficiency, parameters needed for the bounce calculation
// are passed to the kd-tree search function and forwarded directly, without the
// usual layer of abstraction.

// Implicit binary tree.

// Example (depth K = 3).
// Level 0:     |                   0                   |
// Level 1:     |         1         |         2         |
// Level 2:     |    3    |    4    |    5    |    6    |
// Level 3:     |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 |

// Tree structure
//   Node 0 is the root node.
//   Node m's children are nodes 2m+1 and 2m+2.
// Levels
//   There are K+1 levels, numbered 0 to K; nodes on level K are leaf nodes.
//   Each level k contains the 2^k nodes in the range [2^k-1, 2^(k+1)-1).
//   Therefore the total number of non-leaf nodes is 2^K-1.
//   Node m is in level k = logb(m+2)-1 at position i = m+1-2^k.
//   If k<K, node m has two child nodes, in level k+1 at positions 2i and 2i+1.
// Points
//   There are N points, numbered 0, 1, 2, ..., N-1.
//   The points are distributed evenly among the nodes of each level; for each
//   m, the node m, which is in level k at position i (see above), contains the
//   points [iN/2^k, (i+1)N/2^k).
//   The "middle point" of a non-leaf node m (in level k at position i) is
//   defined to be the first point of its second child (node 2m+2, in level k+1
//   at position 2i+1), i.e., point (2i+1)N/2^(k+1).
// Leaf nodes
//   The bottom level, level K, contains 2^K nodes (the leaf nodes). The depth K
//   is chosen, if possible, so that for some real constant M >= 1 we have M <
//   N/(2^K) <= 2M. Such a K exists if N > M; otherwise use depth K = 0. Then,
//   each node contains either floor(N/(2^K)) or ceil(N/(2^K)) points, and at
//   least one node contains ceil(N/(2^K)) nodes.
// KD tree
//   The data for the KD tree consists of a permutation of [0, N), represented
//   as an array of N unsigned integers (kdtree_index), and a co-ordinate value
//   for each non-leaf node, represented as an array of (2^K)-1 floating-point
//   numbers (kdtree_split).
//   The points [begin, end) of each non-leaf node m (in level k at position i)
//   are partitioned about the node's middle point mid in dimension d = k % 3,
//   in the sense that
//   x [kdtree_index[n]] [d] <= kdtree_split[m] for n in [begin, mid),
//   x [kdtree_index[n]] [d] >= kdtree_split[m] for n in [mid, end).

const std::uint8_t inc_mod3 [] = { 1, 2, 0, 1, };

// See "Leaf nodes" above.
inline unsigned required_depth (unsigned point_count)
{
  const unsigned min_points_per_leaf = 3;
  unsigned desired_leaf_count = point_count / min_points_per_leaf;

  // BSR(0) is undefined!
  if (! desired_leaf_count) {
    return 0;
  }

  // Let N = point_count, M = min_points_per_leaf (assumed positive integers).
  // Let r = desired_leaf_count = floor(N/M).
  // N, M, r are positive integers (we have already returned if r is zero).
  // We have   r <= N/M < r+1        (by definition of "floor")
  // so    (*) M <= N/r < M + M/r    (multiply each term by M/r)

  // Now let K = floor(log_2(r)) and let R = 2**K = 1 << K.
  // (K will be the tree depth and R the actual leaf count.)
  // We have   K <= log_2(r) < K+1   (by definition of "floor")
  // so        R < r <= 2R           (raise 2 to the power of each term)
  // and   (*) N/2R <= N/r < N/R     (divide N by each term and reverse)

  // Combine the two starred inequalities to get M < N/R < 2M + 2M/r.

  // Hence, M <= floor(N/R), and ceil(N/R) <= 2M + ceil(2M/r).
  // Each actual leaf node will contain either floor(N/R) or ceil(N/R) points,
  // so if a given leaf node contains m points then
  //   M <= m < 2M + ceil(2M/r).

  // If N is sufficiently large (N >= 4M^2) then r >= 2M and so we have:
  //   M <= m <= 2M.

  return _bit_scan_reverse (desired_leaf_count); // result K = floor(log_2(r)).
}

ALWAYS_INLINE
inline void model_t::kdtree_search ()
{
  // Reallocate memory for the nodes.
  unsigned depth = required_depth (count);
  unsigned nonleaf_count = (1 << depth) - 1;
  if (nonleaf_count > kdtree_capacity) {
    deallocate (kdtree_split);
    kdtree_capacity = nonleaf_count;
    kdtree_split = (float *) allocate (kdtree_capacity * sizeof (float));
  }

  // Phase 1: build the tree.
  unsigned node = 0;
  std::uint8_t dim = 0;
  for (unsigned level = 0; level != depth; ++ level) {
    unsigned begin = 0;
    unsigned level_node_count = 1 << level;
    for (unsigned i = 0; i != level_node_count; ++ i) {
      std::uint64_t end = ((std::uint64_t) (i + 1) * count) >> level;
      std::uint64_t mid = ((std::uint64_t) (2 * i + 1) * count) >> (level + 1);
      partition (kdtree_index, x, dim, begin, mid, end);
      kdtree_split [node ++] = x [kdtree_index [mid]] [dim];
      begin = end;
    }
    dim = inc_mod3 [dim];
  }

  // Enough stack to traverse a tree with more than 2^32 nodes.
  unsigned stack [32];                   // Node index.
  ALIGNED16 float stack_corner [32] [4]; // Extra space for wall phase.

  // Phase 2: for each object, detect collisions with other objects.

  // The later a collision is processed, the greater its tendency to
  // increase the separation of the two objects involved, because
  // there are fewer chances for the effects of the collision to be
  // undone by subsequent collisions.

  // If we iterate over the objects in the permuted order specified by
  // kdtree_index, that tendency will be strongly correlated with
  // position, and the gas will be more compressible in some regions
  // of space than others.

  // To avoid this undesirable anisotropy, choose a different
  // iteration order by constructing another permutation, kdtree_aux.

  if constexpr (RANDOMIZE_COLLISION_ORDER_ENABLED) {
    // To avoid any unfairness, construct a random permutation using
    // the inside-out Fisher-Yates shuffle.
    unsigned bits = _bit_scan_reverse (count) + 1;
    for (unsigned n = 0; n != count; ++ n) {
      // Get a random integer i, uniformly distributed on [0, n + 1).
      unsigned i = ((rng.get () >> bits) * (n + 1)) >> (64 - bits);
      kdtree_aux [n] = kdtree_aux [i];
      kdtree_aux [i] = n;
    }
  }
  else {
    // Iterate over the objects in array order, by computing the
    // inverse permutation of kdtree_index. This entails that
    // red/orange objects are more permeable than blue objects.
    for (unsigned n = 0; n != count; ++ n) {
      n [kdtree_index] [kdtree_aux] = n;
    }
  }

  // For every pair of integers i, n such that 0 <= i' < n' < count,
  // where i' = kdtree[i] and n' = kdtree[n], if |x[i] - x[n]| < 2R
  // then call bounce(i, n). Loop over n', not n, to avoid anisotropy.
  for (unsigned n1 = 0; n1 != count; ++ n1) {
    unsigned n = kdtree_aux [n1];
    if (n < 7) {
      // Skip the tree traversal and just try all the candidates.
      for (unsigned i = 0; i != n; ++ i) {
        bounce (n1, kdtree_index [i]);
      }
      continue;
    }
    // Visit every node whose box intersects the search cube (the bounding
    // cube of the sphere of radius 2R centred on the target point).
    unsigned top = 0;
    // Push node 0 onto the stack.
    stack [top ++] = 0;
    // Traverse the tree discarding nodes not intersecting the search cube.
    unsigned first_node_of_current_level = 0;
    std::uint8_t dim = 0;
    while (top) {
      // Pop a node from the stack.
      // This node's box certainly intersects the search box.
      unsigned node = stack [-- top];
      if (node < nonleaf_count) {
        // Visit a nonleaf node.
        // Ascend to the level that contains node.
        while (first_node_of_current_level > node) {
          first_node_of_current_level /= 2;
          dim = inc_mod3 [dim + 1];
        }
        // Visit children if at least one of this node's points comes before the
        // target. First child is at i = position * count / level_node_count;
        // visit if i < n.
        std::uint64_t position = node - first_node_of_current_level;
        std::uint64_t level_node_count = first_node_of_current_level + 1;
        if (position * count < n * level_node_count) {
          // Push one or both child nodes onto the stack.
          float s = kdtree_split [node];
          if (x [n1] [dim] + 2 * radius >= s) stack [top ++] = 2 * node + 2;
          if (x [n1] [dim] - 2 * radius <= s) stack [top ++] = 2 * node + 1;
          // Descend one level to our children's level.
          dim = inc_mod3 [dim];
          first_node_of_current_level = 2 * first_node_of_current_level + 1;
        }
      }
      else {
        // Visit a leaf node.
        std::uint64_t position = node - nonleaf_count;
        unsigned points_begin = position * count >> depth;
        unsigned points_end = (position + 1) * count >> depth;
        for (unsigned i = points_begin; i != points_end && i < n; ++ i) {
          bounce (n1, kdtree_index [i]);
        }
      }
    }
  }

  // Phase 3: detect collisions with walls.
  for (unsigned iw = 0; iw != 6; ++ iw) {
    // Visit every node whose wall-distance is less than max_radius.
    // The "wall-distance" of a point x is dot(x - anchor, normal).
    // The "wall-distance" of a node is its critical corner's wall-distance.
    // The "critical corner" of a node is the corner nearest the wall.
    const v4f anchor = load4f (walls [iw] [0]);
    const v4f normal = load4f (walls [iw] [1]);
    unsigned top = 0;
    // The root (outer) node box's corners are at infinity. Don't actually
    // use infinity, in order to work under "-ffinite-math-only".
    const v4f big_val = { 0x1.0P+060f, 0x1.0P+060f, 0x1.0P+060f, 0.0f, };
    const v4f sign_bit = { -0.0f, -0.0f, -0.0f, 0.0f, };
    v4f critical_corner = _mm_xor_ps (big_val,
      _mm_and_ps (_mm_cmpge_ps (normal, _mm_setzero_ps ()), sign_bit));
    // Push node 0 onto the stack.
    store4f (stack_corner [top], critical_corner);
    stack [top] = 0;
    ++ top;
    // Traverse the tree.
    unsigned normal_sign_mask = _mm_movemask_ps (normal);
    unsigned first_node_of_current_level = 0;
    std::uint8_t dim = 0;
    while (top) {
      // Pop a node from the stack.
      -- top;
      v4f critical_corner = load4f (stack_corner [top]);
      unsigned node = stack [top];
      // Ascend to the level that contains node.
      while (first_node_of_current_level > node) {
        first_node_of_current_level /= 2;
        dim = inc_mod3 [dim + 1];
      }
      while (node < nonleaf_count) {
        // Visit a nonleaf node.
        // Precondition: node's wall-distance is less than max_radius.
        // The favourite child also qualifies as it shares our critical corner.
        unsigned favourite = normal_sign_mask >> dim & 1;
        unsigned other = favourite ^ 1;
        // Construct the other child's critical corner in place on the stack.
        store4f (stack_corner [top], critical_corner);
        stack_corner [top] [dim] = kdtree_split [node];
        // The other child's critical corner might be in H; if so ...
        v4f corner = load4f (stack_corner [top]);
        float corner_distance = _mm_cvtss_f32 (dot (corner - anchor, normal));
        if (corner_distance < radius) {
          // ...  push its node index (its critical corner is already in place).
          stack [top] = 2 * node + 1 + other;
          ++ top;
        }
        // Visit the favourite child now.
        node = 2 * node + 1 + favourite;
        first_node_of_current_level = 2 * first_node_of_current_level + 1;
        dim = inc_mod3 [dim];
      }
      // Visit a leaf node.
      std::uint64_t position = node - nonleaf_count;
      unsigned points_begin = position * count >> depth;
      unsigned points_end = (position + 1) * count >> depth;
      for (unsigned n = points_begin; n != points_end; ++ n) {
        wall_bounce (iw, kdtree_index [n]);
      }
    }
  }
}

#endif
