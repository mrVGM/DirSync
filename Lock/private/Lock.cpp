#include "Lock.h"

namespace
{
	lock::LockMeta m_instance;
	lock::MainLockMeta m_mainLock;
}

lock::LockMeta::LockMeta() :
	BaseObjectMeta(nullptr)
{
}

const lock::LockMeta& lock::LockMeta::GetInstance()
{
	return m_instance;
}

lock::LockObject::LockObject(const BaseObjectMeta& meta) :
	BaseObject(meta)
{
}

lock::LockObject::~LockObject()
{
}

void lock::LockObject::Lock()
{
	m_semaphore.acquire();
}

void lock::LockObject::UnLock()
{
	m_semaphore.release();
}

const lock::MainLockMeta& lock::MainLockMeta::GetInstance()
{
	return m_mainLock;
}

lock::MainLockMeta::MainLockMeta() :
	BaseObjectMeta(&LockMeta::GetInstance())
{
}
