#include "GherkinParser.h"
#include "gherkin.h"

std::vector<std::u16string> GherkinParser::names = {
	AddComponent(u"GherkinParser", []() { return new GherkinParser; }),
};

GherkinParser::GherkinParser()
{
	provider.reset(new Gherkin::GherkinProvider);

	AddProperty(u"Keywords", u"КлючевыеСлова",
		[&](VH value) { value = this->provider->getKeywords(); },
		[&](VH value) { this->provider->setKeywords(value); }
	);

	AddProperty(u"PrimitiveEscaping", u"ПримитивноеЭкранирование",
		[&](VH value) { value = this->provider->primitiveEscaping; },
		[&](VH value) { this->provider->primitiveEscaping = value; }
	);

	AddFunction(u"Parse", u"Прочитать",
		[&](VH data) {  this->result = this->provider->ParseText(data); }
	);

	AddFunction(u"ParseText", u"ПрочитатьТекст",
		[&](VH data) {  this->result = this->provider->ParseText(data); }
	);

	AddFunction(u"ParseFolder", u"ПрочитатьПапку",
		[&](VH filepath) {  this->result = this->provider->ParseFolder(filepath); }
	);

	AddFunction(u"ParseFile", u"ПрочитатьФайл",
		[&](VH filepath) {  this->result = this->provider->ParseFile(filepath); }
	);

	AddProcedure(u"Exit", u"ЗавершитьРаботуСистемы",
		[&](VH status) { this->ExitCurrentProcess(status); }, { {0, (int64_t)0 } }
	);

#ifdef _WINDOWS
	CreateProgressMonitor();

	AddProcedure(u"ScanFolder", u"СканироватьПапку",
		[&](VH filepath) {  this->ScanFolder(filepath); }
	);

	AddProcedure(u"AbortScan", u"ПрерватьСканирование",
		[&]() {  this->AbortScan(); }
	);
#endif//_WINDOWS
}

#ifdef _WINDOWS

class GherkinProgress
	: public Gherkin::AbstractProgress {
private:
	HWND hWnd;
public:
	GherkinProgress(Gherkin::GherkinProvider& provider, const std::wstring path, HWND hWnd)
		: provider(provider), path(path), hWnd(hWnd) {}
	virtual void Send(const std::string& msg) override {
		auto data = (LPARAM)new std::string(msg);
		::SendMessageW(hWnd, WM_PARSING_PROGRESS, 0, data);
	}
	void OnFinished(const std::string& msg) {
		auto data = (LPARAM)new std::string(msg);
		::SendMessageW(hWnd, WM_PARSING_FINISHED, 0, data);
	}
	const Gherkin::GherkinProvider& provider;
	const std::wstring path;
};

GherkinParser::~GherkinParser()
{
	::DestroyWindow(hWndMonitor);
}

void GherkinParser::ExitCurrentProcess(int64_t status)
{
	ExitProcess((UINT)status);
}

static LRESULT CALLBACK MonitorWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PARSING_PROGRESS:
	case WM_PARSING_FINISHED: {
		auto component = (GherkinParser*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (component) component->OnProgress(message, *(std::string*)lParam);
		return 0;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

static DWORD WINAPI ParserThreadProc(LPVOID lpParam)
{
	std::unique_ptr<GherkinProgress> progress((GherkinProgress*)lpParam);
	auto text = progress->provider.ParseFolder(progress->path, progress.get());
	progress->OnFinished(text);
	return 0;
}

void GherkinParser::CreateProgressMonitor()
{
	const LPCWSTR wsClassName = L"VanessaParserMonitor";

	WNDCLASS wndClass = {};
	wndClass.hInstance = hModule;
	wndClass.lpszClassName = wsClassName;
	wndClass.lpfnWndProc = MonitorWndProc;
	RegisterClass(&wndClass);

	hWndMonitor = CreateWindowW(wsClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hModule, 0);
	SetWindowLongPtr(hWndMonitor, GWLP_USERDATA, (LONG_PTR)this);
}

void GherkinParser::OnProgress(UINT id, const std::string& data)
{
	std::u16string message = id == WM_PARSING_FINISHED ? u"PARSING_FINISHED" : u"PARSING_PROGRESS";
	ExternalEvent(message, MB2WCHAR(data));
}

void GherkinParser::ScanFolder(const std::wstring& path)
{
	auto progress = new GherkinProgress(*provider, path, hWndMonitor);
	CreateThread(0, NULL, ParserThreadProc, (LPVOID)progress, NULL, NULL);
}

void GherkinParser::AbortScan()
{
	provider->AbortScan();
}

#else//_WINDOWS

void GherkinParser::ExitCurrentProcess(int64_t status)
{
	exit((int)status);
}

#endif//_WINDOWS
