%top{
  #include "gherkin.h"
%}

%class{
private:
  GherkinDocument document;
  Gherkin::TokenType current;
public:
  void next() {
    document.next();
  }
  void push(Gherkin::TokenType type) {
    document.push(type, *this);
  }
  void till(Gherkin::TokenType type) {
    current = type;
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
<INITIAL>{language}   { till(Gherkin::Language); }
<INITIAL>{encoding}   { till(Gherkin::Encoding); }
<INITIAL>{comment}    { till(Gherkin::Comment); }
<INITIAL>{tag}        { till(Gherkin::Tag); }
<INITIAL>{colon}      { push(Gherkin::Colon); }
<INITIAL>{operator}   { push(Gherkin::Operator); }
<INITIAL>{param}      { push(Gherkin::Param); }
<INITIAL>{date}       { push(Gherkin::Date); }
<INITIAL>{number}     { push(Gherkin::Number); }
<INITIAL>{table}      { push(Gherkin::Table); start(TABLE); }
<INITIAL>{str_a}      { push(Gherkin::Multiline); start(STR_A); }
<INITIAL>{str_d}      { push(Gherkin::Multiline); start(STR_D); }
<INITIAL>.            { push(Gherkin::Symbol); }

<TAIL>{new_line}      { next(); start(INITIAL); }
<TAIL>{text}          { push(current); }
<TAIL>{space}         // ignore

<TABLE>{new_line}     { next(); start(INITIAL); }
<TABLE>{table}        { push(Gherkin::Table); }
<TABLE>{cell}         { push(Gherkin::Cell); }
<TABLE>{space}        // ignore

<STR_A>{new_line}     { next(); }
<STR_A>{str_a}        { push(Gherkin::Multiline); start(INITIAL); }
<STR_A>{text}         { push(Gherkin::Line); }
<STR_A>{space}         // ignore

<STR_D>{new_line}     { next(); }
<STR_D>{str_d}        { push(Gherkin::Multiline); start(INITIAL); }
<STR_D>{text}         { push(Gherkin::Line); }
<STR_D>{space}         // ignore

<<EOF>>               { next(); return 0; }

%%
