// An Open-Source Toolbox for Computer-Aided Investigation
// on the Fundamental Limits of Information Systems, Version 0.1
//
// Chao Tian, James S. Plank and Brent Hurst
// https://github.com/ct2641/CAI/releases/tag/0.1
// October, 2019
//
#include <stdlib.h>
#include <stdio.h>
#include <set>
#include <map>
#include <math.h>
#include <cstdint>
#include <stdexcept>
#include "problem_description.hpp"
#include "ERfromPD.h"

using namespace std;
using namespace CAI;

typedef std::runtime_error SRE;

static bool JSP_Info = false;

// this will only do permutation
// return the permuted result if is results in a valid subset, otherwise return 1<<num_rvs

int do_permutation(int number, const char *perm, int numRV)
{
	// take the permutation and give the permuted index for input number
	int newnumber = 0;
	for (int j = 0; j < numRV; j++) {
		//need to be more careful here since some permutation has non-mapped slot: check if perm[j]>=0
		if (((number >> j) & 1) == 1) {
			if (perm[j] >= 0)
				newnumber = newnumber | (1 << perm[j]);
			else { // if not, this permutation leads to a subset of random variables not in the valid mask (restricted set)
				newnumber = -1;
				break;
			}
		}
	}
	return(newnumber);
}

int reduce(int number, const Problem_Description_C_Style* PD)
{
	while (1) {
		int oldnumber = number;
		for (int i = PD->num_dependency - 1; i >= 0; i--)
		{
			if ((PD->list_dependency[i * 2 + 1] & number) == PD->list_dependency[i * 2 + 1])
				number = (number&(~PD->list_dependency[i * 2])) | PD->list_dependency[i * 2 + 1];
		}
		if (oldnumber == number)
			return(number);
	}
}

Extracted_Reductions::Extracted_Reductions()
{
	num_effective = 0;
	num_pairs = 0;
}

// This function will make the mapping tables.
// Two arrays are produced: one is the mappingtable for each subset, the other is the
// reverse mapping table. The first one is used in generating the LP< but the reverse mapping
// table is useful when interpreting the result.

