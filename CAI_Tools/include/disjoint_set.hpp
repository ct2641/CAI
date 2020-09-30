#pragma once
#include <vector>
#include <list>

/** This is a class that implements disjoint sets, using "union by rank with path compression."
    It maintains some extra information that can be useful when you're using disjoint sets.
    Running times:

       - Initialize: O(n).
       - Union: O(1).
       - Find: O(alpha(n)), which is basically O(1).
       - Print: O(n)
       - Print_Equiv: O(n log n)
  */

class Disjoint_Set {
  public:
    void Initialize(int num_elements);
    int Union(int s1, int s2);             // s1 and s2 are set id's, not elements.  
                                           // Union() returns the set id of the new union.
    int Find(int element);                 // return the set id of the element.
    void Print() const;
    void Print_Equiv() const;

    /* These vectors are maintained by Union()/Find().  If I could make them read only,
       I would.  I'm making them public so that anyone may use them efficiently, but
       do not modify them, as they are quite interrelated.
     */

    std::vector <int> Links;          // Parent pointer in the set.  If you're the root, then -1,
                                      // and the set id of the set is your index.
    std::vector <int> Sizes;          // Only valid for roots.  It's the size of the set.
    std::vector <int> Ranks;          // Used for union-by-rank.

    std::vector <int> Set_Ids;        // A list of the roots, in no particular order.
    std::vector <int> Set_Id_Indices; // Only valid for roots. It is the root's index in Set_Ids.

    std::vector < std::list <int> > Elements;  // Only valid for roots. The elements of the set.
};
