#include "core/MathEval.h"

#include <QHash>
#include <QLocale>
#include <QStringList>

#include <cmath>
#include <numbers>

namespace matheval {

  namespace {

    class Parser {
    public:
      explicit Parser(const QString& s) : m_s(s) {}

      std::optional<double> run() {
        skipSpaces();
        auto v = parseExpr();
        skipSpaces();
        if (!v || m_pos != m_s.size()) {
          return std::nullopt;
        }
        if (!std::isfinite(*v)) {
          return std::nullopt;
        }
        return v;
      }

    private:
      QChar peek() const { return m_pos < m_s.size() ? m_s[m_pos] : QChar(); }
      void skipSpaces() {
        while (m_pos < m_s.size() && m_s[m_pos].isSpace()) {
          ++m_pos;
        }
      }
      bool eat(QChar c) {
        skipSpaces();
        if (peek() == c) {
          ++m_pos;
          return true;
        }
        return false;
      }
      bool eatWord(const QString& w) {
        skipSpaces();
        if (m_s.mid(m_pos, w.size()).compare(w, Qt::CaseInsensitive) == 0) {
          const int after = m_pos + w.size();
          if (after >= m_s.size() || !m_s[after].isLetter()) {
            m_pos = after;
            return true;
          }
        }
        return false;
      }

      // expr := term (('+'|'-') term)*
      std::optional<double> parseExpr() {
        auto left = parseTerm();
        while (left) {
          if (eat('+')) {
            auto r = parseTerm();
            if (!r) return std::nullopt;
            *left += *r;
          } else if (eat('-') || eat(QChar(0x2212))) {
            auto r = parseTerm();
            if (!r) return std::nullopt;
            *left -= *r;
          } else {
            break;
          }
        }
        return left;
      }

      // term := power (('*'|'x'|'×'|'/'|'÷'|'mod'|implicit) power)*
      std::optional<double> parseTerm() {
        auto left = parseUnary();
        while (left) {
          skipSpaces();
          if (eat('*') || eat(QChar(0x00D7)) || eat('x') || eat('X')) {
            auto r = parseUnary();
            if (!r) return std::nullopt;
            *left *= *r;
          } else if (eat('/') || eat(QChar(0x00F7)) || eat(':')) {
            auto r = parseUnary();
            if (!r) return std::nullopt;
            *left /= *r;
          } else if (eatWord("mod")) {
            auto r = parseUnary();
            if (!r) return std::nullopt;
            *left = std::fmod(*left, *r);
          } else if (peek() == '(' || peek().isLetter()) {
            // implicit multiplication: 2pi, 3(4+1), 2sqrt(9)
            auto r = parseUnary();
            if (!r) return std::nullopt;
            *left *= *r;
          } else {
            break;
          }
        }
        return left;
      }

      std::optional<double> parseUnary() {
        if (eat('-') || eat(QChar(0x2212))) {
          auto v = parseUnary();
          if (!v) return std::nullopt;
          return -*v;
        }
        if (eat('+')) {
          return parseUnary();
        }
        return parsePower();
      }

      // power := postfix ('^' unary)?   (right associative)
      std::optional<double> parsePower() {
        auto base = parsePostfix();
        if (!base) return std::nullopt;
        if (eat('^') || (peek() == '*' && m_pos + 1 < m_s.size() && m_s[m_pos + 1] == '*' && (m_pos += 2, true))) {
          auto exp = parseUnary();
          if (!exp) return std::nullopt;
          return std::pow(*base, *exp);
        }
        return base;
      }

      // postfix := primary ('%' | '!')*
      std::optional<double> parsePostfix() {
        auto v = parsePrimary();
        while (v) {
          if (eat('%')) {
            *v /= 100.0;
          } else if (eat('!')) {
            if (*v < 0 || *v > 170 || std::floor(*v) != *v) return std::nullopt;
            *v = std::tgamma(*v + 1.0);
          } else {
            break;
          }
        }
        return v;
      }

      std::optional<double> parsePrimary() {
        skipSpaces();
        if (eat('(')) {
          auto v = parseExpr();
          if (!v || !eat(')')) return std::nullopt;
          return v;
        }
        const QChar c = peek();
        if (c.isDigit() || c == '.' || c == ',') {
          return parseNumber();
        }
        if (c.isLetter() || c == QChar(0x03C0) /* π */) {
          return parseIdentifier();
        }
        return std::nullopt;
      }

