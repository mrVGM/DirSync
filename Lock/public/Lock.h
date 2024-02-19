#pragma once

#include "BaseObjectMeta.h"
#include "BaseObject.h"

#include <semaphore>

namespace lock
{
	class LockMeta : public BaseObjectMeta
	{
	public:
		static const LockMeta& GetInstance();
		LockMeta();
	};

	class LockObject : public BaseObject
	{
	private:
		std::binary_semaphore m_semaphore{ 1 };

	public:
		LockObject(const BaseObjectMeta& meta);
		virtual ~LockObject();

		void Lock();
		void UnLock();
	};

	class MainLockMeta : public BaseObjectMeta
	{
	public:
		static const MainLockMeta& GetInstance();
		MainLockMeta();
	};
}