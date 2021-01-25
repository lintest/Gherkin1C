#ifndef GHERKIN_H
#define GHERKIN_H

#include <vector>
#include <string>
#include "json.hpp"

using JSON = nlohmann::json;

class GherkinLexer;

class GherkinToken {
public:
  std::string type;
  std::string text;
  size_t columno;
public:
  GherkinToken(const std::string& type, GherkinLexer& l);
};

#endif//GHERKIN_H