void Extracted_Reductions::Make_Mappingtable(const Problem_Description_C_Style* PD)
{
  int length = (1 << PD->num_rvs);           // Number of subsets
  int subset;                                // Subset that we're enumerating
  int subid;                                 // Set id of that subset
  int next;                                  // Next permutation of subset
  int nid;                                   // Set id of next
  int canonical;                             // Canonical version of subset
  int canid;                                 // Set id of the canonical version
  int i, j, k, l, e, bit;                    // Iterators
  char buf[1000];                            // For errors.
  int key;                                   // For turning a character in a string to a digit.
  int given, and_you_become;                 // Subsets of the dependencies
  vector <int> enumerate;                    // Bit locations of of bits not in the given set.
  int have;                                  // Subset that we enumerate, of bits in "enumerate"
  int given_id, ayb_id;                      // Set id of (have | given) and (have | and_you_become)

  vector <int> order;                        // -1 for not in the eq.  Otherwise, oder in the eq.
  vector <int> digits;                       // These are the digits in the equiv class.
  vector <int> class_counts;                 // Counts of digits used for each class in a subset.
  vector < vector <int> > links;             //
                                             //
                                             //
  vector <int> pairings_base;                // These are like char_to_ec, but map chars to
  vector <int> pairings_link;                // base/links in link strings.
  int nlinks;
  multimap <string, int> dmap;
  multimap <string, int>::iterator dit;
  string g;

  printf("Total number of elements before reduction: %d\n", length);

  /* Step #0: Create the disjoint set instance whose elements are the power set of |RV| */

  DS.Initialize(length);

  /* Step #1A is to union together all of subsets that are equivalent
     according to the symmetries. */

  for (subset = 1; subset < length; subset++) {
    /* I should test (DS.Sizes[DS.Find(subset)] == 1), but this is more efficient */
    if (DS.Links[subset] == -1 && DS.Sizes[subset] == 1) {
      subid = DS.Find(subset);
      for (i = 1; i < PD->num_symmetrymap; i++) {
        next = do_permutation(subset, PD->list_symmetry[i], PD->num_rvs);
        if (next >= 0) {
          nid = DS.Find(next);
          if (nid != subid) subid = DS.Union(subid, nid);
        }
      }
    }
  }

  /* Step #1B is to use the equivalence classes to render all permutations of the
     equivalence classes equivalent.  What we're going to do is turn each element
     into a canonical form defined by the equivalence class, and then union it
     with that canonical form.   Since I want to allow "linking" of other RV's
     to RV's in the equivalence class, I'm only going go allow the specification
     of one class at a time.  Otherwise, this is a disaster.   So -- in the class
     is 'x'.  Out of the class is '.' */

  for (e = 0; e < (int) PD->equivalences.size(); e++) {

    /* First, let's turn the equivalence string into something that's easier to process.
       char_to_ec turns a character into an equivalence class.
       ec[digit] is the equivalence class for digit i.
       eq_digits[i] is a vector of the potential digits for equivalence class i. */

    order.clear();
    links.clear();
    order.resize(PD->num_rvs, -1);
    links.resize(PD->equivalences[e].size());

    for (i = 0; i < (int) PD->equivalences[e][0].size(); i++) {
      key = PD->equivalences[e][0][i];
      if (key != '.' && key != 'x') {
        throw SRE((string) "Bad equivalence class - must be '+' or '.': " + PD->equivalences[e][0]);
      }
      if (key == 'x') {
        order[i] = links[0].size();
        links[0].push_back(i);
      }
    }

    /* Print state */

    // printf("OR:"); for (i = 0; i < (int) order.size(); i++) printf(" %d", order[i]); printf("\n");

    for (l = 1; l < (int) PD->equivalences[e].size(); l++) {

      /* First, create the pairings, based on the matching character keys.  Use
         class_counts to make sure that the number of pairings in a class is their all or
         nothing.  */

      nlinks = 0;
      pairings_base.clear();
      pairings_link.clear();
      pairings_base.resize(128, -1);
      pairings_link.resize(128, -1);

      for (j = 0; j < (int) PD->equivalences[e][l].size(); j++) {
        key = PD->equivalences[e][l][j];
        if (key != '.') {
          if ((order[j] >= 0 && pairings_base[key] != -1) ||
              (order[j] < 0 && pairings_link[key] != -1)) {
            sprintf(buf, "Digit %d represented by %c in link string %s is bound more than once.",
                    j, key, PD->equivalences[e][l].c_str());
            throw SRE(buf);
          }
          if (order[j] >= 0) {
            pairings_base[key] = j;
            nlinks++;
          } else if (order[j] == -1) {
            pairings_link[key] = j;
            order[j] = -2;
          } else {
            sprintf(buf, "Digit %d represented by %c in link %s is bound in another string.",
                    j, key, PD->equivalences[e][l].c_str());
            throw SRE(buf);
          }
        }
      }

      /* Error check using class_counts. */

      if (nlinks != (int) links[0].size()) {
        sprintf(buf, "Link string %s missing digits from an equivalence class.",
                  PD->equivalences[e][l].c_str());
        throw(SRE(buf));
      }

      /* Now for each matching key in parings_link/pairings_base, push the link onto the
         base's link vector.  Error check, of course.   */

      for (key = 0; key < (int) pairings_base.size(); key++) {
        if (pairings_base[key] != -1 && pairings_link[key] != -1) {
          links[l].push_back(pairings_link[key]);
        } else if (pairings_base[key] != -1 || pairings_link[key] != -1) {
          sprintf(buf, "Character %c in link string %s has no match",
                  key, PD->equivalences[e][l].c_str());
          throw SRE(buf);
        }
      }
    }

    /* Print state */

//     for (i = 0; i < (int) links.size(); i++) {
//       if (links[i].size() > 0) {
//         printf("Links %d:", i);
//         for (j = 0; j < (int) links[i].size(); j++) printf(" %d", links[i][j]);
//         printf("\n");
//       }
//     }

    /* Next, for each subset, turn it into its canonical form, and then union it with
       its canonical form.   To create a canonical form, take each grouping of bits,
       defined by an element and its links.  Each of these can take any order, so sort
       them, and that's the order. */

    for (subset = 1; subset < length; subset++) {
      canonical = 0;
//      printf("Subset: ");
//      for (i = 0; i < PD->num_rvs; i++) printf("%s", (subset & (1 << i)) ? "1" : "0");
//      printf("\n");

      for (i = 0; i < PD->num_rvs; i++) if (order[i] == -1) canonical |= (subset & (1 << i));

      dmap.clear();
      for (i = 0; i < (int)links[0].size(); i++) {
        g.clear();
        for (j = 0; j < (int)links.size(); j++) {
          g.push_back((subset & (1 << links[j][i])) ? '1' : '0');
        }
        dmap.insert(make_pair(g, i));
      }

      i = 0;
      for (dit = dmap.begin(); dit != dmap.end(); dit++) {
        // printf("G: %s (index %d)\n", dit->first.c_str(), dit->second);
        for (j = 0; j < (int)dit->first.size(); j++) {
          // printf("Bit %c goes to digit %d\n", dit->first[j], links[j][i]);
          if (dit->first[j] == '1') canonical |= (1 << links[j][i]);
        }
        i++;
      }

      // printf("Subset: ");
      // for (i = 0; i < PD->num_rvs; i++) printf("%s", (subset & (1 << i)) ? "1" : "0");

      // printf(" Canonical: ");
      // for (i = 0; i < PD->num_rvs; i++) printf("%s", (canonical & (1 << i)) ? "1" : "0");
      // printf("\n");

      if (canonical != subset) {
        subid = DS.Find(subset);
        canid = DS.Find(canonical);
        if (subid != canid) DS.Union(subid, canid);
        // printf("Doing Union of %d and %d\n", subset, canonical);
      }
    }
  }

  if (JSP_Info) DS.Print_Equiv();

  /* Step #2 is to walk through the dependencies, and then enumerate all subsets that have the
     given set, and for each of these:

      - Check the set's id vs. the id of the set unioned with the dependency.
      - If they're different, union them.

     I'll also note that you can do step 1 and step 2 in either order. */

  for (i = 0; i < PD->num_dependency; i++) {
    given = PD->list_dependency[i*2+1];
    and_you_become = ( PD->list_dependency[i*2] | given );
    if (JSP_Info) printf("Given: %d.  And_You_Become: %d\n", given, and_you_become);
    enumerate.clear();
    for (j = 0; j < PD->num_rvs; j++) {           // Determine the bits that you need to enumerate
      bit = (1 << j);
      if ((bit & given) == 0)  enumerate.push_back(bit);
    }
    for (k = 0; k != (1 << enumerate.size()); k++) {   // Now enumerate them, put them into "have".
      have = 0;
      for (j = 0; j < (int) enumerate.size(); j++) {
        if (k & (1 << j)) have |= enumerate[j];
      }
      given_id = DS.Find(given | have);         // If (have | given) is in a different set than
      ayb_id = DS.Find(and_you_become | have);  // (have | and_you_become), union the sets.
      if (given_id != ayb_id) {
        DS.Union(given_id, ayb_id);
        if (JSP_Info) printf("Union of %d and %d\n", given | have, and_you_become | have);
      }
    }
  }

  if (JSP_Info) DS.Print_Equiv();

  /* Step 3 -- create the mapping table and inverse mapping table. */

  num_effective = 0;
  forward_mappingtable.clear();
  forward_mappingtable.resize(length + PD->num_add_LPvars, 0);

  /* We start at 1, because we don't want the set with { 0 } in our mapping table. */

  for (i = 1; i < (int) DS.Set_Ids.size(); i++) {
    num_effective++;
    forward_mappingtable[DS.Set_Ids[i]] = num_effective;
  }

  inverse_mappingtable.clear();
  inverse_mappingtable.resize(num_effective + 1 + PD->num_add_LPvars, 0);

  for (int i = 1; i < length; i++) {
    if (forward_mappingtable[i] != 0) inverse_mappingtable[forward_mappingtable[i]] = i;
    forward_mappingtable[i] = forward_mappingtable[DS.Find(i)];
  }

  for (int i = 0; i < PD->num_add_LPvars; i++) {
    num_effective++;
    forward_mappingtable[i + length] = num_effective;
    inverse_mappingtable[num_effective] = i + length;
  }

  printf("Total number of elements after reduction: %d\n", num_effective);
}

