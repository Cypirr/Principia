﻿
#pragma once

#include "mathematica/mathematica.hpp"

#include <cmath>
#include <string>
#include <tuple>
#include <vector>

#include "base/not_constructible.hpp"

namespace principia {
namespace mathematica {
namespace internal_mathematica {

using base::not_constructible;
using base::not_null;
using quantities::DebugString;
using quantities::IsFinite;
using quantities::SIUnit;

// A helper struct to scan the elements of a tuple and stringify them.
template<int index, typename... Types>
struct TupleHelper : not_constructible {
  static void ToMathematicaStrings(
      std::tuple<Types...> const& tuple,
      not_null<std::vector<std::string>*> const expressions) {
    TupleHelper<index - 1, Types...>::ToMathematicaStrings(tuple, expressions);
    expressions->push_back(ToMathematica(std::get<index - 1>(tuple)));
  }
};

template<typename... Types>
struct TupleHelper<0, Types...> : not_constructible {
  static void ToMathematicaStrings(
      std::tuple<Types...> const& tuple,
      not_null<std::vector<std::string>*> const expressions) {}
};

inline std::string Apply(
    std::string const& function,
    std::vector<std::string> const& arguments) {
  std::string result;
  result += function;
  result += "[";
  for (int i = 0; i < arguments.size(); ++i) {
    result += arguments[i];
    result += (i + 1 == arguments.size() ? "]" : ",");
  }
  return result;
}

template<typename T>
std::string Option(std::string const& name, T const& right) {
  return Apply("Rule", {name, ToMathematica(right)});
}

template<typename T>
std::string Assign(std::string const& name, T const& right) {
  return Apply("Set", {name, ToMathematica(right)}) + ";\n";
}

template<typename T, typename U>
std::string PlottableDataset(std::vector<T> const& x, std::vector<U> const& y) {
  std::vector<std::string> const xy = {ToMathematica(x), ToMathematica(y)};
  return Apply("Transpose", {ToMathematica(xy)});
}

template<typename T>
std::string ToMathematica(std::vector<T> const& list) {
  std::vector<std::string> expressions;
  expressions.reserve(list.size());
  for (auto const& expression : list) {
    expressions.emplace_back(ToMathematica(expression));
  }
  return Apply("List", expressions);
}

inline std::string ToMathematica(double const& real) {
  if (std::isinf(real)) {
    if (real > 0.0) {
      return "Infinity";
    } else {
      return Apply("Minus", {"Infinity"});
    }
  } else if (std::isnan(real)) {
    return "Indeterminate";
  } else {
    std::string s = DebugString(real);
    s.replace(s.find("e"), 1, "*^");
    return Apply("SetPrecision", {s, "MachinePrecision"});
  }
}

template<typename D>
std::string ToMathematica(Quantity<D> const& quantity) {
  std::string s = DebugString(quantity);
  if (IsFinite(quantity)) {
    s.replace(s.find("e"), 1, "*^");
  }
  std::string const number = ToMathematica(quantity / SIUnit<Quantity<D>>());
  std::size_t const split = s.find(" ");
  std::string const units = Escape(s.substr(split, s.size()));
  return Apply(
      "SetPrecision",
      {Apply("Quantity", {number, units}), "MachinePrecision"});
}

template<typename S, typename F>
std::string ToMathematica(Vector<S, F> const & vector) {
  auto const& coordinates = vector.coordinates();
  std::vector<std::string> expressions;
  expressions.emplace_back(ToMathematica(coordinates.x));
  expressions.emplace_back(ToMathematica(coordinates.y));
  expressions.emplace_back(ToMathematica(coordinates.z));
  return Apply("List", expressions);
}

template<typename V>
std::string ToMathematica(Point<V> const & point) {
  return ToMathematica(point - Point<V>());
}

template<typename... Types>
std::string ToMathematica(std::tuple<Types...> const& tuple) {
  std::vector<std::string> expressions;
  expressions.reserve(sizeof...(Types));
  TupleHelper<sizeof...(Types), Types...>::ToMathematicaStrings(
      tuple, &expressions);
  return Apply("List", expressions);
}

inline std::string ToMathematica(std::string const& str) {
  return str;
}

// Wraps the string in quotes.
// TODO(egg): escape things properly.
inline std::string Escape(std::string const& str) {
  std::string result = {"\""};
  result += str;
  result += "\"";
  return result;
}

}  // namespace internal_mathematica
}  // namespace mathematica
}  // namespace principia
