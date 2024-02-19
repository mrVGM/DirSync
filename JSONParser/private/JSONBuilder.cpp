#include "JSONBuilder.h"

#include "JSONValue.h"
#include "symbol.h"

#define CONSTRUCTOR_IMPL(NAME)\
NAME(json_parser::Builder& builder) :\
json_parser::IBuilder(builder)\
{\
}

namespace
{
	bool MatchSymbol(scripting::ISymbol* symbol, int numChildren, const char** childrenNames, scripting::ISymbol** childSymbols)
	{
		scripting::CompositeSymbol* cs = static_cast<scripting::CompositeSymbol*>(symbol);

		for (int i = 0; i < numChildren; ++i) {
			scripting::ISymbol* cur = cs->m_childSymbols[i];

			if (strcmp(cur->m_name.c_str(), childrenNames[i]) != 0) {
				return false;
			}

			childSymbols[i] = cur;
		}

		return true;
	}

	struct ObjectBuilder;

	struct InitialBuilder : public json_parser::IBuilder
	{
		CONSTRUCTOR_IMPL(InitialBuilder)

		ObjectBuilder* m_objectBuilder = nullptr;

		void DoBuildStep(json_parser::ValueBuilder& builder) override;
		void Dispose() override;
	};

	struct PropertiesBuilder;

	struct ObjectBuilder : public json_parser::IBuilder
	{
		CONSTRUCTOR_IMPL(ObjectBuilder)

		PropertiesBuilder* m_propsBuilder = nullptr;

		void DoBuildStep(json_parser::ValueBuilder& builder) override;
		void Dispose() override;
	};

	struct KeyValuePairBuilder;

	struct PropertiesBuilder : public json_parser::IBuilder
	{
		CONSTRUCTOR_IMPL(PropertiesBuilder)

		std::map<std::string, json_parser::JSONValue> m_result;

		std::list<KeyValuePairBuilder*> m_builders;

		void DoBuildStep(json_parser::ValueBuilder& builder) override;
		void Dispose() override;
	};

	struct ValueBuilder;

	struct KeyValuePairBuilder : public json_parser::IBuilder
	{
		CONSTRUCTOR_IMPL(KeyValuePairBuilder)

		std::pair<std::string, json_parser::JSONValue> m_result;

		ValueBuilder* m_valueBuilder = nullptr;

		void DoBuildStep(json_parser::ValueBuilder& builder) override;
		void Dispose() override;
	};

	struct ListBuilder;

	struct ValueBuilder : public json_parser::IBuilder
	{
		CONSTRUCTOR_IMPL(ValueBuilder)

		ObjectBuilder* m_objectBuilder = nullptr;
		ListBuilder* m_listBuilder = nullptr;

		void DoBuildStep(json_parser::ValueBuilder& builder) override;
		void Dispose() override;
	};

	struct ListItemsBuilder;

	struct ListBuilder : public json_parser::IBuilder
	{
		CONSTRUCTOR_IMPL(ListBuilder)

		ListItemsBuilder* m_listItems = nullptr;

		void DoBuildStep(json_parser::ValueBuilder& builder) override;
		void Dispose() override;
	};

	struct ListItemsBuilder : public json_parser::IBuilder
	{
		CONSTRUCTOR_IMPL(ListItemsBuilder)

		std::list<ValueBuilder*> m_builders;

		void DoBuildStep(json_parser::ValueBuilder& builder) override;
		void Dispose() override;
	};

	void InitialBuilder::DoBuildStep(json_parser::ValueBuilder& builder)
	{
		const char* childNames[] = { "Object" };
		scripting::ISymbol* childSymbols[_countof(childNames)];

		MatchSymbol(m_rootSymbol, _countof(childSymbols), childNames, childSymbols);

		if (!m_objectBuilder)
		{
			json_parser::Builder& tmp = builder.m_builders.emplace();
			m_objectBuilder = new ObjectBuilder(tmp);
			m_objectBuilder->m_rootSymbol = childSymbols[0];
			return;
		}

		m_generatedValues = m_objectBuilder->m_generatedValues;
		m_state = BuilderState::Done;
	}

