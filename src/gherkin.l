%top{
  #include "gherkin.h"
  using namespace Gherkin;
%}

%class{
private:
  TokenType current_token;
  GherkinDocument document;
public:
  void next() {
    document.next();
  }
  void push(TokenType type) {
    document.push(type, *this);
  }
  void till(TokenType type) {
    current_token = type;
    start(TAIL);
  }
  GherkinTags tags() const {
    return document.tags();
  }
  std::string dump() const {
    return document.dump();
  }
}

%option fast nodefault unicode
%xstate TAG
%xstate TAIL
%xstate TABLE
%xstate STR_A
%xstate STR_D

new_line      \r?\n
language      #\s*(?i)(language)\s*:\s*
encoding      #\s*(?i)(encoding)\s*:\s*
operator      \w+
number        -?(\\d*\\.)?\\d+
date          \d+\.\d+\.\d+|\d+\/\d+\/\d+
param         \"[^\"\n\r]*\"|'[^'\n\r]*'|<[^>\n\r]*>
comment       #|\/\/
space         \h+
colon         :
table         \|
cell          [^\s][^\n\r\|]*
text          [^\s][^\r\n]*
str_a         \`\`\`[^\n^r]*
str_d         \"\"\"[^\n^r]*
tag           @

%%

<INITIAL>{new_line}   { next(); start(INITIAL); }
<INITIAL>{space}      // ignore
<INITIAL>{language}   { till(TokenType::Language); }
<INITIAL>{encoding}   { till(TokenType::Encoding); }
<INITIAL>{comment}    { till(TokenType::Comment); }
<INITIAL>{tag}        { till(TokenType::Tag); }
<INITIAL>{colon}      { push(TokenType::Colon); }
<INITIAL>{operator}   { push(TokenType::Operator); }
<INITIAL>{param}      { push(TokenType::Param); }
<INITIAL>{date}       { push(TokenType::Date); }
<INITIAL>{number}     { push(TokenType::Number); }
<INITIAL>{table}      { push(TokenType::Table); start(TABLE); }
<INITIAL>{str_a}      { push(TokenType::Multiline); start(STR_A); }
<INITIAL>{str_d}      { push(TokenType::Multiline); start(STR_D); }
<INITIAL>.            { push(TokenType::Symbol); }

<TAIL>{new_line}      { next(); start(INITIAL); }
<TAIL>{text}          { push(current_token); }
<TAIL>{space}         // ignore

<TABLE>{new_line}     { next(); start(INITIAL); }
<TABLE>{table}        { push(TokenType::Table); }
<TABLE>{cell}         { push(TokenType::Cell); }
<TABLE>{space}        // ignore

<STR_A>{new_line}     { next(); }
<STR_A>{str_a}        { push(TokenType::Multiline); start(INITIAL); }
<STR_A>{text}         { push(TokenType::Line); }
<STR_A>{space}         // ignore

<STR_D>{new_line}     { next(); }
<STR_D>{str_d}        { push(TokenType::Multiline); start(INITIAL); }
<STR_D>{text}         { push(TokenType::Line); }
<STR_D>{space}         // ignore

<<EOF>>               { next(); return 0; }

%%
