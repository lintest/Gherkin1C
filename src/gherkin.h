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
  GherkinToken(Gherkin::TokenType type, GherkinLexer& l);
  operator JSON() const;
};

#endif//GHERKIN_H
