// -*- C++ -*-

#ifndef kdtree_h
#define kdtree_h

#include "mswin.h"
#include "compiler.h"
#include "memory.h"
#include "vector.h"
#include "partition.h"
#include "bounce.h"
#include <x86intrin.h>
#include <cstdint>

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

// Level 0:     |                   0                   |
// Level 1:     |         1         |         2         |
// Level 2:     |    3    |    4    |    5    |    6    |
// Level 3:     |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 |

// Tree structure
//   Node 0 is the root node.
//   Node m's children are nodes 2m+1 and 2m+2.
// Levels
//   There are K+1 levels, numbered 0, 1, 2, ..., K; the nodes on level K are the leaf nodes.
//   For each k, level k contains the 2^k nodes in the range [2^k-1, 2^(k+1)-1).
//   Therefore the total number of non-leaf nodes is 2^K-1.
//   Node m is in level k = logb(m+2)-1 at position i = m+1-2^k.
//   If k<K then node m has two child nodes, which are in level k+1 at positions 2i and 2i+1.
// Points
//   There are N points, numbered 0, 1, 2, ..., N-1.
//   The points are distributed evenly among the nodes of each level:
//   For each m, node m (which is in level k at position i, see above)
//   contains the points [iN/2^k, (i+1)N/2^k).
//   The "middle point" of a non-leaf node m (in level k at position i) is defined to be the first
//   point of its second child (node 2m+2, in level k+1 at position 2i+1), i.e., point (2i+1)N/2^(k+1).
// Leaf nodes
//   The bottom level, level K, contains 2^K nodes.
//   Every node in level K contains either one or two points, and at least one node in level K contains two points.
//   In other words we have 2^K < N <= 2^(K+1).
//   Explicitly, K = logb(N)-1, where for all n > 0, logb(n) is the greatest integer k such that 2^k <= n.
// KD tree
//   The data for the KD tree consists of a permutation of [0, N), represented as an array
//   of N unsigned integers (kdtree_index), and a co-ordinate value for each non-leaf node,
//   represented as an array of (2^K)-1 floating-point numbers (kdtree_split).
//   The points [begin, end) of each non-leaf node m (in level k at position i) are
//   partitioned about the node's middle point mid in dimension d = k % 3, in the sense that
//   x [kdtree_index[n]] [d] <= kdtree_split[m] for n in [begin, mid),
//   x [kdtree_index[n]] [d] >= kdtree_split[m] for n in [mid, end).

static const std::uint8_t inc_mod3 [] = { 1, 2, 0, 1, };

inline unsigned round_up_to_power_of_two (unsigned n)
{
  return 1 << _bit_scan_reverse (n - 1);
}

