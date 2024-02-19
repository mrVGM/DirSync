#pragma once

#include <string>
#include <list>
#include <map>
#include <variant>

namespace json_parser
{
	enum ValueType
	{
		Number = 0,
		String = 1,
		List = 2,
		Object = 3
	};

	struct JSONValue;
	typedef std::variant<double, std::string, std::list<JSONValue>, std::map<std::string, JSONValue>> JSONValuePayload;

	struct JSONValue
	{
		static void FromString(const std::string& str, JSONValue& outValue);

		ValueType m_type = ValueType::Number;
		JSONValuePayload m_payload;

		JSONValue(const JSONValuePayload& payload = 0.0);
		JSONValue(ValueType type);

		std::map<std::string, JSONValue>& GetAsObj();
		std::list<JSONValue>& GetAsList();

		std::string ToString(bool pretty) const;
	};
}