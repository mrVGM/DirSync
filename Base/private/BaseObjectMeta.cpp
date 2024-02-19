#include "BaseObjectMeta.h"

#include "BaseObjectMetaContainer.h"

#include <iostream>

BaseObjectMeta::BaseObjectMeta(const BaseObjectMeta* parentClass) :
	m_parentClass(parentClass)
{
	BaseObjectMetaContainer& container = BaseObjectMetaContainer::GetInstance();
	container.RegisterMeta(this);
}


bool BaseObjectMeta::IsChildOf(const BaseObjectMeta& other) const
{
	const BaseObjectMeta* cur = this;
	while (cur)
	{
		if (cur == &other)
		{
			return true;
		}
		cur = cur->m_parentClass;
	}

	return false;
}

bool BaseObjectMeta::HasTag(const BaseObjectMetaTag& metaTag) const
{
	return m_metaTags.contains(&metaTag);
}