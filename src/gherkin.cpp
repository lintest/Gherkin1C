#include "gherkin.h"
#include "gherkin.lex.h"
#include <reflex/matcher.h>

std::vector<GherkinKeword> GherkinProvider::keywords;

void GherkinProvider::setKeywords(const std::string& text)
{
	auto json = JSON::parse(text);
	keywords.clear();
	for (auto lang = json.begin(); lang != json.end(); ++lang) {
		auto& types = lang.value();
		for (auto type = types.begin(); type != types.end(); ++type) {
			auto& words = type.value();
			if (words.is_array()) {
				for (auto word = words.begin(); word != words.end(); ++word) {
					keywords.push_back({ lang.key(), type.key(), *word });
				}
			}
		}
	}
	std::sort(keywords.begin(), keywords.end(),
		[](const GherkinKeword& a, const GherkinKeword& b) -> bool {
			return a.words.size() > b.words.size();
		}
	);
}

GherkinKeword::GherkinKeword(const std::string& lang, const std::string& type, const std::string& word)
	: lang(lang), type(type)
{
	static const std::string regex = reflex::Matcher::convert("\\w+", reflex::convert_flag::unicode);
	static const reflex::Pattern pattern(regex);
	auto matcher = reflex::Matcher(pattern, word);
	while (matcher.find() != 0) {
		words.push_back(matcher.wstr());
	}
	text = word;
}

GherkinToken::GherkinToken(Gherkin::TokenType t, GherkinLexer& l)
	: type(t), wstr(l.wstr()), text(l.text()), columno(l.columno()) {};

GherkinToken::operator JSON() const
{
	JSON json;
	json["type"] = type2str();
	json["text"] = text;
	return json;
}

std::string GherkinToken::type2str() const
{
	switch (type) {
	case Gherkin::Operator: return "operator";
	case Gherkin::Comment: return "comment";
	case Gherkin::Number: return "number";
	case Gherkin::Colon: return "colon";
	case Gherkin::Param: return "param";
	case Gherkin::Date: return "date";
	case Gherkin::Tag: return "tag";
	case Gherkin::Symbol: return "symbol";
	default: return "none";
	}
}

void GherkinLine::push(Gherkin::TokenType t, GherkinLexer& l)
{
	tokens.push_back({ t, l });
}

GherkinLine::operator JSON() const
{
	JSON json, js;
	for (auto& t : tokens) {
		js.push_back(t);
	}
	json["text"] = text;
	json["tokens"] = js;
	return json;
}

void GherkinDocument::push(Gherkin::TokenType t, GherkinLexer& l)
{
	if (current == nullptr) {
		lines.push_back({});
		current = &lines.back();
		current->text = l.matcher().line();
	}
	current->push(t, l);
}

GherkinDocument::operator JSON() const
{
	JSON json;
	for (auto& line : lines) {
		json.push_back(line);
	}
	return json;
}
