#include "gherkin.h"
#include "lex.yy.h"

using namespace std;

GherkinToken::GherkinToken(const string& type, GherkinLexer& l)
    : type(type), text(l.text()), columno(l.columno()) {};