/* JSP: This determines pairs of elements whose subset id's (in the disjoint set) are
   in different sets. */

void Extracted_Reductions::Make_Pairs(const Problem_Description_C_Style* PD)
{
  int i, j;
  set <int> sets;
  int s, sid;

  pairs.clear();
  pairs.resize(2);

  for (i = PD->num_rvs-1; i >= 1; i--) {
    for (j = i-1; j >= 0; j--) {
      s = ((1 << i) | (1 << j));
      sid = DS.Find(s);
      if (sets.find(sid) == sets.end()) {
        sets.insert(sid);
        pairs[0].push_back(i);
        pairs[1].push_back(j);
      }
    }
  }
   
  num_pairs = pairs[0].size();
}
    
/* JSP: Changed to use a map rather than an O(n^2) loop. */

int Extracted_Reductions::Map_Compress(int* positions, double* values, int num_initial) const
{
  map <int, int> uniques;
  vector <int> pos;
  vector <double> val;
  int k;
  int num_effective;
 
  for (k = 0; k < num_initial; k++) {
    if (positions[k] != 0) {
      positions[k] = forward_mappingtable[positions[k]];
      if (uniques.find(positions[k]) == uniques.end()) {
        uniques[positions[k]] = pos.size();
        pos.push_back(positions[k]);
        val.push_back(values[k]);
      } else {
        val[uniques[positions[k]]] += values[k];
      }
    }
  }

  num_effective = 0;
  for (k = 0; k < (int) pos.size(); k++) {
    if (fabs(val[k]) >= TOLERANCE_LPGENERATION) {
      positions[num_effective] = pos[k];
      values[num_effective] = val[k];
      num_effective++;
    }
  }
      
  /* for (int k = 0; k < num_effective; k++) {
    printf("CM: %d %lf\n", positions[k], values[k]);
   } */
  return num_effective;
}
