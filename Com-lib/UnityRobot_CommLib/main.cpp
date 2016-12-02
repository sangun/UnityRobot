#include <TCPSocketDataLink.hpp>
#include <IDataStreamReceiver.hpp>
#include <IPresentationProtocol.hpp>
using namespace UnityRobot;
using namespace Networking;

#include <iostream>
#include <string>
#include <mutex>
#include "RobotLogger.h"
#include <message.pb.h>
#include "MessageBuilder.h"
#include "ProtobufPresentation.h"
/*
	Just a temporary class for testing connection with unity
*/
class ReceiverSample : public IDataStreamReceiver
{
public:

	std::timed_mutex lock;
	std::string receivedData;

	virtual ~ReceiverSample() { };

	void IncomingData(const std::vector<char>& data, IDataLink* ) override
	{
		std::cout << "incoming!" << std::endl;
		
		receivedData.clear();
		receivedData.append(data.data(), data.size());
		lock.unlock();
	};
};

using Msg = Communication::Message;
using MsgBuilder = Networking::MessageBuilder;
int main(int argc, char** argv)
{
	RobotLogger logger;
	logger.init();
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	std::string address = argc > 2 ?  argv[1] : "145.93.45.16";
	std::string port = argc > 2 ? argv[2] : "1234";

	//ReceiverSample* _receiver = new ReceiverSample();
	auto _receiver = new ProtobufPresentation();
	TCPSocketDataLink link(address, port, std::unique_ptr<IDataStreamReceiver>(_receiver));

	link.Connect();

	std::cout << "Connected: " << ((link.Connected()) ? "SUCCEEDED" : "FAILED") << std::endl;

	if (link.Connected())
	{
		
		Msg toSend = MsgBuilder::create(Communication::MessageType_::IdentificationResponse, Communication::MessageTarget_::Unity, 13);
		MsgBuilder::addStringData(toSend, "stupid robot");
		MsgBuilder::addChangedShape(toSend, 3, { { 5.0, 5.5, 1.0 }, {1.0, 1.5, 1.5} });
		MsgBuilder::addChangedShape(toSend, 5, { {-5.0, -5.5, -1.0} });

		std::cout << "Send result: " << std::boolalpha << link.SendData(_receiver->MessageToBinaryData(toSend)) << '\n';

		_receiver->lock.lock();

		if(_receiver->lock.try_lock_for(std::chrono::milliseconds(4000)))
		{
			if(_receiver->receivedData.size())	
			{
				LogInfo(std::string("received " + std::to_string(_receiver->receivedData.size()) + " bytes"));
				std::vector<char> result;
				int a = 0;
				std::copy(_receiver->receivedData.begin(), _receiver->receivedData.end(), std::back_inserter(result));
				auto msg = _receiver->BinaryDataToMessage(result, a);

				std::cout << "Message received\nSize is: " << msg.ByteSize() << '\n';
				std::cout << "Target: " << msg.messagetarget() << " type: " << msg.messagetype() << '\n';

				for(int i = 0; i < msg.shapeupdateinfo().changedshapes_size(); i++)
				{
					auto shape = msg.shapeupdateinfo().changedshapes().Get(i);
					std::cout << "id: " << shape.id() << '\n';
					std::cout << "vertices are " << shape.vertices_size() << '\n';
					for(int j = 0; j < shape.vertices_size(); j++)
					{
						auto vertex = shape.vertices().Get(j);
						std::cout << "xyz: " << vertex.x() << " " << vertex.y() << " " << vertex.z() << '\n';
					}
				}


			}
			else
			{
				//std::cout << _receiver->receivedData.size();
			}
		}
		else
			std::cout << "lock" << _receiver->receivedData.size();

		_receiver->lock.unlock();

	}

	std::cout << "\nPress enter to exit.";
	std::cin.ignore(1);

	return 0;
}