#include "BaseObjectMetaContainer.h"

namespace
{
	BaseObjectMetaContainer* m_instance = nullptr;
}

BaseObjectMetaContainer& BaseObjectMetaContainer::GetInstance()
{
	if (!m_instance)
	{
		m_instance = new BaseObjectMetaContainer();
	}
	return *m_instance;
}

void BaseObjectMetaContainer::RegisterMeta(const BaseObjectMeta* meta)
{
	m_metaRefs.push_back(meta);
}

void BaseObjectMetaContainer::RegisterMetaTag(const BaseObjectMetaTag* metaTag)
{
	m_metaTagRefs.push_back(metaTag);
}

BaseObjectMetaContainer::BaseObjectMetaContainer()
{
}

void BaseObjectMetaContainer::Shutdown()
{
	if (m_instance)
	{
		delete m_instance;
	}
	m_instance = nullptr;
}

const std::list<const BaseObjectMeta*>& BaseObjectMetaContainer::GetAllMetas() const
{
	return m_metaRefs;
}