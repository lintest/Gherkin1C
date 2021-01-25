#include "lex.yy.h"

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