	void InitialBuilder::Dispose()
	{
		if (m_objectBuilder)
		{
			delete m_objectBuilder;
		}

		m_objectBuilder = nullptr;
	}

	void ObjectBuilder::DoBuildStep(json_parser::ValueBuilder& builder)
	{
		{
			const char* childNames[] = { "EmptyObject" };
			scripting::ISymbol* childSymbols[_countof(childNames)];

			bool matched = MatchSymbol(m_rootSymbol, _countof(childSymbols), childNames, childSymbols);

			if (matched)
			{
				json_parser::JSONValue& val = m_generatedValues.emplace_back();
				val.m_type = json_parser::ValueType::Object;
				val.m_payload = std::map<std::string, json_parser::JSONValue>();
				m_state = BuilderState::Done;
				return;
			}
		}

		const char* childNames[] = { "{", "Properties", "}" };
		scripting::ISymbol* childSymbols[_countof(childNames)];

		MatchSymbol(m_rootSymbol, _countof(childSymbols), childNames, childSymbols);

		if (!m_propsBuilder)
		{
			json_parser::Builder& tmp = builder.m_builders.emplace();
			m_propsBuilder = new PropertiesBuilder(tmp);
			m_propsBuilder->m_rootSymbol = childSymbols[1];
			return;
		}

		json_parser::JSONValue& tmp = m_generatedValues.emplace_back();
		tmp.m_type = json_parser::ValueType::Object;
		tmp.m_payload = m_propsBuilder->m_result;
		m_state = BuilderState::Done;
	}

	void ObjectBuilder::Dispose()
	{
		if (m_propsBuilder)
		{
			delete m_propsBuilder;
		}

		m_propsBuilder = nullptr;
	}

	void PropertiesBuilder::DoBuildStep(json_parser::ValueBuilder& builder)
	{
		if (!m_builders.empty())
		{
			for (auto it = m_builders.begin(); it != m_builders.end(); ++it)
			{
				KeyValuePairBuilder* cur = *it;
				m_result.insert(cur->m_result);
			}
			m_state = BuilderState::Done;
			return;
		}

		scripting::ISymbol* cur = m_rootSymbol;
		while (true)
		{
			{
				const char* childNames[] = { "KeyValuePair" };
				scripting::ISymbol* childSymbols[_countof(childNames)];

				bool matched = MatchSymbol(cur, _countof(childSymbols), childNames, childSymbols);

				if (matched)
				{
					json_parser::Builder& b = builder.m_builders.emplace();
					KeyValuePairBuilder* tmp = new KeyValuePairBuilder(b);
					tmp->m_rootSymbol = childSymbols[0];
					m_builders.emplace_front(tmp);
					break;
				}
			}

			{
				const char* childNames[] = { "Properties", ",", "KeyValuePair" };
				scripting::ISymbol* childSymbols[_countof(childNames)];

				MatchSymbol(cur, _countof(childSymbols), childNames, childSymbols);

				json_parser::Builder& b = builder.m_builders.emplace();
				KeyValuePairBuilder* tmp = new KeyValuePairBuilder(b);
				tmp->m_rootSymbol = childSymbols[2];
				m_builders.emplace_front(tmp);

				cur = childSymbols[0];
			}
		}
	}
	void PropertiesBuilder::Dispose()
	{
		for (auto it = m_builders.begin(); it != m_builders.end(); ++it)
		{
			KeyValuePairBuilder* cur = *it;
			delete cur;
		}

		m_builders.clear();
	}

