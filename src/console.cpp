#include "gherkin.h"
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <codecvt>

using namespace Gherkin;

#ifdef _WINDOWS
#define WCHAR_T wchar_t
#else
#define WCHAR_T uint16_t
#endif //_WINDOWS

std::string WC2MB(const std::wstring& wstr)
{
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(wstr);
}

std::wstring MB2WC(const std::string& str)
{
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}

std::string WCHAR2MB(std::basic_string_view<WCHAR_T> src)
{
#ifdef _WINDOWS
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cvt_utf8_utf16;
	return cvt_utf8_utf16.to_bytes(src.data(), src.data() + src.size());
#else
	static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cvt_utf8_utf16;
	return cvt_utf8_utf16.to_bytes(reinterpret_cast<const char16_t*>(src.data()),
		reinterpret_cast<const char16_t*>(src.data() + src.size()));
#endif//_WINDOWS
}

std::wstring WCHAR2WC(std::basic_string_view<WCHAR_T> src) {
#ifdef _WINDOWS
	return std::wstring(src);
#else
	static std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>> conv;
	return conv.from_bytes(reinterpret_cast<const char*>(src.data()),
		reinterpret_cast<const char*>(src.data() + src.size()));
#endif//_WINDOWS
}

std::u16string MB2WCHAR(std::string_view src) {
#ifdef _WINDOWS
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cvt_utf8_utf16;
	std::wstring tmp = cvt_utf8_utf16.from_bytes(src.data(), src.data() + src.size());
	return std::u16string(reinterpret_cast<const char16_t*>(tmp.data()), tmp.size());
#else
	static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cvt_utf8_utf16;
	return cvt_utf8_utf16.from_bytes(src.data(), src.data() + src.size());
#endif//_WINDOWS
}

std::locale locale_ru = std::locale("ru_RU.UTF-8");

std::u16string upper(const std::u16string& src)
{
	std::u16string str = src;
	std::transform(str.begin(), str.end(), str.begin(), [](wchar_t ch) { return std::toupper(ch, locale_ru); });
	return str;
}

std::u16string lower(const std::u16string& src)
{
	std::u16string str = src;
	std::transform(str.begin(), str.end(), str.begin(), [](wchar_t ch) { return std::tolower(ch, locale_ru); });
	return str;
}

std::wstring upper(const std::wstring& src)
{
	std::wstring str = src;
	std::transform(str.begin(), str.end(), str.begin(), [](wchar_t ch) { return std::toupper(ch, locale_ru); });
	return str;
}

std::wstring lower(const std::wstring& src)
{
	std::wstring str = src;
	std::transform(str.begin(), str.end(), str.begin(), [](wchar_t ch) { return std::tolower(ch, locale_ru); });
	return str;
}

int wmain(int argc, wchar_t* argv[], wchar_t* envp[])
{
	GherkinProvider provider;
	if (argc > 2) {
		std::ifstream fstream;
		fstream.open(argv[2]);
		std::string json((std::istreambuf_iterator<char>(fstream)), std::istreambuf_iterator<char>());
		provider.setKeywords(json);
	}
	else {
		std::string json = R"({"en": {"simple": ["Then", "And", "Scenario Template"]}})";
		provider.setKeywords(json);
	}

	if (argc > 1) {
		std::cout << provider.ParseFolder(WC2MB(argv[1]), "[]", {});
	}

	return 0;
}
