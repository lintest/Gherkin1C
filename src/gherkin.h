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

	class GherkinProvider;
	class GherkinDocument;
	class GherkinDefinition;
	class GherkinKeyword;
	class GherkinToken;
	class GherkinLine;

	using GherkinTags = std::vector<std::string>;
	using GherkinComments = std::vector<std::string>;
	using GherkinTokens = std::vector<GherkinToken>;

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
			GherkinKeyword* match(GherkinTokens& tokens);
		};
		using Keywords = std::map<std::string, std::vector<Keyword>>;
	private:
		static Keywords keywords;
	public:
		static void setKeywords(const std::string& text);
		static GherkinKeyword* matchKeyword(const std::string& lang, GherkinTokens& line);
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
		std::wstring wstr;
		std::string text;
		TokenType type;
		size_t column;
	public:
		GherkinToken(const GherkinToken& src)
			: type(src.type), wstr(src.wstr), text(src.text), column(src.column) {}
		GherkinToken(TokenType t, GherkinLexer& l);
		std::string getText() const { return text; }
		TokenType getType() const { return type; }
		operator JSON() const;
	};

	class GherkinLine {
	private:
		GherkinTokens tokens;
		std::string text;
		size_t lineNumber;
	private:
		std::unique_ptr<GherkinKeyword> keyword;
	public:
		GherkinLine(GherkinLexer& l);
		GherkinLine(size_t lineNumber);
		void push(TokenType t, GherkinLexer& l);
		GherkinKeyword* matchKeyword(GherkinDocument& document);
		const GherkinTokens getTokens() const { return tokens; }
		const GherkinKeyword* getKeyword() const { return keyword.get(); }
		size_t getLineNumber() const { return lineNumber; }
		std::string getText() const { return text; }
		TokenType getType() const;
		int getIndent() const;
		operator JSON() const;
	};

	class GherkinTable {
	public:
	private:
		std::vector<std::string> head;
		std::vector<std::vector<std::string>> body;
	public:
		GherkinTable(const GherkinLine& line);
		void push(const GherkinLine& line);
		operator JSON() const;
	};

	class GherkinElement {
	protected:
		size_t lineNumber;
		GherkinTags tags;
		GherkinComments comments;
		std::vector<GherkinElement*> items;
		std::vector<GherkinTable> tables;
	public:
		GherkinElement(GherkinDocument& document, const GherkinLine& line);
		virtual ~GherkinElement();
		void push(GherkinElement* item);
		GherkinTable* pushTable(const GherkinLine& line);
		virtual operator JSON() const = 0;
	};

	class GherkinDefinition
		: public GherkinElement {
	private:
		friend class GherkinDocument;
		std::string name;
		std::string description;
		GherkinKeyword keyword;
	public:
		GherkinDefinition(GherkinDocument& document, const GherkinLine& line);
		virtual operator JSON() const override;
	};

	class GherkinGroup
		: public GherkinElement {
	private:
		friend class GherkinDocument;
		std::string text;
	private:
		GherkinGroup(GherkinDocument& document, const GherkinLine& line);
		virtual operator JSON() const override;
	};

	class GherkinStep
		: public GherkinElement{
	private:
		friend class GherkinDocument;
		GherkinKeyword keyword;
		GherkinTokens tokens;
	private:
		GherkinStep(GherkinDocument& document, const GherkinLine& line);
		virtual operator JSON() const override;
	};

	class GherkinDocument {
	private:
		friend class GherkinElement;
		friend class GherkinDefinition;
		GherkinLine* currentLine = nullptr;
		GherkinTable* currentTable = nullptr;
		GherkinTags tagStack;
		GherkinComments commentStack;
		GherkinElement* lastElement = nullptr;
		std::vector<std::pair<int, GherkinElement*>> elementStack;
	private:
		std::vector<GherkinLine> lines;
		std::string language;
		std::unique_ptr<GherkinDefinition> feature;
		std::unique_ptr<GherkinDefinition> outline;
		std::unique_ptr<GherkinDefinition> backround;
		std::vector<GherkinDefinition> scenarios;
	private:
		void setLanguage(GherkinLexer& lexer);
		void processLine(GherkinLine& line);
		void setDefinition(std::unique_ptr<GherkinDefinition>& def, GherkinLine& line);
		void addScenarioDefinition(GherkinLine& line);
		void resetElementStack(GherkinElement& element);
		void addTableLine(GherkinLine& line);
		void addElement(GherkinLine& line);
	public:
		GherkinDocument() {}
		std::string dump() const;
		GherkinTags tags() const;
		void next(GherkinLexer& l);
		void push(TokenType type, GherkinLexer& lexer);
		void error(GherkinLexer& lexer, const std::string& error);
		void error(GherkinLine& line, const std::string& error);
		std::string getLanguage() const { return language; }
	};
}

#endif//GHERKIN_H
