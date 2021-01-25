#include "gherkin.h"
#include "lex.yy.h"

using namespace std;

GherkinToken::GherkinToken(Gherkin::TokenType type, GherkinLexer& l)
    : type(type), text(l.text()), columno(l.columno()) {};

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
