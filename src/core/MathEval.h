#pragma once

#include <optional>
#include <QString>

// Small calculator for the launcher (Noctalia uses libqalculate; this covers everyday math).
//
//   2+2*3        -> 8          50% -> 0.5         2^10 -> 1024       10 mod 3 -> 1
//   sqrt(2)      -> 1.414...   2pi -> 6.283...    3,5*2 -> 7 (comma = decimal point)
//   sin cos tan asin acos atan (radians), sind cosd tand (degrees), log (base 10), ln, exp,
//   abs, round, floor, ceil, cbrt, constants pi and e. Operators: + - * x × / ÷ ^ % ( )
namespace matheval {

  // Returns nullopt when `text` isn't a math expression (so app search isn't polluted).
  std::optional<double> evaluate(const QString& text);

  // true if `text` has at least one digit and one operator or function, i.e. worth evaluating.
  bool looksLikeMath(const QString& text);

  // Human-readable result in the user's locale (e.g. "3,1415926536").
  QString format(double value);

} // namespace matheval
