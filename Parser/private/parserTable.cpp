#include "parserTable.h"

#include <functional>

bool scripting::ParserTable::Validate()
{
	std::function<bool(const Action&, const Action&)> clash = [](const Action& a, const Action& b) {
		if (a.m_origin != b.m_origin) {
			return false;
		}

		if (a.m_symbol != b.m_symbol) {
			return false;
		}

		return true;
	};

	for (int i = 0; i < m_actions.size() - 1; i++) {
		for (int j = i + 1; j < m_actions.size(); ++j) {
			if (clash(m_actions[i], m_actions[j])) {
				return false;
			}
		}
	}
	return true;
}

