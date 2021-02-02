#include "gherkin.h"
#include "gherkin.lex.h"
#include <stdafx.h>
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

	static std::string trim(const std::string& text)
	{
		static const std::string regex = reflex::Matcher::convert("\\S[\\s\\S]*\\S|\\S", reflex::convert_flag::unicode);
		static const reflex::Pattern pattern(regex);
		auto matcher = reflex::Matcher(pattern, text);
		return matcher.find() ? matcher.text() : std::string();
	}

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

	GherkinKeyword* GherkinProvider::Keyword::match(GherkinTokens& tokens) const
	{
		if (words.size() > tokens.size())
			return nullptr;

		for (size_t i = 0; i < words.size(); ++i) {
			if (tokens[i].type != TokenType::Operator)
				return nullptr;

			if (!comparei(words[i], tokens[i].wstr))
				return nullptr;
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

	GherkinKeyword* GherkinProvider::matchKeyword(const std::string& lang, GherkinTokens& tokens) const
	{
		std::string language = lang.empty() ? std::string("ru") : lang;
		for (auto& keyword : keywords.at(language)) {
			auto matched = keyword.match(tokens);
			if (matched) return matched;
		}
		return nullptr;
	}

	std::string GherkinProvider::ParseFile(const std::wstring& filename) const
	{
#ifdef _WINDOWS
		std::unique_ptr<FILE, decltype(&fclose)> file(_wfopen(filename.c_str(), L"rb"), &fclose);
#else//_WINDOWS
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::string str = converter.to_bytes(filename);
		std::unique_ptr<FILE, decltype(&fclose)> file(fopen(str.c_str(), "rb"), &fclose);
#endif//_WINDOWS
		reflex::Input input(file.get());
		GherkinDocument doc(*this);
		GherkinLexer lexer(input);
		lexer.init(&doc);
		lexer.lex();
		return JSON(doc);
	}

	std::string GherkinProvider::ParseText(const std::string& text) const
	{
		reflex::Input input(text);
		GherkinDocument doc(*this);
		GherkinLexer lexer(input);
		lexer.init(&doc);
		lexer.lex();
		return JSON(doc);
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

	GherkinToken::GherkinToken(GherkinLexer& lexer, TokenType type, char ch)
		: type(type), wstr(lexer.wstr()), text(lexer.text()), column(lexer.columno()), symbol(ch)
	{
		if (type == TokenType::Param && ch != 0) {
			bool escaping = false;
			std::wstringstream ss;
			for (auto it = wstr.begin(); it != wstr.end(); ++it) {
				if (it == wstr.begin() || (it + 1) == wstr.end())
					continue;

				if (escaping) {
					escaping = false;
					wchar_t wc = *it;
					switch (wc) {
					case L't': wc = L'\t'; break;
					case L'n': wc = L'\n'; break;
					case L'r': wc = L'\r'; break;
					}
					ss << wc;
				}
				else {
					if (*it == L'\\')
						escaping = true;
					else
						ss << *it;
				}
			}
			wstr = ss.str();
			text = WC2MB(wstr);
		}
		else {
			text = trim(text);
		}
	};

	GherkinToken::operator JSON() const
	{
		JSON json;
		json["text"] = text;
		json["column"] = column;
		json["type"] = type2str();

		if (symbol != 0) 
			json["symbol"] = std::string(1, symbol);

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
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		wstr = converter.from_bytes(text);
	}

	GherkinLine::GherkinLine(size_t lineNumber)
		: lineNumber(lineNumber)
	{
	}

	void GherkinLine::push(GherkinLexer& lexer, TokenType type, char ch)
	{
		tokens.emplace_back( lexer, type, ch );
	}

	GherkinKeyword* GherkinLine::matchKeyword(GherkinDocument& document)
	{
		if (tokens.size() == 0) return nullptr;
		if (tokens.begin()->type != TokenType::Operator) return nullptr;
		keyword.reset(document.matchKeyword(tokens));
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
		: lineNumber(line.getLineNumber())
	{
		for (auto& token : line.getTokens()) {
			if (token.getType() == TokenType::Cell)
				head.emplace_back(token.getText());
		}
	}

	void GherkinTable::push(const GherkinLine& line)
	{
		body.push_back({});
		auto& row = body.back();
		for (auto& token : line.getTokens()) {
			if (token.getType() == TokenType::Cell)
				row.emplace_back(token.getText());
		}
	}

	GherkinTable::operator JSON() const
	{
		JSON json;
		json["line"] = lineNumber;
		json["head"] = head;
		json["body"] = body;
		return json;
	}

	GherkinElement::GherkinElement(GherkinDocument& document, const GherkinLine& line)
		: wstr(line.getWstr()), text(line.getText()), lineNumber(line.getLineNumber())
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

	GherkinElement* GherkinElement::push(GherkinDocument& document, const GherkinLine& line)
	{
		GherkinElement* element = nullptr;
		switch (line.getType()) {
		case TokenType::Keyword:
			element = new GherkinStep(document, line);
			break;
		case TokenType::Asterisk:
		case TokenType::Operator:
		case TokenType::Symbol:
			element = new GherkinGroup(document, line);
			break;
		default:
			return nullptr;
		}
		items.push_back(element);
		return element;
	}

	GherkinTable* GherkinElement::pushTable(const GherkinLine& line)
	{
		tables.emplace_back(line);
		return &tables.back();
	}

	GherkinElement::operator JSON() const
	{
		JSON json;

		json["line"] = lineNumber;
		json["text"] = text;

		if (!items.empty()) {
			JSON js;
			for (auto item : items)
				js.push_back(JSON(*item));
			json["items"] = js;
		}

		if (!tags.empty())
			json["tags"] = tags;

		if (!comments.empty())
			json["comments"] = comments;

		if (!tables.empty())
			json["tables"] = tables;

		return json;
	}

	GherkinFeature::GherkinFeature(GherkinDocument& document, const GherkinLine& line)
		: GherkinDefinition(document, line), keyword(*line.getKeyword())
	{
		std::string text = line.getText();
		static const std::string regex = reflex::Matcher::convert("[^:]+:\\s*", reflex::convert_flag::unicode);
		static const reflex::Pattern pattern(regex);
		auto matcher = reflex::Matcher(pattern, text);
		if (matcher.find() && matcher.size() < text.size()) {
			name = trim(text.substr(matcher.size()));
		}
	}

	GherkinElement* GherkinFeature::push(GherkinDocument& document, const GherkinLine& line)
	{
		description.emplace_back(trim(line.getText()));
		return nullptr;
	}

	GherkinFeature::operator JSON() const
	{
		JSON json = GherkinElement::operator JSON();

		if (!name.empty())
			json["name"] = name;

		if (!description.empty())
			json["description"] = description;

		json["keyword"] = keyword;

		return json;
	}

	GherkinDefinition::GherkinDefinition(GherkinDocument& document, const GherkinLine& line)
		: GherkinElement(document, line), keyword(*line.getKeyword())
	{
	}

	GherkinDefinition::operator JSON() const
	{
		JSON json = GherkinElement::operator JSON();
		json["keyword"] = keyword;

		if (!tokens.empty())
			json["tokens"] = tokens;

		return json;
	}

	GherkinStep::GherkinStep(GherkinDocument& document, const GherkinLine& line)
		: GherkinElement(document, line), keyword(*line.getKeyword()), tokens(line.getTokens())
	{
	}

	GherkinStep::operator JSON() const
	{
		JSON json = GherkinElement::operator JSON();
		json["keyword"] = keyword;

		if (!tokens.empty())
			json["tokens"] = tokens;

		return json;
	}

	GherkinGroup::GherkinGroup(GherkinDocument& document, const GherkinLine& line)
		: GherkinElement(document, line), name(trim(line.getText()))
	{
	}

	GherkinGroup::operator JSON() const
	{
		JSON json = GherkinElement::operator JSON();
		json["name"] = name;
		return json;
	}

	GherkinError::GherkinError(GherkinLexer& lexer, const std::string& message)
		: line(lexer.lineno()), column(lexer.columno()), message(message)
	{
	}

	GherkinError::GherkinError(size_t line, const std::string& message)
		: line(line), column(0), message(message)
	{
	}

	GherkinError::operator JSON() const
	{
		JSON json;
		json["line"] = line;
		json["text"] = message;
		if (column) json["column"] = column;
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

	void GherkinDocument::setDefinition(std::unique_ptr<GherkinDefinition>& definition, GherkinLine& line)
	{
		if (definition) {
			auto keyword = line.getKeyword();
			if (keyword) {
				std::string type = GherkinKeyword::type2str(keyword->getType());
				error(line, type + " keyword duplicate error");
			}
			else
				error(line, "Unknown keyword type");
		}
		else {
			GherkinDefinition* def =
				line.getKeyword()->getType() == KeywordType::Feature
				? (GherkinDefinition*) new GherkinFeature(*this, line)
				: new GherkinDefinition(*this, line);
			definition.reset(def);
			resetElementStack(*def);
		}
	}

	void GherkinDocument::addScenarioDefinition(GherkinLine& line)
	{
		scenarios.emplace_back(*this, line);
		auto definition = &scenarios.back();
		resetElementStack(*definition);
	}

	GherkinKeyword* GherkinDocument::matchKeyword(GherkinTokens& line)
	{
		return provider.matchKeyword(language, line);
	}

	void GherkinDocument::exception(GherkinLexer& lexer, const char* message)
	{
		std::stringstream stream_message;
		stream_message << (message != NULL ? message : "lexer error") << " at " << lexer.lineno() << ":" << lexer.columno();
		throw MB2WCHAR(stream_message.str());
	}

	void GherkinDocument::error(GherkinLexer& lexer, const std::string& error)
	{
		errors.emplace_back(lexer, error);
	}

	void GherkinDocument::error(GherkinLine& line, const std::string& error)
	{
		errors.emplace_back(line.getLineNumber(), error);
	}

	void GherkinDocument::push(GherkinLexer& lexer, TokenType type, char ch)
	{
		if (currentLine == nullptr) {
			lines.push_back({ lexer });
			currentLine = &lines.back();
		}
		currentLine->push(lexer, type, ch);
		switch (type) {
		case TokenType::Language:
			setLanguage(lexer);
			break;
		case TokenType::Comment:
			commentStack.push_back(lexer.text());
			break;
		case TokenType::Tag:
			tagStack.push_back(lexer.text());
			break;
		}
	}

	void GherkinDocument::addTableLine(GherkinLine& line)
	{
		if (lastElement) {
			if (currentTable)
				currentTable->push(line);
			else {
				currentTable = lastElement->pushTable(line);
			}
		}
		else {
			//TODO: save error to error list
		}
	}

	void GherkinDocument::addElement(GherkinLine& line)
	{
		auto indent = line.getIndent();
		while (!elementStack.empty()) {
			if (elementStack.back().first < indent) break;
			elementStack.pop_back();
		}
		if (elementStack.empty()) {
			throw u"Element statck is empty";
		}
		auto parent = elementStack.back().second;
		auto element = parent->push(*this, line);
		if (element) {
			elementStack.emplace_back(indent, element);
			lastElement = element;
		}
	}

	void GherkinDocument::processLine(GherkinLine& line)
	{
		if (line.getType() != TokenType::Table)
			currentTable = nullptr;

		auto keyword = line.matchKeyword(*this);
		if (keyword) {
			switch (keyword->getType()) {
			case KeywordType::Feature:
				setDefinition(feature, line);
				break;
			case KeywordType::ScenarioOutline:
				setDefinition(outline, line);
				break;
			case KeywordType::Background:
				setDefinition(background, line);
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
				addElement(line);
				break;
			case TokenType::Table:
				addTableLine(line);
				break;
			case TokenType::Multiline:
				//TODO: add multy line
				break;
			}
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
		json["tags"] = getTags();
		return json.dump();
	}

	const GherkinTags& GherkinDocument::getTags() const
	{
		static const GherkinTags empty;
		return feature ? feature->getTags() : empty;
	}

	GherkinDocument::operator JSON() const
	{
		JSON json;
		json["language"] = language;

		if (feature)
			json["feature"] = JSON(*feature);

		if (outline)
			json["outline"] = JSON(*outline);

		if (background)
			json["background"] = JSON(*background);

		if (!scenarios.empty())
			json["scenarios"] = JSON(scenarios);

		if (!errors.empty())
			json["errors"] = JSON(errors);

		return json.dump();
	}
}
