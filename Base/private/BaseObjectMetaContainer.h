#pragma once

#include <list>

class BaseObjectMeta;
class BaseObjectMetaTag;

class BaseObjectMetaContainer
{
	friend void BaseFrameworkShutdown();
private:
	std::list<const BaseObjectMeta*> m_metaRefs;
	std::list<const BaseObjectMetaTag*> m_metaTagRefs;
	static void Shutdown(); 

protected:

public:
	static BaseObjectMetaContainer& GetInstance();

	void RegisterMeta(const BaseObjectMeta* meta);
	void RegisterMetaTag(const BaseObjectMetaTag* metaTag);
	const std::list<const BaseObjectMeta*>& GetAllMetas() const;

	BaseObjectMetaContainer();
};
