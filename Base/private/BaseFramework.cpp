#include "BaseFramework.h"

#include "BaseObjectMetaContainer.h"
#include "BaseObjectContainer.h"

void BaseFrameworkShutdown()
{
	BaseObjectContainer::Shutdown();
	BaseObjectMetaContainer::Shutdown();
}