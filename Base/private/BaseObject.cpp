#include "BaseObject.h"

#include "BaseObjectMeta.h"
#include "BaseObjectContainer.h"

BaseObject::BaseObject(const BaseObjectMeta& meta) :
	m_meta(meta)
{
	BaseObjectContainer::GetInstance().RegisterObject(*this);
}

const BaseObjectMeta& BaseObject::GetMeta() const
{
	return m_meta;
}

BaseObject::~BaseObject()
{
	BaseObjectContainer::GetInstance().UnregisterObject(*this);
}

bool BaseObject::CanCastTo(BaseObjectMeta& meta)
{
	const BaseObjectMeta& myMeta = GetMeta();
	bool res = myMeta.IsChildOf(meta);

	return res;
}