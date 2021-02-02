#include "GherkinParser.h"
#include "gherkin.h"

std::vector<std::u16string> GherkinParser::names = {
	AddComponent(u"GherkinParser", []() { return new GherkinParser; }),
};

GherkinParser::GherkinParser()
{
	AddProperty(u"Keywords", u"КлючевыеСлова", nullptr,
		[&](VH value) { Gherkin::GherkinProvider::setKeywords(value); }
	);

	AddFunction(u"Parse", u"Прочитать",
		[&](VH data) {  this->result = this->Parse(data); }
	);

	AddFunction(u"ParseFile", u"ПрочитатьФайл",
		[&](VH filename) {  this->result = Gherkin::GherkinProvider::ParseFile(filename); }
	);

	AddProcedure(u"Exit", u"ЗавершитьРаботуСистемы",
		[&](VH status) { this->ExitCurrentProcess(status); }, { {0, (int64_t)0 } }
	);
}

#ifdef _WINDOWS

void GherkinParser::ExitCurrentProcess(int64_t status)
{
	ExitProcess((UINT)status);
}

#else//_WINDOWS
void GherkinParser::ExitCurrentProcess(int64_t status)
{
	exit((int)status);
}

#endif//_WINDOWS

std::string GherkinParser::Parse(VH var)
{
	if (var.type() == VTYPE_PWSTR) {
		std:: string text = var;
		return Gherkin::GherkinProvider::Parse(text);
	}
	else
		return {};
}
