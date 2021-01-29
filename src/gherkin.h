#ifndef GHERKIN_H
#define GHERKIN_H

#include <vector>
#include <string>
#include <memory>
#include "json.hpp"

using JSON = nlohmann::json;

class GherkinLexer;

namespace Gherkin {

	enum class TokenType {
		Language,
		Encoding,
		Multiline,
		Asterisk,
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

	enum class KeywordType {
		Feature,
		Background,
		Scenario,
		ScenarioOutline,
		Examples,
		And,
		But,
		Given,
		Rule,
		Then,
		When,
		None
	};

	using GherkinTags = std::vector<std::string>;
	using GherkinComments = std::vector<std::string>;

	class GherkinProvider;
	class GherkinDocument;
	class GherkinDefinition;
	class GherkinKeyword;
	class GherkinToken;
	class GherkinLine;

	class GherkinProvider {
	public:
		class Keyword {
		private:
			KeywordType type;
			std::string text;
			std::vector<std::wstring> words;
			friend class GherkinKeyword;
		public:
			Keyword(KeywordType type, const std::string& text);
			bool comp(const Keyword& other) const { return words.size() > other.words.size(); }
			GherkinKeyword* match(GherkinLine& line);
		};
		using Keywords = std::map<std::string, std::vector<Keyword>>;
	private:
		static Keywords keywords;
	public:
		static void setKeywords(const std::string& text);
		static GherkinKeyword* matchKeyword(const std::string& lang, GherkinLine& line);
		static std::string ParseFile(const std::wstring& filename);
	};

	class GherkinKeyword {
	private:
		friend class GherkinDocument;
		KeywordType type;
		std::string text;
		bool toplevel;
	public:
		static KeywordType str2type(const std::string& text);
		static std::string type2str(KeywordType type);
		GherkinKeyword(const GherkinProvider::Keyword& source, bool toplevel)
			: type(source.type), text(source.text), toplevel(toplevel) {}
		GherkinKeyword(const GherkinKeyword& source)
			: type(source.type), text(source.text), toplevel(source.toplevel) {}
		operator JSON() const;
	};

	class GherkinToken {
	private:
		std::string type2str() const;
	public:
		TokenType type;
		std::wstring wstr;
		std::string text;
		size_t column;
	public:
		GherkinToken(const GherkinToken& src)
			: type(src.type), wstr(src.wstr), text(src.text), column(src.column) {}
		GherkinToken(TokenType t, GherkinLexer& l);
		operator JSON() const;
	};

	class GherkinLine {
	private:
		friend class GherkinProvider;
		friend class GherkinDefinition;
		friend class GherkinDocument;
		friend class GherkinKeyword;
		std::vector<GherkinToken> tokens;
		std::string text;
		size_t lineNumber;
	private:
		std::unique_ptr<GherkinKeyword> keyword;
		GherkinKeyword* matchKeyword(GherkinDocument& document);
	public:
		GherkinLine(GherkinLexer& l);
		void push(TokenType t, GherkinLexer& l);
		TokenType type() const;
		operator JSON() const;
	};

	class GherkinDefinition {
	private:
		friend class GherkinDocument;
		size_t lineNumber;
		std::string name;
		std::string description;
		GherkinComments comments;
		GherkinKeyword keyword;
		GherkinTags tags;
	public:
		GherkinDefinition(GherkinDocument& document, const GherkinLine& source);
	};

	class GherkinDocument {
	private:
		friend class GherkinDefinition;
		GherkinLine* currentLine = nullptr;
		GherkinTags tagStack;
		GherkinComments commentStack;
		std::vector<GherkinDefinition> stepStack;
		GherkinDefinition* lastDefinition = nullptr;
	private:
		std::vector<GherkinLine> lines;
		std::string language;
		std::unique_ptr<GherkinDefinition> feature;
		std::unique_ptr<GherkinDefinition> outline;
		std::unique_ptr<GherkinDefinition> backround;
		std::vector<GherkinDefinition> scenarios;
	private:
		void setLanguage(GherkinLexer& lexer);
		void setDefinition(std::unique_ptr<GherkinDefinition>& def, GherkinLine& line);
		void addScenarioDefinition(GherkinLine& line);
	public:
		GherkinDocument() {}
		std::string dump() const;
		GherkinTags tags() const;
		void next();
		void push(TokenType type, GherkinLexer& lexer);
		void error(GherkinLexer& lexer, const std::string& error);
		void error(GherkinLine& line, const std::string& error);
		std::string getLanguage() const { return language; }
	};
}

#endif//GHERKIN_H
