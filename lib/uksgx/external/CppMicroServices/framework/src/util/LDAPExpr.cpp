/*=============================================================================

  Library: CppMicroServices

  Copyright (c) The CppMicroServices developers. See the COPYRIGHT
  file at the top-level directory of this distribution and at
  https://github.com/CppMicroServices/CppMicroServices/COPYRIGHT .

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=============================================================================*/

#include "LDAPExpr.h"

#include "cppmicroservices/Any.h"
#include "cppmicroservices/Constants.h"

#include "Properties.h"

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace cppmicroservices {

namespace LDAPExprConstants {

static LDAPExpr::Byte WILDCARD()
{
  static LDAPExpr::Byte b = std::numeric_limits<LDAPExpr::Byte>::max();
  return b;
}

static const std::string& WILDCARD_STRING()
{
  static std::string s(1, WILDCARD());
  return s;
}

static const std::string& NULLQ()
{
  static std::string s = "Null query";
  return s;
}

static const std::string& GARBAGE()
{
  static std::string s = "Trailing garbage";
  return s;
}

static const std::string& EOS()
{
  static std::string s = "Unexpected end of query";
  return s;
}

static const std::string& MALFORMED()
{
  static std::string s = "Malformed query";
  return s;
}

static const std::string& OPERATOR()
{
  static std::string s = "Undefined operator";
  return s;
}
}

bool stricomp(const std::string::value_type& v1,
              const std::string::value_type& v2)
{
  return ::tolower(v1) == ::tolower(v2);
}

//! Contains the current parser position and parsing utility methods.
class LDAPExpr::ParseState
{

private:
  std::size_t m_pos;
  std::string m_str;

public:
  ParseState(const std::string& str);

  //! Move m_pos to remove the prefix \a pre
  bool prefix(const std::string& pre);

  /** Peek a char at m_pos
  \note If index out of bounds, throw exception
  */
  LDAPExpr::Byte peek();

  //! Increment m_pos by n
  void skip(int n);

  //! return string from m_pos until the end
  std::string rest() const;

  //! Move m_pos until there's no spaces
  void skipWhite();

  //! Get string until special chars. Move m_pos
  std::string getAttributeName();

  //! Get string and convert * to WILDCARD
  std::string getAttributeValue();

  //! Throw InvalidSyntaxException exception
  void error(const std::string& m) const;
};

class LDAPExprData : public SharedData
{
public:
  LDAPExprData(int op, const std::vector<LDAPExpr>& args)
    : m_operator(op)
    , m_args(args)
    , m_attrName()
    , m_attrValue()
  {}

  LDAPExprData(int op, std::string attrName, const std::string& attrValue)
    : m_operator(op)
    , m_args()
    , m_attrName(attrName)
    , m_attrValue(attrValue)
  {}

  LDAPExprData(const LDAPExprData& other)
    : SharedData(other)
    , m_operator(other.m_operator)
    , m_args(other.m_args)
    , m_attrName(other.m_attrName)
    , m_attrValue(other.m_attrValue)
  {}

  int m_operator;
  std::vector<LDAPExpr> m_args;
  std::string m_attrName;
  std::string m_attrValue;
};

LDAPExpr::LDAPExpr()
  : d()
{}

LDAPExpr::LDAPExpr(const std::string& filter)
  : d()
{
  ParseState ps(filter);
  try {
    LDAPExpr expr = ParseExpr(ps);

    if (!Trim(ps.rest()).empty()) {
      ps.error(LDAPExprConstants::GARBAGE() + " '" + ps.rest() + "'");
    }

    d = expr.d;
  } catch (const std::out_of_range&) {
    ps.error(LDAPExprConstants::EOS());
  }
}

LDAPExpr::LDAPExpr(int op, const std::vector<LDAPExpr>& args)
  : d(new LDAPExprData(op, args))
{}

LDAPExpr::LDAPExpr(int op,
                   const std::string& attrName,
                   const std::string& attrValue)
  : d(new LDAPExprData(op, attrName, attrValue))
{}

LDAPExpr::LDAPExpr(const LDAPExpr& other)
  : d(other.d)
{}

LDAPExpr& LDAPExpr::operator=(const LDAPExpr& other)
{
  d = other.d;
  return *this;
}

LDAPExpr::~LDAPExpr() {}

std::string LDAPExpr::Trim(std::string str)
{
  str.erase(0, str.find_first_not_of(' '));
  str.erase(str.find_last_not_of(' ') + 1);
  return str;
}

bool LDAPExpr::GetMatchedObjectClasses(ObjectClassSet& objClasses) const
{
  if (d->m_operator == EQ) {
    if (d->m_attrName.length() == Constants::OBJECTCLASS.length() &&
        std::equal(d->m_attrName.begin(),
                   d->m_attrName.end(),
                   Constants::OBJECTCLASS.begin(),
                   stricomp) &&
        d->m_attrValue.find(LDAPExprConstants::WILDCARD()) ==
          std::string::npos) {
      objClasses.insert(d->m_attrValue);
      return true;
    }
    return false;
  } else if (d->m_operator == AND) {
    bool result = false;
    for (std::size_t i = 0; i < d->m_args.size(); i++) {
      LDAPExpr::ObjectClassSet r;
      if (d->m_args[i].GetMatchedObjectClasses(r)) {
        result = true;
        if (objClasses.empty()) {
          objClasses = r;
        } else {
          // if AND op and classes in several operands,
          // then only the intersection is possible.
          LDAPExpr::ObjectClassSet::iterator it1 = objClasses.begin();
          LDAPExpr::ObjectClassSet::iterator it2 = r.begin();
          while ((it1 != objClasses.end()) && (it2 != r.end())) {
            if (*it1 < *it2) {
              objClasses.erase(it1++);
            } else if (*it2 < *it1) {
              ++it2;
            } else { // *it1 == *it2
              ++it1;
              ++it2;
            }
          }
          // Anything left in set_1 from here on did not appear in set_2,
          // so we remove it.
          objClasses.erase(it1, objClasses.end());
        }
      }
    }
    return result;
  } else if (d->m_operator == OR) {
    for (std::size_t i = 0; i < d->m_args.size(); i++) {
      LDAPExpr::ObjectClassSet r;
      if (d->m_args[i].GetMatchedObjectClasses(r)) {
        std::copy(
          r.begin(), r.end(), std::inserter(objClasses, objClasses.begin()));
      } else {
        objClasses.clear();
        return false;
      }
    }
    return true;
  }
  return false;
}

std::string LDAPExpr::ToLower(const std::string& str)
{
  std::string lowerStr(str);
  std::transform(str.begin(), str.end(), lowerStr.begin(), ::tolower);
  return lowerStr;
}

bool LDAPExpr::IsSimple(const StringList& keywords,
                        LocalCache& cache,
                        bool matchCase) const
{
  if (cache.empty()) {
    cache.resize(keywords.size());
  }

  if (d->m_operator == EQ) {
    StringList::const_iterator index;
    if ((index =
           std::find(keywords.begin(),
                     keywords.end(),
                     matchCase ? d->m_attrName : ToLower(d->m_attrName))) !=
          keywords.end() &&
        d->m_attrValue.find_first_of(LDAPExprConstants::WILDCARD()) ==
          std::string::npos) {
      cache[index - keywords.begin()] = StringList(1, d->m_attrValue);
      return true;
    }
  } else if (d->m_operator == OR) {
    for (std::size_t i = 0; i < d->m_args.size(); i++) {
      if (!d->m_args[i].IsSimple(keywords, cache, matchCase))
        return false;
    }
    return true;
  }
  return false;
}

bool LDAPExpr::IsNull() const
{
  return !d;
}

bool LDAPExpr::Evaluate(const PropertiesHandle& p, bool matchCase) const
{
  if ((d->m_operator & SIMPLE) != 0) {
    // try case sensitive match first
    int index = p->FindCaseSensitive_unlocked(d->m_attrName);
    if (index < 0 && !matchCase)
      index = p->Find_unlocked(d->m_attrName);
    return index < 0
             ? false
             : Compare(p->Value_unlocked(index), d->m_operator, d->m_attrValue);
  } else { // (d->m_operator & COMPLEX) != 0
    switch (d->m_operator) {
      case AND:
        for (std::size_t i = 0; i < d->m_args.size(); i++) {
          if (!d->m_args[i].Evaluate(p, matchCase))
            return false;
        }
        return true;
      case OR:
        for (std::size_t i = 0; i < d->m_args.size(); i++) {
          if (d->m_args[i].Evaluate(p, matchCase))
            return true;
        }
        return false;
      case NOT:
        return !d->m_args[0].Evaluate(p, matchCase);
      default:
        return false; // Cannot happen
    }
  }
}

bool LDAPExpr::Compare(const Any& obj, int op, const std::string& s) const
{
  if (obj.Empty())
    return false;
  if (op == EQ && s == LDAPExprConstants::WILDCARD_STRING())
    return true;

  try {
    const std::type_info& objType = obj.Type();
    if (objType == typeid(std::string)) {
      return CompareString(ref_any_cast<std::string>(obj), op, s);
    } else if (objType == typeid(std::vector<std::string>)) {
      const std::vector<std::string>& list =
        ref_any_cast<std::vector<std::string>>(obj);
      for (std::size_t it = 0; it != list.size(); it++) {
        if (CompareString(list[it], op, s))
          return true;
      }
    } else if (objType == typeid(std::list<std::string>)) {
      const std::list<std::string>& list =
        ref_any_cast<std::list<std::string>>(obj);
      for (std::list<std::string>::const_iterator it = list.begin();
           it != list.end();
           ++it) {
        if (CompareString(*it, op, s))
          return true;
      }
    } else if (objType == typeid(char)) {
      return CompareString(std::string(1, ref_any_cast<char>(obj)), op, s);
    } else if (objType == typeid(bool)) {
      if (op == LE || op == GE)
        return false;

      std::string boolVal = any_cast<bool>(obj) ? "true" : "false";
      return std::equal(s.begin(), s.end(), boolVal.begin(), stricomp);
    } else if (objType == typeid(short)) {
      return CompareIntegralType<short>(obj, op, s);
    } else if (objType == typeid(int)) {
      return CompareIntegralType<int>(obj, op, s);
    } else if (objType == typeid(long int)) {
      return CompareIntegralType<long int>(obj, op, s);
    } else if (objType == typeid(long long int)) {
      return CompareIntegralType<long long int>(obj, op, s);
    } else if (objType == typeid(unsigned char)) {
      return CompareIntegralType<unsigned char>(obj, op, s);
    } else if (objType == typeid(unsigned short)) {
      return CompareIntegralType<unsigned short>(obj, op, s);
    } else if (objType == typeid(unsigned int)) {
      return CompareIntegralType<unsigned int>(obj, op, s);
    } else if (objType == typeid(unsigned long int)) {
      return CompareIntegralType<unsigned long int>(obj, op, s);
    } else if (objType == typeid(unsigned long long int)) {
      return CompareIntegralType<unsigned long long int>(obj, op, s);
    } else if (objType == typeid(float)) {
      errno = 0;
      char* endptr = nullptr;
      double sFloat = strtod(s.c_str(), &endptr);
      if ((errno == ERANGE &&
           (sFloat == 0 || sFloat == HUGE_VAL || sFloat == -HUGE_VAL)) ||
          (errno != 0 && sFloat == 0) || endptr == s.c_str()) {
        return false;
      }

      double floatVal = static_cast<double>(any_cast<float>(obj));

      switch (op) {
        case LE:
          return floatVal <= sFloat;
        case GE:
          return floatVal >= sFloat;
        default: /*APPROX and EQ*/
          double diff = floatVal - sFloat;
          return (diff < std::numeric_limits<float>::epsilon()) &&
                 (diff > -std::numeric_limits<float>::epsilon());
      }
    } else if (objType == typeid(double)) {
      errno = 0;
      char* endptr = nullptr;
      double sDouble = strtod(s.c_str(), &endptr);
      if ((errno == ERANGE &&
           (sDouble == 0 || sDouble == HUGE_VAL || sDouble == -HUGE_VAL)) ||
          (errno != 0 && sDouble == 0) || endptr == s.c_str()) {
        return false;
      }

      double doubleVal = any_cast<double>(obj);

      switch (op) {
        case LE:
          return doubleVal <= sDouble;
        case GE:
          return doubleVal >= sDouble;
        default: /*APPROX and EQ*/
          double diff = doubleVal - sDouble;
          return (diff < std::numeric_limits<double>::epsilon()) &&
                 (diff > -std::numeric_limits<double>::epsilon());
      }
    } else if (objType == typeid(std::vector<Any>)) {
      const std::vector<Any>& list = ref_any_cast<std::vector<Any>>(obj);
      for (std::size_t it = 0; it != list.size(); it++) {
        if (Compare(list[it], op, s))
          return true;
      }
    }
  } catch (...) {
    // This might happen if a std::string-to-datatype conversion fails
    // Just consider it a false match and ignore the exception
  }
  return false;
}

template<typename T>
bool LDAPExpr::CompareIntegralType(const Any& obj,
                                   const int op,
                                   const std::string& s) const
{
  errno = 0;
  char* endptr = nullptr;
  long longInt = strtol(s.c_str(), &endptr, 10);
  if ((errno == ERANGE && (longInt == std::numeric_limits<long>::max() ||
                           longInt == std::numeric_limits<long>::min())) ||
      (errno != 0 && longInt == 0) || endptr == s.c_str()) {
    return false;
  }

  T sInt = static_cast<T>(longInt);
  T intVal = any_cast<T>(obj);

  switch (op) {
    case LE:
      return intVal <= sInt;
    case GE:
      return intVal >= sInt;
    default: /*APPROX and EQ*/
      return intVal == sInt;
  }
}

bool LDAPExpr::CompareString(const std::string& s1,
                             int op,
                             const std::string& s2)
{
  switch (op) {
    case LE:
      return s1.compare(s2) <= 0;
    case GE:
      return s1.compare(s2) >= 0;
    case EQ:
      return PatSubstr(s1, s2);
    case APPROX:
      return FixupString(s2) == FixupString(s1);
    default:
      return false;
  }
}

std::string LDAPExpr::FixupString(const std::string& s)
{
  std::string sb;
  sb.reserve(s.size());
  std::size_t len = s.length();
  for (std::size_t i = 0; i < len; i++) {
    char c = s.at(i);
    if (!std::isspace(c)) {
      if (std::isupper(c))
        c = std::tolower(c);
      sb.append(1, c);
    }
  }
  return sb;
}

bool LDAPExpr::PatSubstr(const std::string& s,
                         int si,
                         const std::string& pat,
                         int pi)
{
  if (pat.size() - pi == 0)
    return s.size() - si == 0;
  if (pat[pi] == LDAPExprConstants::WILDCARD()) {
    pi++;
    for (;;) {
      if (PatSubstr(s, si, pat, pi))
        return true;
      if (s.size() - si == 0)
        return false;
      si++;
    }
  } else {
    if (s.size() - si == 0) {
      return false;
    }
    if (s[si] != pat[pi]) {
      return false;
    }
    return PatSubstr(s, ++si, pat, ++pi);
  }
}

bool LDAPExpr::PatSubstr(const std::string& s, const std::string& pat)
{
  return PatSubstr(s, 0, pat, 0);
}

LDAPExpr LDAPExpr::ParseExpr(ParseState& ps)
{
  ps.skipWhite();
  if (!ps.prefix("("))
    ps.error(LDAPExprConstants::MALFORMED());

  int op;
  ps.skipWhite();
  Byte c = ps.peek();
  if (c == '&') {
    op = AND;
  } else if (c == '|') {
    op = OR;
  } else if (c == '!') {
    op = NOT;
  } else {
    return ParseSimple(ps);
  }
  ps.skip(1); // Ignore the d->m_operator

  std::vector<LDAPExpr> v;
  do {
    v.push_back(ParseExpr(ps));
    ps.skipWhite();
  } while (ps.peek() == '(');

  std::size_t n = v.size();
  if (!ps.prefix(")") || n == 0 || (op == NOT && n > 1))
    ps.error(LDAPExprConstants::MALFORMED());

  return LDAPExpr(op, v);
}

LDAPExpr LDAPExpr::ParseSimple(ParseState& ps)
{
  std::string attrName = ps.getAttributeName();
  if (attrName.empty())
    ps.error(LDAPExprConstants::MALFORMED());
  int op = 0;
  if (ps.prefix("="))
    op = EQ;
  else if (ps.prefix("<="))
    op = LE;
  else if (ps.prefix(">="))
    op = GE;
  else if (ps.prefix("~="))
    op = APPROX;
  else {
    //      System.out.println("undef op='" + ps.peek() + "'");
    ps.error(LDAPExprConstants::OPERATOR()); // Does not return
  }
  std::string attrValue = ps.getAttributeValue();
  if (!ps.prefix(")"))
    ps.error(LDAPExprConstants::MALFORMED());
  return LDAPExpr(op, attrName, attrValue);
}

const std::string LDAPExpr::ToString() const
{
  std::string res;
  res.append("(");
  if ((d->m_operator & SIMPLE) != 0) {
    res.append(d->m_attrName);
    switch (d->m_operator) {
      case EQ:
        res.append("=");
        break;
      case LE:
        res.append("<=");
        break;
      case GE:
        res.append(">=");
        break;
      case APPROX:
        res.append("~=");
        break;
    }

    for (std::size_t i = 0; i < d->m_attrValue.length(); i++) {
      Byte c = d->m_attrValue.at(i);
      if (c == '(' || c == ')' || c == '*' || c == '\\') {
        res.append(1, '\\');
      } else if (c == LDAPExprConstants::WILDCARD()) {
        c = '*';
      }
      res.append(1, c);
    }
  } else {
    switch (d->m_operator) {
      case AND:
        res.append("&");
        break;
      case OR:
        res.append("|");
        break;
      case NOT:
        res.append("!");
        break;
    }
    for (std::size_t i = 0; i < d->m_args.size(); i++) {
      res.append(d->m_args[i].ToString());
    }
  }
  res.append(")");
  return res;
}

LDAPExpr::ParseState::ParseState(const std::string& str)
  : m_pos(0)
  , m_str()
{
  if (str.empty()) {
    error(LDAPExprConstants::NULLQ());
  }

  m_str = str;
}

bool LDAPExpr::ParseState::prefix(const std::string& pre)
{
  std::string::iterator startIter = m_str.begin() + m_pos;
  if (!std::equal(pre.begin(), pre.end(), startIter))
    return false;
  m_pos += pre.size();
  return true;
}

char LDAPExpr::ParseState::peek()
{
  if (m_pos >= m_str.size()) {
    throw std::out_of_range("LDAPExpr");
  }
  return m_str.at(m_pos);
}

void LDAPExpr::ParseState::skip(int n)
{
  m_pos += n;
}

std::string LDAPExpr::ParseState::rest() const
{
  return m_str.substr(m_pos);
}

void LDAPExpr::ParseState::skipWhite()
{
  while (std::isspace(peek())) {
    m_pos++;
  }
}

std::string LDAPExpr::ParseState::getAttributeName()
{
  std::size_t start = m_pos;
  std::size_t n = 0;
  bool nIsSet = false;
  for (;; m_pos++) {
    Byte c = peek();
    if (c == '(' || c == ')' || c == '<' || c == '>' || c == '=' || c == '~') {
      break;
    } else if (!std::isspace(c)) {
      n = m_pos - start + 1;
      nIsSet = true;
    }
  }
  if (!nIsSet) {
    return std::string();
  }
  return m_str.substr(start, n);
}

std::string LDAPExpr::ParseState::getAttributeValue()
{
  std::string sb;
  bool exit = false;
  while (!exit) {
    Byte c = peek();
    switch (c) {
      case '(':
      case ')':
        exit = true;
        break;
      case '*':
        sb.append(1, LDAPExprConstants::WILDCARD());
        break;
      case '\\':
        sb.append(1, m_str.at(++m_pos));
        break;
      default:
        sb.append(1, c);
        break;
    }

    if (!exit) {
      m_pos++;
    }
  }
  return sb;
}

void LDAPExpr::ParseState::error(const std::string& m) const
{
  std::string errorStr = m + ": " + (m_str.empty() ? "" : m_str.substr(m_pos));
  throw std::invalid_argument(errorStr);
}
}