	void KeyValuePairBuilder::DoBuildStep(json_parser::ValueBuilder& builder)
	{
		const char* childNames[] = { "String", ":", "Value" };
		scripting::ISymbol* childSymbols[_countof(childNames)];

		MatchSymbol(m_rootSymbol, _countof(childSymbols), childNames, childSymbols);

		if (!m_valueBuilder)
		{
			json_parser::Builder& b = builder.m_builders.emplace();
			m_valueBuilder = new ValueBuilder(b);
			m_valueBuilder->m_rootSymbol = childSymbols[2];
			return;
		}

		m_result = std::pair<std::string, json_parser::JSONValue>(childSymbols[0]->m_symbolData.m_string, m_valueBuilder->m_generatedValues.front());
		m_state = BuilderState::Done;
	}

	void KeyValuePairBuilder::Dispose()
	{
		if (m_valueBuilder)
		{
			delete m_valueBuilder;
		}

		m_valueBuilder = nullptr;
	}

	void ValueBuilder::DoBuildStep(json_parser::ValueBuilder& builder)
	{
		{
			const char* childNames[] = { "String" };
			scripting::ISymbol* childSymbols[_countof(childNames)];

			bool matched = MatchSymbol(m_rootSymbol, _countof(childSymbols), childNames, childSymbols);

			if (matched)
			{
				json_parser::JSONValue& val = m_generatedValues.emplace_back();
				val.m_type = json_parser::ValueType::String;
				val.m_payload = childSymbols[0]->m_symbolData.m_string;
				m_state = BuilderState::Done;
				return;
			}
		}

		{
			const char* childNames[] = { "Number" };
			scripting::ISymbol* childSymbols[_countof(childNames)];

			bool matched = MatchSymbol(m_rootSymbol, _countof(childSymbols), childNames, childSymbols);

			if (matched)
			{
				json_parser::JSONValue& val = m_generatedValues.emplace_back();
				val.m_type = json_parser::ValueType::Number;
				val.m_payload = childSymbols[0]->m_symbolData.m_number;
				m_state = BuilderState::Done;
				return;
			}
		}

		{
			const char* childNames[] = { "Object" };
			scripting::ISymbol* childSymbols[_countof(childNames)];

			bool matched = MatchSymbol(m_rootSymbol, _countof(childSymbols), childNames, childSymbols);

			if (matched)
			{
				if (!m_objectBuilder)
				{
					json_parser::Builder& b = builder.m_builders.emplace();
					m_objectBuilder = new ObjectBuilder(b);
					m_objectBuilder->m_rootSymbol = childSymbols[0];
					return;
				}

				m_generatedValues = m_objectBuilder->m_generatedValues;
				m_state = BuilderState::Done;
				return;
			}
		}

		{
			const char* childNames[] = { "List" };
			scripting::ISymbol* childSymbols[_countof(childNames)];

			bool matched = MatchSymbol(m_rootSymbol, _countof(childSymbols), childNames, childSymbols);

			if (matched)
			{
				if (!m_listBuilder)
				{
					json_parser::Builder& b = builder.m_builders.emplace();
					m_listBuilder = new ListBuilder(b);
					m_listBuilder->m_rootSymbol = childSymbols[0];
					return;
				}

				m_generatedValues = m_listBuilder->m_generatedValues;
				m_state = BuilderState::Done;
				return;
			}
		}
	}
	void ValueBuilder::Dispose()
	{
		if (m_objectBuilder)
		{
			delete m_objectBuilder;
		}

		if (m_listBuilder)
		{
			delete m_listBuilder;
		}

		m_objectBuilder = nullptr;
		m_listBuilder = nullptr;
	}

