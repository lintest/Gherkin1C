#ifndef GHERKIN_H
#define GHERKIN_H

#include <vector>
#include <string>
#include "json.hpp"

using JSON = nlohmann::json;

namespace Gherkin {
    enum TokenType {
        Operator,
        Comment,
        Number,
        Colon,
        Param,
        Date,
        Tag,
        Symbol
    };
}

class GherkinLexer;

class GherkinToken {
private:
  std::string type2str() const;
public:
  Gherkin::TokenType type;
  std::string text;
  size_t columno;
public:
  GherkinToken(Gherkin::TokenType t, GherkinLexer& l);
  operator JSON() const;
};

class GherkinLine {
private:
  friend class GherkinDocument;
  std::vector<GherkinToken> tokens;
  std::string text;
public:
  GherkinLine() {}
  operator JSON() const;
  void push(Gherkin::TokenType t, GherkinLexer& l);
};

class GherkinDocument {
private:
  std::vector<GherkinLine> lines;
  GherkinLine* current = nullptr;
  std::string text;
public:
  GherkinDocument() {}
  operator JSON() const;
  void next() { current = nullptr; }
  void push(Gherkin::TokenType t, GherkinLexer& l);
};

#endif//GHERKIN_H
