#include "GherkinParser.h"
#include "gherkin.lex.h"
#include <fstream>
#include <stdio.h>

std::vector<std::u16string> GherkinParser::names = {
	AddComponent(u"GherkinParser", []() { return new GherkinParser; }),
};

GherkinParser::GherkinParser()
{
	AddProperty(u"Keywords", u"КлючевыеСлова", nullptr, [&](VH value) { GherkinProvider::setKeywords(value); });
	AddFunction(u"ParseFile", u"ПрочитатьФайл", [&](VH filename) {  this->result = this->ParseFile(filename); });
}

std::string GherkinParser::ParseFile(const std::wstring& filename)
{
	std::ifstream istream(filename);
	reflex::Input input = istream;
	GherkinLexer lexer(input);
	lexer.lex();
	return lexer.dump();
}
