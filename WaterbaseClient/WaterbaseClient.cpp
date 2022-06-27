// Waterbase.cpp : Defines the entry point for the application.
//

#include "WaterbaseClient.h"
#include <iostream>
#include <ostream>
#include <istream>
#include <fstream>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <idl/idl.capnp.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <cstdio>

#include <boost/array.hpp>
#include <boost/asio.hpp>
using boost::asio::ip::tcp;



void sendLength(tcp::socket& socket, u_int32_t length){
	u_int32_t l = htonl(length);
	boost::asio::write(socket, boost::asio::buffer(&l, sizeof(l)) );
}



void send(tcp::socket& socket, ::capnp::MallocMessageBuilder& message){
	const auto& segements = message.getSegmentsForOutput();
	
	sendLength(socket, segements.size());
	for(auto& segement : segements){
		auto seg = segement.asChars();
		sendLength(socket, seg.size());
		boost::asio::write(socket, boost::asio::buffer(seg.asChars().begin(), seg.size()));
	}

	for(auto& seg : segements){
		for(char c: seg.asChars())
			std::cout << (int)c;
		std::cout << std::endl;
	}
}


u_int32_t readLength(tcp::socket& socket){
	u_int32_t l;
	boost::asio::read(socket, boost::asio::buffer(&l, sizeof(l)), boost::asio::transfer_exactly(sizeof(l)));
	return ntohl(l);
}

void readSegment(tcp::socket& socket, std::vector<char>& seg){
	boost::asio::read(socket, boost::asio::buffer(seg, seg.size()), boost::asio::transfer_exactly(seg.size()));
}

struct MessageReader {
    std::vector<std::vector<char>> segs;
    std::vector<kj::ArrayPtr<const capnp::word>> bufs;
    capnp::SegmentArrayMessageReader reader;

    MessageReader(std::vector<std::vector<char>> segs,
                  std::vector<kj::ArrayPtr<const capnp::word>> bufs)
        : segs(std::move(segs)),
          bufs(std::move(bufs)),
          reader(kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>>(
              (const kj::ArrayPtr<const capnp::word>*)this->bufs.data(),
              this->bufs.size())) {}

	MessageReader(MessageReader&& mr):segs(std::move(mr.segs)),
          bufs(std::move(mr.bufs)),
		  reader(kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>>(
              (const kj::ArrayPtr<const capnp::word>*)this->bufs.data(),
              this->bufs.size())) {}
};

MessageReader read(tcp::socket& socket){
	std::vector<std::vector<char>> segs;
    std::vector<kj::ArrayPtr<const capnp::word>> bufs;

	auto segements = readLength(socket);
	std::cout << "segements:" << segements << std::endl;
	for(size_t i = 0; i < segements; i++){
		auto segment_size = readLength(socket);
		std::cout <<  "segement size:" << segment_size << std::endl;
		segs.emplace_back(segment_size);
		readSegment(socket, segs.back());
	}
	
	for(auto& seg : segs){
		bufs.emplace_back((const capnp::word*)seg.data(), seg.size());
	}

	return MessageReader(std::move(segs), std::move(bufs));
}

int main(int argc, char* argv[])
{
  try
  {
    boost::asio::io_context io_context;

	char* ip = "localhost";
    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints = resolver.resolve(ip, "7777");

    tcp::socket socket(io_context);
    boost::asio::connect(socket, endpoints);

	{
		::capnp::MallocMessageBuilder message;	
		auto messageType = message.initRoot<MessageType>();
		messageType.setType(MessageType::Type::PERSON_REQUEST);
		send(socket, message);
	}

	{
		::capnp::MallocMessageBuilder message;	
		auto personRequest = message.initRoot<PersonRequest>();
		personRequest.setName("Luke");
		send(socket, message);
	}

	{
		auto response = read(socket).reader.getRoot<PersonResponse>();
		std::cout << response.getEmail().cStr() << std::endl;

	}
	
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}

