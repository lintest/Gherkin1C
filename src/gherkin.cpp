#include "gherkin.h"
#include "gherkin.lex.h"

GherkinToken::GherkinToken(Gherkin::TokenType t, GherkinLexer& l)
    : type(t), text(l.text()), columno(l.columno()) {};

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
    case Gherkin::Operator: return "operator";
    case Gherkin::Comment: return "comment";
    case Gherkin::Number: return "number";
    case Gherkin::Colon: return "colon";
    case Gherkin::Param: return "param";
    case Gherkin::Date: return "date";
    case Gherkin::Tag: return "tag";
    case Gherkin::Symbol: return "symbol";
    default: return "none";
    }
}

void GherkinLine::push(Gherkin::TokenType t, GherkinLexer& l)
{
    tokens.push_back({t, l});
}

GherkinLine::operator JSON() const
{
    JSON json;
    for (auto& t : tokens) {
        json.push_back(t);
    }
    return json;
}

void GherkinDocument::push(Gherkin::TokenType t, GherkinLexer& l)
{
    if (current == nullptr) {
        lines.push_back({});
        current = &lines.back();
        current->text = l.matcher().line();
    }
    current->push(t, l);
}
