#pragma once

/**
 * A few methods that are missing from STL.
 *
 * @author Dávid Márk Nemeskey
 */

#include <string>
#include <set>
#include <algorithm>
#include <sstream>

/** Joins the elements of container @p c, interspeding them with @p delim. */
template <class C> std::string join(C c, const std::string& delim=", ") {
  std::ostringstream ss;
  for (typename C::const_iterator it = c.begin(); it != c.end(); ++it) {
    ss << *it << delim;
  }
  std::string ret = ss.str();
  return ret.substr(0, ret.size() - delim.size());
}


/** Returns @c true if @p first is fully included in @p second. */
template <class T> bool set_in(std::set<T> first, std::set<T> second) {
  std::set<T> intersect;
  std::set_intersection(first.begin(), first.end(), second.begin(), second.end(),
                        std::inserter(intersect, intersect.begin()));
  return intersect.size() == first.size();
}

/** Returns @c true if @p first == @p second. */
template <class T> bool set_equal(std::set<T> first, std::set<T> second) {
  if (first.size() != second.size()) return false;
  return set_in(first, second);
}
