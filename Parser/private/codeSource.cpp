#include "codeSource.h"
#include "tokenizer.h"
#include "symbol.h"

#include <vector>
#include <iostream>
#include <set>

void scripting::CodeSource::Tokenize()
{
	for (int i = 0; i < m_code.size(); ++i) {
		std::string tmp;
		tmp.push_back(m_code[i]);
		SimpleSymbol* symbol = CreateSimpleSymbol();
		symbol->m_name = tmp;
		symbol->m_codePosition = i;

		m_symbols.push_back(symbol);
	}
}

void printSymbols(const std::vector<scripting::ISymbol*>& symbols)
{
	for (int i = 0; i < symbols.size(); ++i) {
		std::cout << symbols[i]->m_name << " ";
	}
}

bool scripting::CodeSource::TokenizeForParser()
{
	NewLineSymbolTokenizer newLineTokenizer;
	StringTokenizer stringTokenizer;
	CommentTokenizer commentTokenizer;
	BlankTokenizer blankTokenizer;
	UnsignedNumberTokenizer numberTokenizer;
	NameTokenizer nameTokenizer;

	std::vector<KeywordTokenizer> keywordTokenizers;
	keywordTokenizers.push_back(KeywordTokenizer("continue"));
	keywordTokenizers.push_back(KeywordTokenizer("return"));
	keywordTokenizers.push_back(KeywordTokenizer("break"));
	keywordTokenizers.push_back(KeywordTokenizer("while"));
	keywordTokenizers.push_back(KeywordTokenizer("catch"));
	keywordTokenizers.push_back(KeywordTokenizer("func"));
	keywordTokenizers.push_back(KeywordTokenizer("none"));
	keywordTokenizers.push_back(KeywordTokenizer("let"));
	keywordTokenizers.push_back(KeywordTokenizer("try"));
	keywordTokenizers.push_back(KeywordTokenizer("if"));

	std::vector<ISymbol*> symbols = newLineTokenizer.Tokenize(m_symbols);
	if (newLineTokenizer.m_error) {
		return false;
	}
	
	symbols = stringTokenizer.Tokenize(symbols);
	if (stringTokenizer.m_error) {
		return false;
	}

	symbols = commentTokenizer.Tokenize(symbols);
	if (commentTokenizer.m_error) {
		return false;
	}

	std::vector<ISymbol*> tmp;
	for (int i = 0; i < symbols.size(); ++i) {
		if (symbols[i]->m_name == "Comment") {
			continue;
		}
		tmp.push_back(symbols[i]);
	}

	symbols = tmp;

	symbols = blankTokenizer.Tokenize(symbols);
	if (blankTokenizer.m_error) {
		return false;
	}

	symbols = nameTokenizer.Tokenize(symbols);
	if (nameTokenizer.m_error) {
		return false;
	}

	for (int i = 0; i < keywordTokenizers.size(); ++i) {
		KeywordTokenizer& cur = keywordTokenizers[i];
		symbols = cur.Tokenize(symbols);
		if (cur.m_error) {
			return false;
		}
	}

	symbols = numberTokenizer.Tokenize(symbols);
	if (numberTokenizer.m_error) {
		return false;
	}

	tmp.clear();
	
	for (int i = 0; i < symbols.size(); ++i) {
		if (symbols[i]->m_name == "Blank") {
			continue;
		}
		tmp.push_back(symbols[i]);
	}

	m_parserReady = tmp;
	SimpleSymbol* terminal = CreateSimpleSymbol();
	terminal->m_name = "Terminal";
	terminal->m_codePosition = -1;
	m_parserReady.push_back(terminal);

	return true;
}