      std::optional<double> parseNumber() {
        QString num;
        bool sawSep = false;
        while (m_pos < m_s.size()) {
          const QChar c = m_s[m_pos];
          if (c.isDigit()) {
            num += c;
          } else if ((c == '.' || c == ',') && !sawSep) {
            sawSep = true;
            num += '.';
          } else if (c == '_' || c == QChar(0x2009)) {
            // digit grouping
          } else {
            break;
          }
          ++m_pos;
        }
        // scientific notation: 1e3, 2.5e-4
        if (m_pos < m_s.size() && (m_s[m_pos] == 'e' || m_s[m_pos] == 'E')) {
          int p = m_pos + 1;
          QString exp = "e";
          if (p < m_s.size() && (m_s[p] == '+' || m_s[p] == '-')) exp += m_s[p++];
          if (p < m_s.size() && m_s[p].isDigit()) {
            while (p < m_s.size() && m_s[p].isDigit()) exp += m_s[p++];
            num += exp;
            m_pos = p;
          }
        }
        bool ok = false;
        const double v = num.toDouble(&ok);
        return ok ? std::optional<double>(v) : std::nullopt;
      }

      std::optional<double> parseIdentifier() {
        if (peek() == QChar(0x03C0)) {
          ++m_pos;
          return std::numbers::pi;
        }
        int start = m_pos;
        while (m_pos < m_s.size() && m_s[m_pos].isLetter()) {
          ++m_pos;
        }
        const QString name = m_s.mid(start, m_pos - start).toLower();
        if (name == "pi") return std::numbers::pi;
        if (name == "e") return std::numbers::e;
        if (name == "tau") return 2 * std::numbers::pi;

        using Fn = double (*)(double);
        static const QHash<QString, Fn> fns{
            {"sqrt", [](double x) { return std::sqrt(x); }},
            {"raiz", [](double x) { return std::sqrt(x); }},
            {"cbrt", [](double x) { return std::cbrt(x); }},
            {"sin", [](double x) { return std::sin(x); }},
            {"sen", [](double x) { return std::sin(x); }},
            {"cos", [](double x) { return std::cos(x); }},
            {"tan", [](double x) { return std::tan(x); }},
            {"tg", [](double x) { return std::tan(x); }},
            {"sind", [](double x) { return std::sin(x * std::numbers::pi / 180); }},
            {"cosd", [](double x) { return std::cos(x * std::numbers::pi / 180); }},
            {"tand", [](double x) { return std::tan(x * std::numbers::pi / 180); }},
            {"asin", [](double x) { return std::asin(x); }},
            {"acos", [](double x) { return std::acos(x); }},
            {"atan", [](double x) { return std::atan(x); }},
            {"log", [](double x) { return std::log10(x); }},
            {"ln", [](double x) { return std::log(x); }},
            {"exp", [](double x) { return std::exp(x); }},
            {"abs", [](double x) { return std::fabs(x); }},
            {"round", [](double x) { return std::round(x); }},
            {"floor", [](double x) { return std::floor(x); }},
            {"ceil", [](double x) { return std::ceil(x); }},
        };
        const auto it = fns.constFind(name);
        if (it == fns.constEnd()) {
          return std::nullopt;
        }
        // function argument: either (expr) or a single unary term (sqrt 9)
        auto arg = parseUnary();
        if (!arg) return std::nullopt;
        return it.value()(*arg);
      }

      const QString m_s;
      int m_pos = 0;
    };

  } // namespace

  bool looksLikeMath(const QString& text) {
    bool digit = false;
    bool op = false;
    for (const QChar c : text) {
      if (c.isDigit()) {
        digit = true;
      } else if (QStringLiteral("+-*/^%()×÷!x").contains(c) || c == QChar(0x2212)) {
        op = true;
      }
    }
    if (digit && op) {
      return true;
    }
    // "sqrt 2", "pi", "sin(1)"
    static const QStringList words{"sqrt", "raiz", "sin", "sen", "cos", "tan", "log", "ln", "pi", "abs", "exp", "mod"};
    const QString t = text.toLower();
    for (const QString& w : words) {
      if (t.contains(w) && (digit || w == "pi")) {
        return true;
      }
    }
    return false;
  }

  std::optional<double> evaluate(const QString& text) {
    const QString t = text.trimmed();
    if (t.isEmpty()) {
      return std::nullopt;
    }
    return Parser(t).run();
  }

  QString format(double value) {
    if (std::fabs(value) < 1e-12) {
      value = 0;
    }
    QLocale locale = QLocale::system();
    locale.setNumberOptions(QLocale::OmitGroupSeparator); // "1024", not "1.024" (ambiguous when copied)
    // Integers up to 15 digits print exactly; everything else with 12 significant digits.
    if (std::fabs(value) < 1e15 && std::floor(value) == value) {
      return locale.toString(static_cast<qlonglong>(value));
    }
    QString s = locale.toString(value, 'g', 12);
    return s;
  }

} // namespace matheval
