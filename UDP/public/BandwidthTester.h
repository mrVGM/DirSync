#pragma once

#include "UDP.h"

#include "Job.h"

#include "BaseObject.h"
#include "BaseObjectMeta.h"

namespace udp
{
	class BandWidthTesterJSMeta : public BaseObjectMeta
	{
	public:
		static const BandWidthTesterJSMeta& GetInstance();
		BandWidthTesterJSMeta();
	};

	class BandWidthTesterMeta : public BaseObjectMeta
	{
	public:
		static const BandWidthTesterMeta& GetInstance();
		BandWidthTesterMeta();
	};

	class BandWidthTesterObject : public BaseObject, public Endpoint
	{
	public:
		std::string m_serverIP;
		int m_serverPort = -1;

		size_t m_received = 0;

		BandWidthTesterObject(
			int serverPort,
			const std::string& serverIP);

		virtual ~BandWidthTesterObject();

		void Init() override;

		void StartTest(size_t numPackets, size_t delay, jobs::Job* done);
	};
}