bool scripting::CodeSource::TokenizeForColladaReader()
{
	NewLineSymbolTokenizer newLineTokenizer;
	StringTokenizer stringTokenizer;
	BlankTokenizer blankTokenizer;
	UnsignedNumberTokenizer numberTokenizer;
	PoweredNumberTokenizer poweredNumberTokenizer;
	NameTokenizer nameTokenizer;

	std::vector<ISymbol*> symbols = newLineTokenizer.Tokenize(m_symbols);
	if (newLineTokenizer.m_error) {
		return false;
	}

	symbols = stringTokenizer.Tokenize(symbols);
	if (stringTokenizer.m_error) {
		return false;
	}

	symbols = blankTokenizer.Tokenize(symbols);
	if (blankTokenizer.m_error) {
		return false;
	}

	symbols = nameTokenizer.Tokenize(symbols);
	if (nameTokenizer.m_error) {
		return false;
	}

	symbols = numberTokenizer.Tokenize(symbols);
	if (numberTokenizer.m_error) {
		return false;
	}

	std::vector<ISymbol*> tmp;
	for (int i = 0; i < symbols.size(); ++i) {
		if (symbols[i]->m_name == "Blank") {
			continue;
		}
		tmp.push_back(symbols[i]);
	}

	symbols = tmp;
	tmp.clear();

	{
		scripting::ISymbol* minus = nullptr;
		for (int i = 0; i < symbols.size(); ++i) {
			scripting::ISymbol* cur = symbols[i];
			if (minus) {
				if (cur->m_name == "Number") {
					CompositeSymbol* cs = CreateCompositeSymbol();
					cs->m_name = "Number";
					cs->m_symbolData.m_number = -cur->m_symbolData.m_number;
					cs->m_childSymbols.push_back(minus);
					cs->m_childSymbols.push_back(cur);
					tmp.push_back(cs);
				}
				else {
					tmp.push_back(minus);
					tmp.push_back(cur);
				}
				minus = nullptr;
				continue;
			}

			if (cur->m_name == "-") {
				minus = cur;
				continue;
			}

			tmp.push_back(cur);
		}

		if (minus) {
			tmp.push_back(minus);
		}
	}


	symbols = tmp;
	tmp.clear();

	symbols = poweredNumberTokenizer.Tokenize(symbols);

	std::set<std::string> specialSymbols;
	specialSymbols.insert("<");
	specialSymbols.insert(">");
	specialSymbols.insert("?");
	specialSymbols.insert("/");
	specialSymbols.insert("=");
	specialSymbols.insert(":");

	for (int i = 0; i < symbols.size(); ++i) {
		scripting::ISymbol* cur = symbols[i];

		if (cur->m_name.size() > 1) {
			tmp.push_back(cur);
			continue;
		}

		if (specialSymbols.contains(cur->m_name)) {
			tmp.push_back(cur);
			continue;
		}

		cur->m_symbolData.m_string = cur->m_name;
		cur->m_name = "SingleSymbol";
		tmp.push_back(cur);
	}

	symbols = tmp;

	m_parserReady = symbols;
	SimpleSymbol* terminal = CreateSimpleSymbol();
	terminal->m_name = "Terminal";
	terminal->m_codePosition = -1;
	m_parserReady.push_back(terminal);

	return true;
}

bool scripting::CodeSource::TokenizeForJSONReader()
{
	NewLineSymbolTokenizer newLineTokenizer;
	StringTokenizer stringTokenizer;
	BlankTokenizer blankTokenizer;
	UnsignedNumberTokenizer numberTokenizer;
	PoweredNumberTokenizer poweredNumberTokenizer;
	
	std::vector<ISymbol*> symbols = newLineTokenizer.Tokenize(m_symbols);
	if (newLineTokenizer.m_error) {
		return false;
	}

	symbols = stringTokenizer.Tokenize(symbols);
	if (stringTokenizer.m_error) {
		return false;
	}

	symbols = blankTokenizer.Tokenize(symbols);
	if (blankTokenizer.m_error) {
		return false;
	}

	symbols = numberTokenizer.Tokenize(symbols);
	if (numberTokenizer.m_error) {
		return false;
	}

	std::vector<ISymbol*> tmp;
	for (int i = 0; i < symbols.size(); ++i) {
		if (symbols[i]->m_name == "Blank") {
			continue;
		}
		tmp.push_back(symbols[i]);
	}

	symbols = tmp;
	tmp.clear();

	{
		scripting::ISymbol* minus = nullptr;
		for (int i = 0; i < symbols.size(); ++i) {
			scripting::ISymbol* cur = symbols[i];
			if (minus) {
				if (cur->m_name == "Number") {
					CompositeSymbol* cs = CreateCompositeSymbol();
					cs->m_name = "Number";
					cs->m_symbolData.m_number = -cur->m_symbolData.m_number;
					cs->m_childSymbols.push_back(minus);
					cs->m_childSymbols.push_back(cur);
					tmp.push_back(cs);
				}
				else {
					tmp.push_back(minus);
					tmp.push_back(cur);
				}
				minus = nullptr;
				continue;
			}

			if (cur->m_name == "-") {
				minus = cur;
				continue;
			}

			tmp.push_back(cur);
		}

		if (minus) {
			tmp.push_back(minus);
		}
	}

	symbols = tmp;
	tmp.clear();

	symbols = poweredNumberTokenizer.Tokenize(symbols);
	
	m_parserReady = symbols;
	SimpleSymbol* terminal = CreateSimpleSymbol();
	terminal->m_name = "Terminal";
	terminal->m_codePosition = -1;
	m_parserReady.push_back(terminal);

	return true;
}

scripting::SimpleSymbol* scripting::CodeSource::CreateSimpleSymbol()
{
	SimpleSymbol* res = new SimpleSymbol();
	res->m_codeSource = this;
	m_created.push_back(res);
	return res;
}

scripting::CompositeSymbol* scripting::CodeSource::CreateCompositeSymbol()
{
	CompositeSymbol* res = new CompositeSymbol();
	res->m_codeSource = this;
	m_created.push_back(res);
	return res;
}

scripting::CodeSource::~CodeSource()
{
	for (int i = 0; i < m_created.size(); ++i) {
		delete m_created[i];
	}
}
