#include "gherkin.h"
#include "gherkin.lex.h"
#include <reflex/matcher.h>

std::vector<GherkinKeword> GherkinProvider::keywords;

void GherkinProvider::setKeywords(const std::string& text)
{
	auto json = JSON::parse(text);
	keywords.clear();
	for (auto lang = json.begin(); lang != json.end(); ++lang) {
		std::string language = lang.key();
		if ((language != "en") && (language != "ru")) continue;
		auto& types = lang.value();
		for (auto type = types.begin(); type != types.end(); ++type) {
			auto& words = type.value();
			if (words.is_array()) {
				for (auto word = words.begin(); word != words.end(); ++word) {
					keywords.push_back({ language, type.key(), *word });
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

#ifdef USE_BOOST

#include <boost/algorithm/string.hpp>

static bool comparei(const std::wstring &a, const std::wstring &b)
{
	return boost::iequals(a, b);
}

#else//USE_BOOST

#ifdef _WINDOWS

static bool comparei(const std::wstring &a, const std::wstring &b)
{
	static _locale_t locale = _create_locale(LC_ALL, "ru-RU");
	auto res = _wcsnicmp_l(a.c_str(), b.c_str(), std::max(a.size(), b.size()), locale);
	return res == 0;
}

#include <string.h>

#else//_WINDOWS

static bool comparei(std::wstring a, std::wstring b)
{
	transform(a.begin(), a.end(), a.begin(), toupper);
	transform(b.begin(), b.end(), b.begin(), toupper);
	return (a == b);
}

#endif//_WINDOWS

#endif//USE_BOOST

GherkinKeword* GherkinProvider::matchKeyword(const GherkinLine& line)
{
	for (auto& keyword : keywords) {
		auto matched = keyword.matchKeyword(line);
		if (matched) return matched;
	}
	return nullptr;
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

GherkinKeword* GherkinKeword::matchKeyword(const GherkinLine& line)
{
	if (words.size() > line.tokens.size()) return nullptr;
	for (size_t i = 0; i < words.size(); ++i) {
		if (line.tokens[i].type != Gherkin::Operator) return nullptr;
		if (!comparei(words[i], line.tokens[i].wstr)) return nullptr;
	}
	return this;
}

GherkinKeword::operator JSON() const
{
	JSON json;
	json["lang"] = lang;
	json["text"] = text;
	json["type"] = type;
	return json;
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
	auto keyword = GherkinProvider::matchKeyword(*this);
	if (keyword) json["keyword"] = *keyword;
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
