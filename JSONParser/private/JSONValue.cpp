#include "JSONValue.h"

#include "JSONParser.h"
#include "JSONBuilder.h"

#include <list>
#include <string>
#include <map>
#include <stack>

#include <vector>

#include <sstream>

namespace
{
	struct Builder;

	void SortMapKeys(const std::map<std::string, Builder*>& map, std::vector<std::string>& keys)
	{
		keys.clear();
		for (auto it = map.begin(); it != map.end(); ++it)
		{
			keys.push_back(it->first);
		}

		for (int i = 0; i < keys.size() - 1; ++i)
		{
			for (int j = i + 1; j < keys.size(); ++j)
			{
				if (keys[i] < keys[j])
				{
					std::string tmp = keys[i];
					keys[i] = keys[j];
					keys[j] = tmp;
				}
			}
		}
	}

	struct BuildManager
	{
		std::stack<Builder*> m_builders;
		const json_parser::JSONValue& m_value;
		Builder* m_initial = nullptr;
		std::string m_ident;
		std::string m_newLine;

		BuildManager(const json_parser::JSONValue& val) :
			m_value(val)
		{
		}

		void Build(std::string& outRes);
	};

	struct Builder
	{
		const json_parser::JSONValue& m_value;
		BuildManager& m_manager;

		std::list<std::string> m_lines;

		std::map<std::string, Builder*> m_propsBuilders;
		std::list<Builder*> m_listBuilders;

		Builder(BuildManager& manager, const json_parser::JSONValue& value) :
			m_manager(manager),
			m_value(value)
		{
		}

		bool Build()
		{
			using namespace json_parser;

			if (m_value.m_type == ValueType::Number)
			{
				std::stringstream ss;
				ss << std::get<double>(m_value.m_payload);

				m_lines.push_back(ss.str());
				return true;
			}

			if (m_value.m_type == ValueType::String)
			{
				std::stringstream ss;
				ss << "\"" << std::get<std::string>(m_value.m_payload) << "\"";

				m_lines.push_back(ss.str());
				return true;
			}

			if (m_value.m_type == ValueType::List)
			{
				const std::list<JSONValue>& listVal = std::get<std::list<JSONValue>>(m_value.m_payload);
				if (listVal.empty())
				{
					m_lines.push_back("[]");
					return true;
				}

				if (!m_listBuilders.empty())
				{
					m_lines.push_back("[");

					auto last = --m_listBuilders.end();
					for (auto builderIt = m_listBuilders.begin(); builderIt != m_listBuilders.end(); ++builderIt)
					{
						Builder* cur = *builderIt;
						for (auto it = cur->m_lines.begin(); it != cur->m_lines.end(); ++it)
						{
							m_lines.push_back(m_manager.m_ident + *it);
						}

						if (builderIt != last)
						{
							m_lines.back() += ",";
						}
					}
					m_lines.push_back("]");
					return true;
				}

				for (auto it = listVal.begin(); it != listVal.end(); ++it)
				{
					Builder* cur = new Builder(m_manager, *it);
					m_listBuilders.push_back(cur);
					m_manager.m_builders.push(cur);
				}

				return false;
			}

			if (m_value.m_type == ValueType::Object)
			{
				const std::map<std::string, JSONValue>& mapVal = std::get<std::map<std::string, JSONValue>>(m_value.m_payload);
				if (mapVal.empty())
				{
					m_lines.push_back("{}");
					return true;
				}

				if (!m_propsBuilders.empty())
				{
					m_lines.push_back("{");

					std::vector<std::string> keys;
					SortMapKeys(m_propsBuilders, keys);

					auto last = --keys.end();
					for (auto keyIt = keys.begin(); keyIt != keys.end(); ++keyIt)
					{
						std::string key = *keyIt;
						Builder* curBuilder = m_propsBuilders[key];

						std::stringstream ss;
						ss << m_manager.m_ident << "\"" << key << "\":";
						auto it = curBuilder->m_lines.begin();
						ss << *it;
						++it;
						m_lines.push_back(ss.str());

						for (; it != curBuilder->m_lines.end(); ++it)
						{
							m_lines.push_back(m_manager.m_ident + *it);
						}

						if (keyIt != last)
						{
							m_lines.back() += ",";
						}
					}

					m_lines.push_back("}");
					return true;
				}

				for (auto it = mapVal.begin(); it != mapVal.end(); ++it)
				{
					Builder* cur = new Builder(m_manager, it->second);
					m_propsBuilders[it->first] = cur;
					m_manager.m_builders.push(cur);
				}

				return false;
			}

			return false;
		}

		void Dispose()
		{
			for (auto it = m_listBuilders.begin(); it != m_listBuilders.end(); ++it)
			{
				delete (*it);
			}

			for (auto it = m_propsBuilders.begin(); it != m_propsBuilders.end(); ++it)
			{
				delete it->second;
			}

			m_listBuilders.clear();
			m_propsBuilders.clear();
		}
	};


	void BuildManager::Build(std::string& outRes)
	{
		m_initial = new Builder(*this, m_value);
		m_builders.push(m_initial);

		while (!m_builders.empty())
		{
			Builder* cur = m_builders.top();
			bool res = cur->Build();
			if (res)
			{
				cur->Dispose();
				m_builders.pop();
			}
		}

		std::stringstream ss;
		for (auto it = m_initial->m_lines.begin(); it != m_initial->m_lines.end(); ++it)
		{
			ss << *it << m_newLine;
		}

		outRes = ss.str();

		delete m_initial;
	}
}

void json_parser::JSONValue::FromString(const std::string& str, JSONValue& outValue)
{
	scripting::CodeSource cs;
	cs.m_code = str;
	cs.Tokenize();
	cs.TokenizeForJSONReader();
	scripting::ISymbol* s = Parse(cs);

	ValueBuilder vb(s);
	vb.Build(outValue);
}

json_parser::JSONValue::JSONValue(const JSONValuePayload& payload)
{
	m_payload = payload;
	m_type = static_cast<ValueType>(payload.index());
}

json_parser::JSONValue::JSONValue(ValueType type)
{
	m_type = type;
	
	switch (m_type)
	{
	case ValueType::List:
		m_payload = std::list<JSONValue>();
		break;
	case ValueType::Object:
		m_payload = std::map<std::string, JSONValue>();
		break;
	}
}


std::map<std::string, json_parser::JSONValue>& json_parser::JSONValue::GetAsObj()
{
	return std::get<std::map<std::string, json_parser::JSONValue>>(m_payload);
}

std::list<json_parser::JSONValue>& json_parser::JSONValue::GetAsList()
{
	return std::get<std::list<json_parser::JSONValue>>(m_payload);
}

std::string json_parser::JSONValue::ToString(bool pretty) const
{
	BuildManager tmp(*this);
	tmp.m_ident = pretty ? "  " : "";
	tmp.m_newLine = pretty ? "\n" : "";
	std::string res;
	tmp.Build(res);
	return res;
}
