#include "JobSystemMeta.h"

namespace
{
	jobs::JobSystemMeta m_instance;
	jobs::MainJobSystemMeta m_mainJS;
	jobs::AsyncJobSystemMeta m_asyncJS;
}

jobs::JobSystemMeta::JobSystemMeta() :
	BaseObjectMeta(nullptr)
{
}

jobs::MainJobSystemMeta::MainJobSystemMeta() :
	BaseObjectMeta(&JobSystemMeta::GetInstance())
{
}

jobs::AsyncJobSystemMeta::AsyncJobSystemMeta() :
	BaseObjectMeta(&JobSystemMeta::GetInstance())
{
}

const jobs::JobSystemMeta& jobs::JobSystemMeta::GetInstance()
{
	return m_instance;
}

const jobs::MainJobSystemMeta& jobs::MainJobSystemMeta::GetInstance()
{
	return m_mainJS;
}

const jobs::AsyncJobSystemMeta& jobs::AsyncJobSystemMeta::GetInstance()
{
	return m_asyncJS;
}