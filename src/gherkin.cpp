#include "gherkin.h"
#include "gherkin.lex.h"
#include <codecvt>
#include <locale>
#include <stdio.h>
#include <reflex/matcher.h>
#include <boost/algorithm/string.hpp>

#ifdef USE_BOOST

#include <boost/algorithm/string.hpp>

static bool comparei(const std::wstring& a, const std::wstring& b)
{
	return boost::iequals(a, b);
}

#else//USE_BOOST

#ifdef _WINDOWS

static bool comparei(const std::wstring& a, const std::wstring& b)
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

GherkinProvider::Keywords GherkinProvider::keywords;

GherkinProvider::Keyword::Keyword(const std::string& type, const std::string& text)
	:type(type), text(text)
{
	static const std::string regex = reflex::Matcher::convert("\\w+", reflex::convert_flag::unicode);
	static const reflex::Pattern pattern(regex);
	auto matcher = reflex::Matcher(pattern, text);
	while (matcher.find() != 0) {
		words.push_back(matcher.wstr());
	}
}

GherkinKeyword* GherkinProvider::Keyword::match(GherkinLine& line)
{
	if (words.size() > line.tokens.size()) return nullptr;
	for (size_t i = 0; i < words.size(); ++i) {
		if (line.tokens[i].type != Gherkin::Operator) return nullptr;
		if (!comparei(words[i], line.tokens[i].wstr)) return nullptr;
	}
	bool toplevel = false;
	size_t keynum = words.end() - words.begin();
	for (auto& t : line.tokens) {
		if (keynum > 0) {
			t.type = Gherkin::Keyword;
			keynum--;
		}
		else {
			if (t.type == Gherkin::Colon) 
				toplevel = true;
			break;
		}
	}
	return new GherkinKeyword(*this, toplevel);
}

void GherkinProvider::setKeywords(const std::string& text)
{
	auto json = JSON::parse(text);
	keywords.clear();
	for (auto lang = json.begin(); lang != json.end(); ++lang) {
		std::string language = lang.key();
		if ((language != "en") && (language != "ru")) continue;
		auto& vector = keywords[language];
		auto& types = lang.value();
		for (auto type = types.begin(); type != types.end(); ++type) {
			auto& words = type.value();
			if (words.is_array()) {
				for (auto word = words.begin(); word != words.end(); ++word) {
					vector.push_back({ type.key(), *word });
				}
			}
		}
		std::sort(vector.begin(), vector.end(),
			[](const Keyword& a, const Keyword& b) -> bool { return a.comp(b); }
		);
	}
}

GherkinKeyword* GherkinProvider::matchKeyword(const std::string& lang, GherkinLine& line)
{
	std::string language = lang.empty() ? std::string("ru") : lang;
	for (auto& keyword : keywords[language]) {
		auto matched = keyword.match(line);
		if (matched) return matched;
	}
	return nullptr;
}

std::string GherkinProvider::ParseFile(const std::wstring& filename)
{
#ifdef _WINDOWS
	FILE* file = _wfopen(filename.c_str(), L"rb");
#else
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	FILE* file = fopen(converter.to_bytes(filename).c_str(), "rb");
#endif
	reflex::Input input = file;
	GherkinLexer lexer(input);
	lexer.lex();
	fclose(file);
	return lexer.dump();
}

GherkinKeyword::operator JSON() const
{
	JSON json;
	json["text"] = text;
	json["type"] = type;
	json["toplevel"] = toplevel;
	return json;
}

static std::string trim(const std::string& text)
{
	static const std::string regex = reflex::Matcher::convert("\\S[\\s\\S]*\\S|\\S", reflex::convert_flag::unicode);
	static const reflex::Pattern pattern(regex);
	auto matcher = reflex::Matcher(pattern, text);
	return matcher.find() ? matcher.text() : std::string();
}

GherkinToken::GherkinToken(Gherkin::TokenType t, GherkinLexer& l)
	: type(t), wstr(l.wstr()), text(l.text()), columno(l.columno())
{
	text = trim(text);
};

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
	case Gherkin::Language: return "language";
	case Gherkin::Encoding: return "encoding";
	case Gherkin::Operator: return "operator";
	case Gherkin::Comment: return "comment";
	case Gherkin::Keyword: return "keyword";
	case Gherkin::Number: return "number";
	case Gherkin::Colon: return "colon";
	case Gherkin::Param: return "param";
	case Gherkin::Table: return "table";
	case Gherkin::Cell: return "cell";
	case Gherkin::Line: return "line";
	case Gherkin::Date: return "date";
	case Gherkin::Text: return "text";
	case Gherkin::Tag: return "tag";
	case Gherkin::Symbol: return "symbol";
	case Gherkin::Multiline: return "multiline";
	default: return "none";
	}
}

GherkinLine::GherkinLine(GherkinLexer& l)
	: lineNumber(l.lineno()) {}

void GherkinLine::push(Gherkin::TokenType t, GherkinLexer& l)
{
	tokens.push_back({ t, l });
}

void GherkinLine::matchKeyword(const std::string& language)
{
	if (tokens.size() == 0) return;
	if (tokens.begin()->type != Gherkin::Operator) return;
	keyword.reset(GherkinProvider::matchKeyword(language, *this));
}

GherkinLine::operator JSON() const
{
	JSON json, js;
	for (auto& t : tokens) {
		js.push_back(t);
	}
	json["tokens"] = js;
	json["text"] = text;
	json["line"] = lineNumber;
	if (keyword) json["keyword"] = *keyword;
	return json;
}

Gherkin::TokenType GherkinLine::type() const
{
	return tokens.empty() ? Gherkin::None : tokens.begin()->type;
}

void GherkinDocument::setLanguage(GherkinLexer& lexer)
{
	if (language.empty())
		language = trim(lexer.text());
	else
		error(lexer, "Language key duplicate error");
}

void GherkinDocument::error(GherkinLexer& lexer, const std::string& error)
{
	//TODO: save error to error list
}

void GherkinDocument::push(Gherkin::TokenType t, GherkinLexer& l)
{
	if (current == nullptr) {
		lines.push_back({ l });
		current = &lines.back();
		current->text = l.matcher().line();
	}
	current->push(t, l);
	if (t == Gherkin::Language) setLanguage(l);
}

void GherkinDocument::next()
{
	if (current) current->matchKeyword(language);
	current = nullptr;
}

std::string GherkinDocument::dump() const
{
	JSON json, js;
	for (auto& line : lines) {
		js.push_back(line);
	}
	json["lines"] = js;
	json["tags"] = tags();
	return json.dump();
}

GherkinTags GherkinDocument::tags() const
{
	GherkinTags result;
	for (auto& line : lines) {
		if (line.tokens.empty()) continue;
		auto& token = *line.tokens.begin();
		if (token.type == Gherkin::Tag) {
			result.push_back(token.text);
		}
	}
	return result;
}
