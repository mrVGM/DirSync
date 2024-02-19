#pragma once

#include <map>
#include <set>
#include <list>
#include <thread>

class BaseObjectMeta;
class BaseObject;

class BaseObjectContainer
{
	friend void BaseFrameworkShutdown();
private:
	std::map<const BaseObjectMeta*, std::set<BaseObject*>> m_objects;
	static void Shutdown();
	std::set<BaseObject*>& GetAllObjectsOfExactClass(const BaseObjectMeta& meta);

	std::thread::id m_exclusiveThreadID;

	void CheckAccess();

protected:

public:
	static BaseObjectContainer& GetInstance();

	void RegisterObject(BaseObject& object);
	void UnregisterObject(BaseObject& object);

	void GetAllObjectsOfClass(const BaseObjectMeta& meta, std::list<BaseObject*>& objects);
	BaseObject* GetObjectOfClass(const BaseObjectMeta& meta);

	BaseObjectContainer();
	~BaseObjectContainer();

	void StartExclusiveThreadAccess();
	void StopExclusiveThreadAccess();
};
