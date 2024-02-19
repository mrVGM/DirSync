#include "JSONParser.h"

#include "parser.h"
#include "JSONBuilder.h"

#include "codeSource.h"

namespace
{
	const char* m_jsonGrammar = R"GRAM(
"Start" "Initial" "Terminal"

"Initial" "Object"

"Value" "String"
"Value" "Number"
"Value" "List"
"Value" "Object"

"List" "EmptyList"
"List" "[" "ListItems" "]"

"EmptyList" "[" "]"
"ListItems" "Value"
"ListItems" "ListItems" "," "Value"

"Object" "EmptyObject"
"Object" "{" "Properties" "}"

"EmptyObject" "{" "}"
"Properties" "KeyValuePair"
"Properties" "Properties" "," "KeyValuePair"

"KeyValuePair" "String" ":" "Value"
)GRAM";

	struct ParserDataContainer
	{
		scripting::Grammar* m_grammar = nullptr;
		scripting::Parser* m_parser = nullptr;
		scripting::ParserTable m_parserTable;

		void Init(const std::string& grammarSrc)
		{
			m_grammar = new scripting::Grammar(grammarSrc);
			m_grammar->GenerateParserStates();
			m_grammar->GenerateParserTable(m_parserTable);

			bool valid = m_parserTable.Validate();

			m_parser = new scripting::Parser(*m_grammar, m_parserTable);
		}

		~ParserDataContainer()
		{
			if (m_grammar)
			{
				delete m_grammar;
			}

			if (m_parser)
			{
				delete m_parser;
			}
		}
	};

	ParserDataContainer m_grammarContainer;
}

void json_parser::Boot()
{
	m_grammarContainer.Init(m_jsonGrammar);
}

scripting::ISymbol* json_parser::Parse(scripting::CodeSource& codeSource)
{
	return m_grammarContainer.m_parser->Parse(codeSource);
}
