#pragma once

class BaseObjectMeta;

class BaseObject
{
private:
	const BaseObjectMeta& m_meta;
protected:
	BaseObject(const BaseObjectMeta& meta);
public:
	BaseObject(const BaseObject& other) = delete;
	BaseObject& operator=(const BaseObject& other) = delete;

	const BaseObjectMeta& GetMeta() const;
	bool CanCastTo(BaseObjectMeta& meta);

	virtual ~BaseObject();
};