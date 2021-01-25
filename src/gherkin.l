%top{
  #include "gherkin.h"
%}

%class{
private:
  std::vector<std::vector<GherkinToken>> lines{{}};
public:
  void push(Gherkin::TokenType type) {
    lines.back().push_back( {type, *this} );
  }
  void next() {
    lines.push_back( {} );
  }
  std::string dump() {
    JSON json;
    for (auto& line : lines) {
      JSON js;
      for (auto& token : line) {
        js.push_back(token);
      }
      json.push_back(js);
    }
    return json.dump();
  }
}

%option fast nodefault unicode

new_line      \r?\n
operator      [\w]+
number        -?(\\d*\\.)?\\d+
date          \d+\.\d+\.\d+|\d+\/\d+\/\d+
param         \"[^\"\n]*\"|'[^'\n]*'|<[^>\n]*>
comment       #[^\n]*|\/\/[^\n]*
space         \h+
colon         [:]

%%

{new_line}   { next(); }
{space}      // ignore
{colon}      { push(Gherkin::Colon); }
{operator}   { push(Gherkin::Operator); }
{date}       { push(Gherkin::Date); }
{param}      { push(Gherkin::Param); }
{number}     { push(Gherkin::Number); }
{comment}    { push(Gherkin::Comment); }
.            { push(Gherkin::Symbol); }

%%