ALWAYS_INLINE
inline void model_t::kdtree_search ()
{
  // Undefined behaviour if count == 0.
  unsigned leaf_count = round_up_to_power_of_two (count);
  unsigned nonleaf_count = leaf_count - 1;
  if (nonleaf_count > kdtree_capacity) {
    deallocate (kdtree_split);
    kdtree_capacity = nonleaf_count;
    kdtree_split = (float *) allocate (kdtree_capacity * sizeof (float));
  }

  unsigned node = 0;
  unsigned level_node_count = 1;
  std::uint8_t dim = 0;
  while (count > 2 * level_node_count) {
    unsigned begin = 0;
    for (unsigned i = 0; i != level_node_count; ++ i) {
      unsigned end = (i + 1) * count / level_node_count;
      unsigned mid = (2 * i + 1) * count / (2 * level_node_count);
      partition (kdtree_index, x, dim, begin, mid, end);
      kdtree_split [node ++] = x [kdtree_index [mid]] [dim];
      begin = end;
    }
    level_node_count = 2 * level_node_count;
    dim = inc_mod3 [dim];
  }

  // Enough stack to traverse a tree with more than 2^32 nodes.
  unsigned stack_node [32];              // Node index
  ALIGNED16 float stack_corner [32] [4]; // Extra stack space used in the wall search phase.

  // For every pair of integers i, n such that 0 <= i < n < count,
  // if |x[i'] - x[n']| < 2R then call bounce(i', n'),
  // where i' = kdtree_index[i] and n' = kdtree_index[n].
  for (unsigned n = 1; n != count; ++ n) {
    // Visit every node whose box intersects the search cube (the bounding
    // cube of the sphere of radius 2R centred on the target point).
    unsigned stack_top = 0;
    // Push node 0 onto the stack.
    stack_node [stack_top ++] = 0;
    // Traverse the tree discarding nodes not intersecting the search cube.
    unsigned first_node_of_current_level = 0;
    std::uint8_t dim = 0;
    while (stack_top) {
      // Pop a node from the stack.
      // This node's box certainly intersects the search box.
      unsigned node = stack_node [-- stack_top];
      if (node < nonleaf_count) {
        // Visit a nonleaf node.
        // Ascend to the level that contains node.
        while (first_node_of_current_level > node) {
          first_node_of_current_level /= 2;
          dim = inc_mod3 [dim + 1];
        }
        unsigned position = node - first_node_of_current_level;
        unsigned level_node_count = first_node_of_current_level + 1;
        // Consider the children if at least one of this node's points comes before the target.
        // The first child is at i = position * count / level_node_count, and we require i < n.
        if (position * count < n * level_node_count) { // Division avoided.
          // Push one or both child nodes onto the stack.
          float s = kdtree_split [node];
          if (x [kdtree_index [n]] [dim] - 2 * radius <= s) stack_node [stack_top ++] = 2 * node + 1;
          if (x [kdtree_index [n]] [dim] + 2 * radius >= s) stack_node [stack_top ++] = 2 * node + 2;
          // Descend one level to our children's level.
          dim = inc_mod3 [dim];
          first_node_of_current_level = 2 * first_node_of_current_level + 1;
        }
      }
      else {
        // Visit a leaf node.
        unsigned position = node - nonleaf_count;
        unsigned points_begin = position * count / leaf_count;
        unsigned points_end = (position + 1) * count / leaf_count;
        for (unsigned i = points_begin; i != points_end && i < n; ++ i) {
          bounce (kdtree_index [i], kdtree_index [n]);
        }
      }
    }
  }

  // Detect collisions with walls.
  for (unsigned iw = 0; iw != 6; ++ iw) {
    // Visit every node whose critical corner's wall-distance is less than max_radius.
    // The "wall-distance" of a point x is dot(x - anchor, normal).
    // The "critical corner" of a node is the corner of the node box of least wall-distance.
    // The critical reader will see that the critical corner is not always uniquely defined,
    // and why it doesn't matter.
    const v4f anchor = load4f (walls [iw] [0]);
    const v4f normal = load4f (walls [iw] [1]);
    unsigned stack_top = 0;
    // The root (outer) node box's corners are at infinity.
    // Don't actually use infinity, in order to work under "-ffinite-math-only".
    static const v4f big_value = { 0x1.000000P+060f, 0x1.000000P+060f, 0x1.000000P+060f, 0.0f, };
    static const v4f sign_bit = { -0.0f, -0.0f, -0.0f, 0.0f, };
    v4f normal_non_negative = _mm_cmpge_ps (normal, _mm_setzero_ps ());
    v4f critical_corner = _mm_xor_ps (_mm_and_ps (normal_non_negative, sign_bit), big_value);
    // Push node 0 onto the stack.
    store4f (stack_corner [stack_top], critical_corner);
    stack_node [stack_top] = 0;
    ++ stack_top;
    // Traverse the tree.
    unsigned normal_sign_mask = _mm_movemask_ps (normal);
    unsigned first_node_of_current_level = 0;
    std::uint8_t dim = 0;
    while (stack_top) {
      // Pop a node from the stack.
      -- stack_top;
      v4f critical_corner = load4f (stack_corner [stack_top]);
      unsigned node = stack_node [stack_top];
      // Ascend to the level that contains node.
      while (first_node_of_current_level > node) {
        first_node_of_current_level /= 2;
        dim = inc_mod3 [dim + 1];
      }
      while (node < nonleaf_count) {
        // Visit a nonleaf node.
        // Precondition: node's critical corner's wall-distance is less than max_radius.
        // The favourite child's critical corner is the same as this node's so certainly also qualifies.
        unsigned favourite = normal_sign_mask >> dim & 1;
        unsigned other = favourite ^ 1;
        // Construct the other child's critical corner in place, after the end of the stack.
        store4f (stack_corner [stack_top], critical_corner);
        stack_corner [stack_top] [dim] = kdtree_split [node];
        // The other child's critical corner might be in H; if so ...
        float other_child_critical_corner_distance = _mm_cvtss_f32 (dot (load4f (stack_corner [stack_top]) - anchor, normal));
        if (other_child_critical_corner_distance < radius) {
          // ...  push the other child onto the stack (its critical corner is already in place).
          stack_node [stack_top] = 2 * node + 1 + other;
          ++ stack_top;
        }
        // Visit the favourite child now.
        node = 2 * node + 1 + favourite;
        first_node_of_current_level = 2 * first_node_of_current_level + 1;
        dim = inc_mod3 [dim];
      }
      // Visit a leaf node.
      unsigned position = node - nonleaf_count;
      unsigned points_begin = position * count / leaf_count;
      unsigned points_end = (position + 1) * count / leaf_count;
      for (unsigned n = points_begin; n != points_end; ++ n) {
        wall_bounce (iw, kdtree_index [n]);
      }
    }
  }
}

#endif
