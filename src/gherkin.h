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
		KeywordType type;
		std::string text;
		bool toplevel;
	public:
		static KeywordType str2type(const std::string &text);
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
		void matchKeyword(const std::string& language);
	public:
		GherkinLine(GherkinLexer& l);
		void push(TokenType t, GherkinLexer& l);
		TokenType type() const;
		operator JSON() const;
	};

	class GherkinDefinition {
	private:
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
		std::string language;
		GherkinTags tag_stack;
		GherkinComments comment_stack;
		void setLanguage(GherkinLexer& lexer);
		std::vector<GherkinLine> lines;
		GherkinLine* current = nullptr;
		std::unique_ptr<GherkinDefinition> feature;
		std::unique_ptr<GherkinDefinition> backround;
		std::vector<GherkinDefinition> scenarios;
	public:
		GherkinDocument() {}
		std::string dump() const;
		GherkinTags tags() const;
		void next();
		void push(TokenType type, GherkinLexer& lexer);
		void error(GherkinLexer& lexer, const std::string& error);
	};

}

#endif//GHERKIN_H
