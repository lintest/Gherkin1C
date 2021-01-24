%option fast nodefault unicode

new_line      \r?\n
operator      [\w]+
number        -?(\\d*\\.)?\\d+
date          \d+\.\d+\.\d+|\d+\/\d+\/\d+
param         \"[^\"]*\"|'[^']*'|<[^>]*>
comment       #[^\n]*|\/\/[^\n]*
space         [\s]
colon         [:]

%%

{new_line}   { std::wcout << lineno() << "================\n"; }
{space}      // ignore
{colon}      { std::wcout << "Colon: " << text() << "\n"; }
{operator}   { std::wcout << "Text: " << text() << "\n"; }
{date}       { std::wcout << "Date: " << text() << "\n"; }
{param}      { std::wcout << "Param: " << text() << "\n"; }
{number}     { std::wcout << "Number: " << text() << "\n"; }
{comment}    { std::wcout << "Comment: " << text() << "\n"; }
.            { std::wcout << "Symbol: " << text() << "\n"; }

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

  Lexer(input).lex();

  if (input.file() != stdin)
    fclose(input.file());

  return 0;
}
