/* disjoint_set.cpp
   Union by Rank with Path Compression implementation of Disjoint Sets.
   James S. Plank
   Tue Sep 25 15:51:14 EDT 2018
 */

#include <vector>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <iostream>
#include <set>
#include "disjoint_set.hpp"
using namespace std;

typedef std::runtime_error SRE;

void Disjoint_Set::Initialize(int nelements)
{
  size_t i;

  if (nelements <= 0) throw SRE("Disjoint_Set::Initialize called with nelements <= 0");

  Links.resize(nelements, -1);
  Ranks.resize(nelements, 1);
  Sizes.resize(nelements, 1);
  Elements.resize(nelements);
  for (i = 0; i < Links.size(); i++) {
    Set_Ids.push_back(i);
    Set_Id_Indices.push_back(i);
    Elements[i].push_back(i);
  }
}

int Disjoint_Set::Union(int s1, int s2)
{
  int p, c;
  size_t last;

  if (Links.size() == 0) throw SRE("Disjoint_Set::Union called on an uninitialized Disjoint_Set.");
  if (s1 < 0 || s1 >= (int) Links.size() || s2 < 0 || s2 >= (int) Links.size()) {
    throw SRE("Disjoint_Set::Union called on a bad element (negative or too big).");
  }
  if (Links[s1] != -1 || Links[s2] != -1) {
    throw SRE("Must call Disjoint_Set::Union on a set, and not just an element.\n");
  }

  if (Ranks[s1] > Ranks[s2]) {
    p = s1;
    c = s2;
  } else {
    p = s2;
    c = s1;
  }
  
  /* - Set the child's link to point to the parent.
     - If that changed the parent's rank, increment the parent.
     - Update the parent's size to include the child's elements.
     - Move the child's elements to the parent's elements list.  This is O(1), BTW.
   */

  Links[c] = p;
  if (Ranks[p] == Ranks[c]) Ranks[p]++;
  Sizes[p] += Sizes[c];
  Elements[p].splice(Elements[p].begin(), Elements[c]);

  // This removes the child from Set_Id's by replacing it with the last element,
  // and then deleting the last element.

  last = Set_Ids[Set_Ids.size() - 1];
  Set_Id_Indices[last] = Set_Id_Indices[c];
  Set_Ids[Set_Id_Indices[last]] = last;
  Set_Ids.pop_back();

  return p;
}

int Disjoint_Set::Find(int e)
{
  int p, c;   // P is the parent, c is the child.

  if (Links.size() == 0) throw SRE("Disjoint_Set::Find called on an uninitialized Disjoint_Set.");
  if (e < 0 || e >= (int) Links.size()) {
    throw SRE("Disjoint_Set::Find() called on a bad element (negative or too big).");
  }

  /* Find the root of the tree, but along the way, set
     the parents' Links to the children. */

  c = -1;
  while (Links[e] != -1) {
    p = Links[e];
    Links[e] = c;
    c = e;
    e = p;
  }

  /* Now, travel back to the original element, setting
     every Link to the root of the tree. */

  p = e;
  e = c;
  while (e != -1) {
    c = Links[e];
    Links[e] = p;
    e =c;
  }
  return p;
}

void Disjoint_Set::Print() const
{
  size_t i;
  int s;
  list <int>::const_iterator eit;

  printf("\n");
  printf("Node:  ");
  for (i = 0; i < Links.size(); i++) printf(" %2lu", i);  
  printf("\n");

  printf("Links: ");
  for (i = 0; i < Links.size(); i++) printf(" %2d", Links[i]);  
  printf("\n");

  printf("Ranks: ");
  for (i = 0; i < Ranks.size(); i++) printf(" %2d", Ranks[i]);  
  printf("\n");

  printf("Set IDs: {");
  for (i = 0; i < Set_Ids.size(); i++) printf("%s%d", (i == 0) ? "" : ",", Set_Ids[i]);  
  printf("}\n");

  printf("Sets: ");
  for (i = 0; i < Set_Ids.size(); i++) {
    s = Set_Ids[i];
    if (Elements[Set_Ids[i]].empty()) {
      printf("s = %d\n", s);
      throw std::logic_error("Disjoint_Set::Print() - empty=0");
    }
    eit = Elements[s].begin();
    printf("{%d", *eit);
    for (eit++; eit != Elements[s].end(); eit++) printf(",%d", *eit);
    printf("}");
  }
  printf("\n");

  printf("\n");
}

void Disjoint_Set::Print_Equiv() const
{
  set <string> str;
  set <string>::iterator stit;
  set <int> s;
  set <int>::iterator sit;
  list <int>::const_iterator eit;
  char buf[50];
  string ss;
  size_t i, j;
  

  for (i = 0; i < Set_Ids.size(); i++) {
    j = Set_Ids[i];
    s.clear();
    ss = "{";
    for (eit = Elements[j].begin(); eit != Elements[j].end(); eit++) s.insert(*eit);
    for (sit = s.begin(); sit != s.end(); sit++) {
      if (sit != s.begin()) ss += ",";
      sprintf(buf, "%d", *sit);
      ss += buf;
    }
    ss += "}"; 
    str.insert(ss);
  }

  ss = "";
  for (stit = str.begin(); stit != str.end(); stit++) {
    if (stit != str.begin()) ss += ",";
    ss += *stit;
  }
  printf("%s\n", ss.c_str());
}
