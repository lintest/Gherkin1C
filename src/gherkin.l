%top{
  #include "gherkin.h"
%}

%class{
private:
  GherkinDocument document;
public:
  void next() {
    document.next();
  }
  void push(Gherkin::TokenType type) {
    document.push(type, *this);
  }
  std::string dump() {
    return document.dump();
  }
}

%option fast nodefault unicode

new_line      \r?\n
operator      \w+
number        -?(\\d*\\.)?\\d+
date          \d+\.\d+\.\d+|\d+\/\d+\/\d+
param         \"[^\"\n]*\"|'[^'\n]*'|<[^>\n]*>
comment       #[^\n]*|\/\/[^\n]*
space         \h+
colon         :
tag           @

%%

{new_line}   { next(); }
{space}      // ignore
{tag}        { push(Gherkin::Tag); }
{colon}      { push(Gherkin::Colon); }
{operator}   { push(Gherkin::Operator); }
{date}       { push(Gherkin::Date); }
{param}      { push(Gherkin::Param); }
{number}     { push(Gherkin::Number); }
{comment}    { push(Gherkin::Comment); }
.            { push(Gherkin::Symbol); }

%%
