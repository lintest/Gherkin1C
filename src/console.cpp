#include "gherkin.lex.h"
#include <fstream>
#include <string>

int main(int argc, char** argv)
{
	reflex::Input input;
	if (argc > 1) {
		input = fopen(argv[1], "r");
		if (input.file() == NULL) {
			perror("Cannot open file for reading");
			exit(EXIT_FAILURE);
		}
	}
	else {
		input = stdin;
	}

	GherkinProvider provider;

	if (argc > 2) {
		std::ifstream fstream;
		fstream.open(argv[2]);
		std::string json((std::istreambuf_iterator<char>(fstream)), std::istreambuf_iterator<char>());
		provider.setKeywords(json);
	}
	else {
		std::string json = R"({"en": {"simple": ["Then", "And", "Scenario Template"]}})";
		provider.setKeywords(json);
	}

	GherkinDocument doc(provider);
	GherkinLexer lexer(input);
	lexer.init(&doc);
	lexer.lex();
	std::cout << JSON(doc).dump();

	if (input.file() != stdin)
		fclose(input.file());

	return 0;
}