	void ListBuilder::DoBuildStep(json_parser::ValueBuilder& builder)
	{
		{
			const char* childNames[] = { "EmptyList" };
			scripting::ISymbol* childSymbols[_countof(childNames)];

			bool matched = MatchSymbol(m_rootSymbol, _countof(childSymbols), childNames, childSymbols);

			if (matched)
			{
				json_parser::JSONValue& val = m_generatedValues.emplace_back();
				val.m_type = json_parser::ValueType::List;
				val.m_payload = std::list<json_parser::JSONValue>();
				m_state = BuilderState::Done;
				return;
			}
		}

		const char* childNames[] = { "[", "ListItems", "]" };
		scripting::ISymbol* childSymbols[_countof(childNames)];

		MatchSymbol(m_rootSymbol, _countof(childSymbols), childNames, childSymbols);

		if (!m_listItems)
		{
			json_parser::Builder& tmp = builder.m_builders.emplace();
			m_listItems = new ListItemsBuilder(tmp);
			m_listItems->m_rootSymbol = childSymbols[1];
			return;
		}

		json_parser::JSONValue& tmp = m_generatedValues.emplace_back();
		tmp.m_type = json_parser::ValueType::List;
		tmp.m_payload = m_listItems->m_generatedValues;
		m_state = BuilderState::Done;
	}

	void ListBuilder::Dispose()
	{
		if (m_listItems)
		{
			delete m_listItems;
		}

		m_listItems = nullptr;
	}

	void ListItemsBuilder::DoBuildStep(json_parser::ValueBuilder& builder)
	{
		if (!m_builders.empty())
		{
			for (auto it = m_builders.begin(); it != m_builders.end(); ++it)
			{
				ValueBuilder* cur = *it;
				m_generatedValues.push_back(cur->m_generatedValues.front());
			}
			m_state = BuilderState::Done;
			return;
		}

		scripting::ISymbol* cur = m_rootSymbol;
		while (true)
		{
			{
				const char* childNames[] = { "Value" };
				scripting::ISymbol* childSymbols[_countof(childNames)];

				bool matched = MatchSymbol(cur, _countof(childSymbols), childNames, childSymbols);

				if (matched)
				{
					json_parser::Builder& b = builder.m_builders.emplace();
					ValueBuilder* tmp = new ValueBuilder(b);
					tmp->m_rootSymbol = childSymbols[0];
					m_builders.emplace_front(tmp);
					break;
				}
			}

			{
				const char* childNames[] = { "ListItems", ",", "Value" };
				scripting::ISymbol* childSymbols[_countof(childNames)];

				MatchSymbol(cur, _countof(childSymbols), childNames, childSymbols);

				json_parser::Builder& b = builder.m_builders.emplace();
				ValueBuilder* tmp = new ValueBuilder(b);
				tmp->m_rootSymbol = childSymbols[2];
				m_builders.emplace_front(tmp);

				cur = childSymbols[0];
			}
		}
	}

	void ListItemsBuilder::Dispose()
	{
		for (auto it = m_builders.begin(); it != m_builders.end(); ++it)
		{
			ValueBuilder* cur = *it;
			delete cur;
		}

		m_builders.clear();
	}
}

#undef CONSTRUCTOR_IMPL

json_parser::IBuilder::IBuilder(Builder& builder)
{
	builder.m_builder = this;
}

void json_parser::Builder::DoBuildStep(ValueBuilder& builder)
{
	m_builder->DoBuildStep(builder);
}

json_parser::ValueBuilder::ValueBuilder(scripting::ISymbol* rootSymbol)
{
	Builder& b = m_builders.emplace();
	m_initial = new InitialBuilder(b);
	m_initial->m_rootSymbol = rootSymbol;
}

bool json_parser::ValueBuilder::Build()
{
	if (m_builders.empty())
	{
		m_result = m_initial->m_generatedValues.front();
		delete m_initial;
		m_initial = nullptr;

		return true;
	}

	Builder& b = m_builders.top();
	if (b.m_builder->m_state == IBuilder::BuilderState::Done)
	{
		b.m_builder->Dispose();
		m_builders.pop();
		return false;
	}

	b.DoBuildStep(*this);
	return false;
}

bool json_parser::ValueBuilder::Build(JSONValue& outValue)
{
	while (!Build()) {}
	outValue = m_result;
	return true;
}
