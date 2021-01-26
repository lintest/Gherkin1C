#ifndef GHERKIN_H
#define GHERKIN_H

#include <vector>
#include <string>
#include "json.hpp"

using JSON = nlohmann::json;

namespace Gherkin {
	enum TokenType {
		Operator,
		Comment,
		Number,
		Colon,
		Param,
		Date,
		Tag,
		Symbol
	};
}

class GherkinLexer;
class GherkinToken;
class GherkinLine;

class GherkinKeword {
private:
	friend class GherkinProvider;
	friend class GherkinLine;
	bool toplevel = false;
	std::string type;
	std::string lang;
	std::string text;
	std::vector<std::wstring> words;
public:
	GherkinKeword(const std::string& lang, const std::string& type, const std::string& word);
	GherkinKeword* matchKeyword(const GherkinLine& line);
	operator JSON() const;
};

class GherkinProvider {
private:
	static std::vector<GherkinKeword> keywords;
public:
	static GherkinKeword* matchKeyword(const GherkinLine& line);
	static void setKeywords(const std::string& text);
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
	friend class GherkinKeword;
	std::vector<GherkinToken> tokens;
	std::string text;
private:
	JSON& dump(JSON& json, GherkinKeword* keyword) const;
	JSON& dump(JSON& json) const;
public:
	GherkinLine() {}
	operator JSON() const;
	void push(Gherkin::TokenType t, GherkinLexer& l);
};

class GherkinDocument {
private:
	std::vector<GherkinLine> lines;
	GherkinLine* current = nullptr;
	std::string text;
public:
	GherkinDocument() {}
	std::string dump() const;
	void next() { current = nullptr; }
	void push(Gherkin::TokenType t, GherkinLexer& l);
};

#endif//GHERKIN_H
