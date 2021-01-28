#ifndef GHERKIN_H
#define GHERKIN_H

#include <vector>
#include <string>
#include <memory>
#include "json.hpp"

using JSON = nlohmann::json;

namespace Gherkin {
	enum TokenType {
		Language,
		Encoding,
		Multiline,
		Operator,
		Keyword,
		Comment,
		Number,
		Symbol,
		Colon,
		Param,
		Table,
		Cell,
		Line,
		Text,
		Date,
		Tag,
		None
	};
}

using GherkinTags = std::vector<std::string>;

class GherkinKeyword;
class GherkinLexer;
class GherkinToken;
class GherkinLine;

class GherkinProvider {
public:
	class Keyword {
	private:
		std::string type;
		std::string text;
		std::vector<std::wstring> words;
		friend class GherkinKeyword;
	public:
		Keyword(const std::string& type, const std::string& text);
		bool comp(const Keyword& other) const { return words.size() > other.words.size(); }
		GherkinKeyword* match(GherkinLine& line);
	};
	using Keywords = std::map<std::string, std::vector<Keyword>>;
private:
	static Keywords keywords;
public:
	static void setKeywords(const std::string& text);
	static GherkinKeyword* matchKeyword(const std::string &lang, GherkinLine& line);
	static std::string ParseFile(const std::wstring& filename);
};

class GherkinKeyword {
private:
	std::string type;
	std::string text;
	bool toplevel;
public:
	GherkinKeyword(GherkinProvider::Keyword& source, bool toplevel)
		: type(source.type), text(source.text), toplevel(toplevel) {}
	operator JSON() const;
};

class GherkinToken {
private:
	std::string type2str() const;
public:
	Gherkin::TokenType type;
	std::wstring wstr;
	std::string text;
	size_t columno;
public:
	GherkinToken(Gherkin::TokenType t, GherkinLexer& l);
	operator JSON() const;
};

class GherkinLine {
private:
	friend class GherkinProvider;
	friend class GherkinDocument;
	friend class GherkinKeyword;
	std::vector<GherkinToken> tokens;
	std::string text;
	size_t lineNumber;
private:
	std::unique_ptr<GherkinKeyword> keyword;
	void matchKeyword(const std::string& language);
public:
	GherkinLine(GherkinLexer& l);
	void push(Gherkin::TokenType t, GherkinLexer& l);
	Gherkin::TokenType type() const;
	operator JSON() const;
};

class GherkinDocument {
private:
	std::vector<GherkinLine> lines;
	GherkinLine* current = nullptr;
	std::string language;
private:
	void setLanguage(GherkinLexer& lexer);
public:
	GherkinDocument() {}
	std::string dump() const;
	GherkinTags tags() const;
	void next();
	void push(Gherkin::TokenType type, GherkinLexer& lexer);
	void error(GherkinLexer& lexer, const std::string& error);
};

#endif//GHERKIN_H
