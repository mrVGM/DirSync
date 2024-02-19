#include "BaseObjectContainer.h"

#include "BaseObjectMeta.h"
#include "BaseObject.h"
#include "BaseObjectMetaContainer.h"
#include "BaseObjectContainer.h"


namespace
{
	BaseObjectContainer* m_instance = nullptr;
}

BaseObjectContainer& BaseObjectContainer::GetInstance()
{
	if (!m_instance)
	{
		m_instance = new BaseObjectContainer();
	}
	return *m_instance;
}

void BaseObjectContainer::Shutdown()
{
	if (m_instance)
	{
		delete m_instance;
	}

	m_instance = nullptr;
}

std::set<BaseObject*>& BaseObjectContainer::GetAllObjectsOfExactClass(const BaseObjectMeta& meta)
{
	CheckAccess();

	auto it = m_objects.find(&meta);

	if (it == m_objects.end())
	{
		m_objects[&meta] = std::set<BaseObject*>();
		return m_objects[&meta];
	}

	return it->second;
}

void BaseObjectContainer::RegisterObject(BaseObject& object)
{
	CheckAccess();
	const BaseObjectMeta& meta = object.GetMeta();
	std::set<BaseObject*>& objects = GetAllObjectsOfExactClass(meta);

	objects.insert(&object);
}

void BaseObjectContainer::UnregisterObject(BaseObject& object)
{
	CheckAccess();
	const BaseObjectMeta& meta = object.GetMeta();
	std::set<BaseObject*>& objects = GetAllObjectsOfExactClass(meta);

	objects.erase(&object);
}

void BaseObjectContainer::GetAllObjectsOfClass(const BaseObjectMeta& meta, std::list<BaseObject*>& objects)
{
	CheckAccess();
	const std::list<const BaseObjectMeta*>& allMetas = BaseObjectMetaContainer::GetInstance().GetAllMetas();

	for (auto metaIt = allMetas.begin(); metaIt != allMetas.end(); ++metaIt)
	{
		const BaseObjectMeta& curMeta = *(*metaIt);
		if (!curMeta.IsChildOf(meta))
		{
			continue;
		}

		const std::set<BaseObject*>& objectSet = GetAllObjectsOfExactClass(curMeta);
		for (auto objectIt = objectSet.begin(); objectIt != objectSet.end(); ++objectIt)
		{
			objects.push_back(*objectIt);
		}
	}
}

BaseObject* BaseObjectContainer::GetObjectOfClass(const BaseObjectMeta& meta)
{
	CheckAccess();
	for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		const BaseObjectMeta& curMeta = *it->first;
		if (!curMeta.IsChildOf(meta))
		{
			continue;
		}

		for (auto objectIt = it->second.begin(); objectIt != it->second.end(); ++objectIt)
		{
			return *objectIt;
		}
	}

	return nullptr;
}

BaseObjectContainer::BaseObjectContainer()
{
}

BaseObjectContainer::~BaseObjectContainer()
{
	for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		std::set<BaseObject*> curSet = it->second;

		for (auto objectIt = curSet.begin(); objectIt != curSet.end(); ++objectIt)
		{
			delete* objectIt;
		}
	}
}

void BaseObjectContainer::StartExclusiveThreadAccess()
{
	m_exclusiveThreadID = std::this_thread::get_id();
}

void BaseObjectContainer::StopExclusiveThreadAccess()
{
	m_exclusiveThreadID = std::thread::id();
}

void BaseObjectContainer::CheckAccess()
{
	static std::thread::id noThreadID;
	if (m_exclusiveThreadID == noThreadID)
	{
		return;
	}

	if (m_exclusiveThreadID != std::this_thread::get_id())
	{
		throw "Illegal Thread Access!";
	}
}