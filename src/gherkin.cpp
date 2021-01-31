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

static std::string trim(const std::string& text)
{
	static const std::string regex = reflex::Matcher::convert("\\S[\\s\\S]*\\S|\\S", reflex::convert_flag::unicode);
	static const reflex::Pattern pattern(regex);
	auto matcher = reflex::Matcher(pattern, text);
	return matcher.find() ? matcher.text() : std::string();
}

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

	GherkinKeyword* GherkinProvider::Keyword::match(GherkinTokens& tokens)
	{
		if (words.size() > tokens.size()) 
			return nullptr;

		for (size_t i = 0; i < words.size(); ++i) {
			if (tokens[i].type != TokenType::Operator) return nullptr;
			if (!comparei(words[i], tokens[i].wstr)) return nullptr;
		}
		bool toplevel = false;
		size_t keynum = words.end() - words.begin();
		for (auto& t : tokens) {
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
			if ((language != "en") && (language != "ru")) continue; // TODO: remove this line
			auto& vector = keywords[language];
			auto& types = lang.value();
			for (auto type = types.begin(); type != types.end(); ++type) {
				KeywordType t = GherkinKeyword::str2type(type.key());
				auto& words = type.value();
				if (words.is_array()) {
					for (auto word = words.begin(); word != words.end(); ++word) {
						std::string text = trim(*word);
						if (text == "*") continue;
						vector.push_back({ t, *word });
					}
				}
			}
			std::sort(vector.begin(), vector.end(),
				[](const Keyword& a, const Keyword& b) -> bool { return a.comp(b); }
			);
		}
	}

	GherkinKeyword* GherkinProvider::matchKeyword(const std::string& lang, GherkinTokens& tokens)
	{
		std::string language = lang.empty() ? std::string("ru") : lang;
		for (auto& keyword : keywords[language]) {
			auto matched = keyword.match(tokens);
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

	KeywordType GherkinKeyword::str2type(const std::string& text)
	{
		std::string type = text;
		transform(type.begin(), type.end(), type.begin(), tolower);
		static std::map<std::string, KeywordType> types{
			{ "and", KeywordType::And},
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
		case TokenType::Language: return "Language";
		case TokenType::Encoding: return "Encoding";
		case TokenType::Asterisk: return "Asterisk";
		case TokenType::Operator: return "Operator";
		case TokenType::Comment: return "Comment";
		case TokenType::Keyword: return "Keyword";
		case TokenType::Number: return "Number";
		case TokenType::Colon: return "Colon";
		case TokenType::Param: return "Param";
		case TokenType::Table: return "Table";
		case TokenType::Cell: return "Cell";
		case TokenType::Line: return "Line";
		case TokenType::Date: return "Date";
		case TokenType::Text: return "Text";
		case TokenType::Tag: return "Tag";
		case TokenType::Symbol: return "Symbol";
		case TokenType::Multiline: return "Multiline";
		default: return "None";
		}
	}

	GherkinLine::GherkinLine(GherkinLexer& l)
		: lineNumber(l.lineno()), text(l.matcher().line())
	{
	}

	GherkinLine::GherkinLine(size_t lineNumber)
		: lineNumber(lineNumber)
	{
	}

	void GherkinLine::push(TokenType t, GherkinLexer& l)
	{
		tokens.push_back({ t, l });
	}

	GherkinKeyword* GherkinLine::matchKeyword(GherkinDocument& document)
	{
		if (tokens.size() == 0) return nullptr;
		if (tokens.begin()->type != TokenType::Operator) return nullptr;
		keyword.reset(GherkinProvider::matchKeyword(document.getLanguage(), tokens));
		//TODO: check does colon exists for top level keywords: Feature, Background, Scenario...
		return keyword.get();
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

	Gherkin::TokenType GherkinLine::getType() const
	{
		return tokens.empty() ? TokenType::None : tokens.begin()->type;
	}

	int GherkinLine::getIndent() const
	{
		int indent = 0;
		const int tabSize = 4;
		for (auto ch : text) {
			switch (ch) {
			case ' ':
				indent++;
				break;
			case '\t':
				indent = indent + tabSize - (indent % tabSize);
				break;
			default:
				return indent;
			}
		}
		return INT_MAX;
	}

	GherkinTable::GherkinTable(const GherkinLine& line)
	{
		for (auto& token : line.getTokens()) {
		}
	}
	
	void GherkinTable::push(const GherkinLine& line)
	{
	}

	GherkinTable::operator JSON() const
	{
		JSON json;
		return json;
	}

	GherkinElement::GherkinElement(GherkinDocument& document, const GherkinLine& line)
		: lineNumber(line.getLineNumber())
	{
		comments = std::move(document.commentStack);
		tags = std::move(document.tagStack);
	}

	GherkinElement::~GherkinElement()
	{
		for (auto ptr : items) {
			free(ptr);
		}
		items.clear();
	}

	void GherkinElement::push(GherkinElement* item)
	{
		items.push_back(item);
	}

	GherkinDefinition::GherkinDefinition(GherkinDocument& document, const GherkinLine& line)
		: GherkinElement(document, line), keyword(*line.getKeyword())
	{
	}

	GherkinDefinition::operator JSON() const
	{
		JSON json;
		return json;
	}

	GherkinStep::GherkinStep(GherkinDocument& document, const GherkinLine& line)
		: GherkinElement(document, line), keyword(*line.getKeyword())
	{
	}

	GherkinStep::operator JSON() const
	{
		JSON json;
		return json;
	}

	GherkinGroup::GherkinGroup(GherkinDocument& document, const GherkinLine& line)
		: GherkinElement(document, line), text(trim(line.getText()))
	{
	}

	GherkinGroup::operator JSON() const
	{
		JSON json;
		return json;
	}

	void GherkinDocument::setLanguage(GherkinLexer& lexer)
	{
		if (language.empty())
			language = trim(lexer.text());
		else
			error(lexer, "Language key duplicate error");
	}

	void GherkinDocument::resetElementStack(GherkinElement& element)
	{
		lastElement = &element;
		elementStack.clear();
		elementStack.emplace_back(-1, &element);
	}

	void GherkinDocument::setDefinition(std::unique_ptr<GherkinDefinition>& def, GherkinLine& line)
	{
		if (def) {
			std::string type = GherkinKeyword::type2str(line.getKeyword()->type);
			error(line, type + " keyword duplicate error");
		}
		else {
			auto definition = new GherkinDefinition(*this, line);
			feature.reset(definition);
			resetElementStack(*definition);
		}
	}

	void GherkinDocument::addScenarioDefinition(GherkinLine& line)
	{
		scenarios.emplace_back(*this, line);
		auto definition = &scenarios.back();
		resetElementStack(*definition);
	}

	void GherkinDocument::error(GherkinLexer& lexer, const std::string& error)
	{
		//TODO: save error to error list
	}

	void GherkinDocument::error(GherkinLine& line, const std::string& error)
	{
		//TODO: save error to error list
	}

	void GherkinDocument::push(TokenType t, GherkinLexer& l)
	{
		if (currentLine == nullptr) {
			lines.push_back({ l });
			currentLine = &lines.back();
		}
		currentLine->push(t, l);
		switch (t) {
		case TokenType::Language:
			setLanguage(l);
			break;
		case TokenType::Comment:
			commentStack.push_back(l.text());
			break;
		case TokenType::Tag:
			tagStack.push_back(l.text());
			break;
		}
	}

	void GherkinDocument::addElement(GherkinLine& line)
	{
		std::unique_ptr<GherkinElement> element;
		switch (currentLine->getType()) {
		case TokenType::Keyword:
			element.reset(new GherkinStep(*this, line));
			break;
		case TokenType::Asterisk:
		case TokenType::Operator:
		case TokenType::Symbol:
			element.reset(new GherkinGroup(*this, line));
			break;
		case TokenType::Multiline:
			//TODO: add multy line
			break;
		case TokenType::Table:
			//TODO: add table
			break;
		default:
			break;
		}
		if (element) {
			auto indent = line.getIndent();
			while (!elementStack.empty()) {
				if (elementStack.back().first < indent) break;
				elementStack.pop_back();
			}
			if (elementStack.empty()) {
				throw u"Element statck is empty";
			}
			elementStack.emplace_back(indent, element.get());
			elementStack.back().second->push(element.release());
		}
	}

	void GherkinDocument::processLine(GherkinLine& line)
	{
		if (line.getType() != TokenType::Table)
			currentTable == nullptr;

		auto keyword = line.matchKeyword(*this);
		if (keyword) {
			switch (keyword->type) {
			case KeywordType::Feature:
				setDefinition(feature, line);
				break;
			case KeywordType::ScenarioOutline:
				setDefinition(outline, line);
				break;
			case KeywordType::Background:
				setDefinition(backround, line);
				break;
			case KeywordType::Scenario:
				addScenarioDefinition(line);
				break;
			default:
				addElement(line);
			}
		}
		else {
			switch (line.getType()) {
			case TokenType::Asterisk:
			case TokenType::Operator:
			case TokenType::Symbol:
			case TokenType::Multiline:
			case TokenType::Table:
				addElement(line);
				break;
			}
			//TODO: add tables and multiline string to lastElement
		}
	}

	void GherkinDocument::next(GherkinLexer& l)
	{
		if (currentLine) {
			processLine(*currentLine);
			currentLine = nullptr;
		}
		else {
			auto lineNumber = l.lineno();
			if (lineNumber > 1) {
				lines.push_back({ lineNumber });
				processLine(lines.back());
			}
			return;
		}
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
		return feature ? feature->tags : GherkinTags();
	}
}