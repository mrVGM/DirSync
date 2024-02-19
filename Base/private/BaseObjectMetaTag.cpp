#include "BaseObjectMetaTag.h"

#include "BaseObjectMetaContainer.h"

BaseObjectMetaTag::BaseObjectMetaTag()
{
	BaseObjectMetaContainer& container = BaseObjectMetaContainer::GetInstance();
	container.RegisterMetaTag(this);
}
