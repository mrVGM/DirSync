#pragma once

class BaseObjectMetaTag
{
protected:
	BaseObjectMetaTag();

public:
	BaseObjectMetaTag(const BaseObjectMetaTag& other) = delete;
	BaseObjectMetaTag& operator=(const BaseObjectMetaTag& other) = delete;
};
