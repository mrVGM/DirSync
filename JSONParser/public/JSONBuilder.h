#pragma once

#include "JSONValue.h"

#include <stack>
#include <list>

namespace scripting
{
	class ISymbol;
}

namespace json_parser
{
	struct JSONValue;

	struct ValueBuilder;
	struct IBuilder;

	struct Builder
	{
		IBuilder* m_builder = nullptr;
		void DoBuildStep(ValueBuilder& builder);
	};

	struct ValueBuilder
	{
		std::stack<Builder> m_builders;
		IBuilder* m_initial = nullptr;
		JSONValue m_result;

		ValueBuilder(scripting::ISymbol* rootSymbol);

		bool Build();
		bool Build(JSONValue& outValue);
	};

	struct IBuilder
	{
		enum BuilderState
		{
			Pending,
			Failed,
			Done,
		};

		scripting::ISymbol* m_rootSymbol = nullptr;
		BuilderState m_state = BuilderState::Pending;
		std::list<JSONValue> m_generatedValues;

		virtual void DoBuildStep(ValueBuilder& builder) = 0;
		virtual void Dispose() = 0;

	protected:
		IBuilder(Builder& builder);
	};
}