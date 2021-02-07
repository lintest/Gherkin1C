#ifndef GHERKIN_H
#define GHERKIN_H

#include <vector>
#include <string>
#include <memory>
#include "json.hpp"

using JSON = nlohmann::json;

class GherkinLexer;
class GherkinParser;

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
		If,
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
	class GherkinGroup;
	class GherkinLine;
	class GherkinStep;

	using GherkinSnippet = std::wstring;
	using GherkinTags = std::vector<std::string>;
	using GherkinComments = std::vector<std::string>;
	using GherkinTokens = std::vector<GherkinToken>;
	using GherkinDef = std::unique_ptr<GherkinDefinition>;
	using GherkinSnippets = std::vector<std::wstring>;

	class AbstractProgress {
	public:
		virtual void Send(const std::string& msg) = 0;
	};

	class GherkinProvider {
	public:
		class Keyword {
		private:
			KeywordType type;
			std::string text;
			std::vector<std::wstring> words;
			friend class GherkinProvider;
			friend class GherkinKeyword;
		public:
			Keyword(KeywordType type, const std::string& text);
			GherkinKeyword* match(GherkinTokens& tokens) const;
			bool comp(const Keyword& other) const {
				return words.size() > other.words.size();
			}
		};
		using Keywords = std::map<std::string, std::vector<Keyword>>;
	private:
		Keywords keywords;
		size_t identifier = 0;
		GherkinParser* parser = nullptr;
	public:
		bool primitiveEscaping = false;
		std::string getKeywords() const;
		void setKeywords(const std::string& text);
		GherkinKeyword* matchKeyword(const std::string& lang, GherkinTokens& line) const;
		std::string ParseFolder(const std::wstring& path, const std::string& filter, AbstractProgress* progress = nullptr) const;
		std::string ParseFile(const std::wstring& path) const;
		std::string ParseText(const std::string& text) const;
		void AbortScan() { ++identifier; };
	};

	class GherkinKeyword {
	private:
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
		KeywordType getType() const { return type; }
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
		char symbol;
	public:
		GherkinToken(const GherkinToken& src)
			: type(src.type), wstr(src.wstr), text(src.text), column(src.column), symbol(src.symbol) {}
		GherkinToken(GherkinLexer& lexer, TokenType type, char ch);
		std::string getText() const { return text; }
		std::wstring getWstr() const { return wstr; }
		TokenType getType() const { return type; }
		operator JSON() const;
	};

	class GherkinLine {
	private:
		std::wstring wstr;
		GherkinTokens tokens;
		std::string text;
		size_t lineNumber;
	private:
		std::unique_ptr<GherkinKeyword> keyword;
	public:
		GherkinLine(GherkinLexer& l);
		GherkinLine(size_t lineNumber);
		void push(GherkinLexer& lexer, TokenType type, char ch);
		GherkinKeyword* matchKeyword(GherkinDocument& document);
		const GherkinTokens getTokens() const { return tokens; }
		const GherkinKeyword* getKeyword() const { return keyword.get(); }
		size_t getLineNumber() const { return lineNumber; }
		std::wstring getWstr() const { return wstr; }
		std::string getText() const { return text; }
		TokenType getType() const;
		int getIndent() const;
		operator JSON() const;
	};

	class GherkinTable {
	public:
	private:
		size_t lineNumber;
		std::vector<std::string> head;
		std::vector<std::vector<std::string>> body;
	public:
		GherkinTable(const GherkinLine& line);
		void push(const GherkinLine& line);
		operator JSON() const;
	};

	class GherkinElement {
	protected:
		std::wstring wstr;
		std::string text;
		size_t lineNumber;
		GherkinTags tags;
		GherkinComments comments;
		std::vector<std::unique_ptr<GherkinElement>> items;
		std::vector<GherkinTable> tables;
	public:
		GherkinElement(GherkinLexer& lexer, const GherkinLine& line);
		virtual GherkinElement* push(GherkinLexer& lexer, const GherkinLine& line);
		GherkinTable* pushTable(const GherkinLine& line);
		const GherkinTags& getTags() const { return tags; }
		virtual KeywordType getType() const { return KeywordType::None; }
		virtual GherkinSnippet getSnippet() const { return {}; }
		virtual operator JSON() const;
	};

	class GherkinGroup
		: public GherkinElement {
	private:
		std::string name;
	public:
		GherkinGroup(GherkinLexer& lexer, const GherkinLine& line);
		virtual operator JSON() const override;
	};

	class GherkinStep
		: public GherkinElement {
	private:
		GherkinKeyword keyword;
		GherkinTokens tokens;
	public:
		GherkinStep(GherkinLexer& lexer, const GherkinLine& line);
		virtual GherkinSnippet getSnippet() const override;
		virtual operator JSON() const override;
	};

	class GherkinDefinition
		: public GherkinElement {
	private:
		GherkinTokens tokens;
		std::unique_ptr<GherkinStep> examples;
	protected:
		GherkinKeyword keyword;
	public:
		GherkinDefinition(GherkinLexer& lexer, const GherkinLine& line);
		virtual GherkinElement* push(GherkinLexer& lexer, const GherkinLine& line) override;
		virtual KeywordType getType() const override { return keyword.getType(); };
		virtual GherkinSnippet getSnippet() const override;
		virtual operator JSON() const override;
	};

	class GherkinFeature
		: public GherkinDefinition {
	private:
		std::string name;
		std::vector<std::string> description;
	public:
		GherkinFeature(GherkinLexer& lexer, const GherkinLine& line);
		virtual GherkinElement* push(GherkinLexer& lexer, const GherkinLine& line) override;
		virtual operator JSON() const override;
	};

	class GherkinException
		: public std::runtime_error {
	private:
		const size_t line = 0;
		const size_t column = 0;
	public:
		GherkinException(GherkinLexer& lexer, const std::string& message);
		GherkinException(GherkinLexer& lexer, char const* const message);
		GherkinException(const GherkinException& src);
		operator JSON() const;
	};

	class GherkinError {
	private:
		size_t line = 0;
		size_t column = 0;
		std::string message;
	public:
		GherkinError(GherkinLexer& lexer, const std::string& message);
		GherkinError(size_t line, const std::string& message);
		operator JSON() const;
	};

	class GherkinDocument {
	private:
		std::string language;
		GherkinDef feature;
		GherkinDef background;
		std::vector<GherkinDef> scenarios;
		std::vector<GherkinError> errors;
	private:
		void setLanguage(GherkinLexer& lexer);
		void processLine(GherkinLexer& lexer, GherkinLine& line);
		void setDefinition(GherkinDef& definition, GherkinLexer& lexer, GherkinLine& line);
		void addScenarioDefinition(GherkinLexer& lexer, GherkinLine& line);
		void addScenarioExamples(GherkinLexer& lexer, GherkinLine& line);
		void resetElementStack(GherkinLexer& lexer, GherkinElement& element);
		void addTableLine(GherkinLexer& lexer, GherkinLine& line);
		void addElement(GherkinLexer& lexer, GherkinLine& line);
	public:
		GherkinDocument(const GherkinProvider& provider) : provider(provider) {}
		const GherkinProvider& provider;
		void next(GherkinLexer& lexer);
		void push(GherkinLexer& lexer, TokenType type, char ch = 0);
		void exception(GherkinLexer& lexer, const char* message);
		void error(GherkinLexer& lexer, const std::string& error);
		void error(GherkinLine& line, const std::string& error);
		GherkinKeyword* matchKeyword(GherkinTokens& line);
		GherkinSnippets getExportSnippets() const;
		const GherkinTags& getTags() const;
		std::string dump() const;
		operator JSON() const;
	};
}

#ifdef _WINDOWS

#define WM_PARSING_PROGRESS (WM_USER + 3)
#define WM_PARSING_FINISHED (WM_USER + 4)

#endif//_WINDOWS

#endif//GHERKIN_H
