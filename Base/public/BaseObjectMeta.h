#pragma once

#include <set>

class BaseObjectMetaTag;

class BaseObjectMeta
{
private:
	const BaseObjectMeta* m_parentClass = nullptr;
protected:
	std::set<const BaseObjectMetaTag*> m_metaTags;
	BaseObjectMeta(const BaseObjectMeta* parentClass);

public:
	BaseObjectMeta(const BaseObjectMeta& other) = delete;
	BaseObjectMeta& operator=(const BaseObjectMeta& other) = delete;

	bool IsChildOf(const BaseObjectMeta& other) const;
	bool HasTag(const BaseObjectMetaTag& metaTag) const;
};
