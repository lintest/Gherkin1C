%top{
  #include "gherkin.h"
%}

%class{
private:
  std::vector<std::vector<GherkinToken>> lines{{}};
public:
  void push(const std::string& type) {
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
        JSON j;
        j["type"] = token.type;
        j["text"] = token.text;
        js.push_back(j);
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
param         \"[^\"]*\"|'[^']*'|<[^>]*>
comment       #[^\n]*|\/\/[^\n]*
space         \h+
colon         [:]

%%

{new_line}   { std::wcout << lineno() << "================\n"; next(); }
{space}      // ignore
{colon}      { std::wcout << "Colon: " << text() << "\n"; push("colon"); }
{operator}   { std::wcout << "Text: " << text() << "\n"; push("operator"); }
{date}       { std::wcout << "Date: " << text() << "\n"; push("date"); }
{param}      { std::wcout << "Param: " << text() << "\n"; push("param"); }
{number}     { std::wcout << "Number: " << text() << "\n"; push("number"); }
{comment}    { std::wcout << "Comment: " << text() << "\n"; push("comment"); }
.            { std::wcout << "Symbol: " << text() << "\n"; push("symbol"); }

%%

int main(int argc, char **argv)
{
  reflex::Input input;
  if (argc > 1) {
    input = fopen(argv[1], "r");
    if (input.file() == NULL) {
      perror("Cannot open file for reading");
      exit(EXIT_FAILURE);
    }
  } else {
    input = stdin;
  }

  GherkinLexer lexer(input);
  lexer.lex();
  std::cout << lexer.dump();

  if (input.file() != stdin)
    fclose(input.file());

  return 0;
}
