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

namespace Gherkin {

	GherkinProvider::Keywords GherkinProvider::keywords;

	GherkinProvider::Keyword::Keyword(KeywordType type, const std::string& text)
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
			if (line.tokens[i].type != TokenType::Operator) return nullptr;
			if (!comparei(words[i], line.tokens[i].wstr)) return nullptr;
		}
		bool toplevel = false;
		size_t keynum = words.end() - words.begin();
		for (auto& t : line.tokens) {
			if (keynum > 0) {
				t.type = TokenType::Keyword;
				keynum--;
			}
			else {
				if (t.type == TokenType::Colon)
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
				KeywordType t = GherkinKeyword::str2type(type.key());
				auto& words = type.value();
				if (words.is_array()) {
					for (auto word = words.begin(); word != words.end(); ++word) {
						vector.push_back({ t, *word });
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
#else//_WINDOWS
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		FILE* file = fopen(converter.to_bytes(filename).c_str(), "rb");
#endif//_WINDOWS
		reflex::Input input = file;
		GherkinLexer lexer(input);
		lexer.lex();
		fclose(file);
		return lexer.dump();
	}

	KeywordType GherkinKeyword::str2type(const std::string &text)
	{
		std::string type = text;
		transform(type.begin(), type.end(), type.begin(), tolower);
		static std::map<std::string, KeywordType> types{
			{"and", KeywordType::And},
			{ "background", KeywordType::Background },
			{ "but", KeywordType::But },
			{ "examples", KeywordType::Examples },
			{ "feature", KeywordType::Feature },
			{ "given", KeywordType::Given },
			{ "rule", KeywordType::Rule },
			{ "scenario", KeywordType::Scenario },
			{ "scenariooutline", KeywordType::ScenarioOutline },
			{ "then", KeywordType::Then },
			{ "when", KeywordType::When },
		};
		return types.count(type) ? types[type] : KeywordType::None;
	}

	std::string GherkinKeyword::type2str(KeywordType type)
	{
		switch (type) {
		case KeywordType::And: return "And";
		case KeywordType::Background: return "Background";
		case KeywordType::But: return "But";
		case KeywordType::Examples: return "Then";
		case KeywordType::Feature: return "Feature";
		case KeywordType::Given: return "Given";
		case KeywordType::Scenario: return "Scenario";
		case KeywordType::ScenarioOutline: return "ScenarioOutline";
		case KeywordType::Rule: return "Rule";
		case KeywordType::Then: return "Then";
		case KeywordType::When: return "When";
		default: return {};
		}
	}

	GherkinKeyword::operator JSON() const
	{
		JSON json;
		json["text"] = text;
		json["type"] = type2str(type);
		if (toplevel) json["toplevel"] = toplevel;
		return json;
	}

	static std::string trim(const std::string& text)
	{
		static const std::string regex = reflex::Matcher::convert("\\S[\\s\\S]*\\S|\\S", reflex::convert_flag::unicode);
		static const reflex::Pattern pattern(regex);
		auto matcher = reflex::Matcher(pattern, text);
		return matcher.find() ? matcher.text() : std::string();
	}

	GherkinToken::GherkinToken(TokenType t, GherkinLexer& l)
		: type(t), wstr(l.wstr()), text(l.text()), column(l.columno())
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
		case TokenType::Language: return "language";
		case TokenType::Encoding: return "encoding";
		case TokenType::Operator: return "operator";
		case TokenType::Comment: return "comment";
		case TokenType::Keyword: return "keyword";
		case TokenType::Number: return "number";
		case TokenType::Colon: return "colon";
		case TokenType::Param: return "param";
		case TokenType::Table: return "table";
		case TokenType::Cell: return "cell";
		case TokenType::Line: return "line";
		case TokenType::Date: return "date";
		case TokenType::Text: return "text";
		case TokenType::Tag: return "tag";
		case TokenType::Symbol: return "symbol";
		case TokenType::Multiline: return "multiline";
		default: return "none";
		}
	}

	GherkinLine::GherkinLine(GherkinLexer& l)
		: lineNumber(l.lineno()) {}

	void GherkinLine::push(TokenType t, GherkinLexer& l)
	{
		tokens.push_back({ t, l });
	}

	void GherkinLine::matchKeyword(const std::string& language)
	{
		if (tokens.size() == 0) return;
		if (tokens.begin()->type != TokenType::Operator) return;
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
		return tokens.empty() ? TokenType::None : tokens.begin()->type;
	}

	GherkinDefinition::GherkinDefinition(GherkinDocument& document, const GherkinLine& source)
		: keyword(*source.keyword)
	{
		comments = std::move(document.comment_stack);
		tags = std::move(document.tag_stack);
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

	void GherkinDocument::push(TokenType t, GherkinLexer& l)
	{
		if (current == nullptr) {
			lines.push_back({ l });
			current = &lines.back();
			current->text = l.matcher().line();
		}
		current->push(t, l);
		if (t == TokenType::Language) setLanguage(l);
	}

	void GherkinDocument::next()
	{
		if (current) {
			current->matchKeyword(language);
		}
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
			if (token.type == TokenType::Tag) {
				result.push_back(token.text);
			}
		}
		return result;
	}
}