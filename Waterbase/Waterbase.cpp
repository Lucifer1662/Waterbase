// Waterbase.cpp : Defines the entry point for the application.
//

#include "Waterbase.h"

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <idl/idl.capnp.h>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <charconv>
#include <fstream>
#include <iostream>
#include <istream>
#include <list>
#include <ostream>
#include <string>
#include <string_view>
// #include <boost/asio/co_spawn.hpp>
#include <boost/asio/coroutine.hpp>
// #include <boost/asio/experimental/co_spawn.hpp>
#include <boost/asio/yield.hpp>
#include <cstdio>
#include <optional>

using boost::asio::ip::tcp;

void Get(std::string path, std::ostream& os) {
    std::ifstream is(path);
    if (is.is_open()) {
        char c;
        while (is.get(c)) {
            os.put(c);
        }
    }
}

void Put(std::string path, std::istream& is) {
    std::ofstream os(path);
    if (os.is_open()) {
        char c;
        while (is.get(c)) {
            os.put(c);
        }
    }
}

// struct MessageReader{
//     std::vector<std::vector<char>> segs;
//     std::vector<kj::ArrayPtr<const capnp::word>> bufs;
// 	tcp::socket& socket;
// 	u_int64_t s;
// 	u_int64_t num_segments;
//
// 	MessageReader(tcp::socket& socket):socket(socket){}
//
// 	template<typename Func>
// 	void async_read_length(Func&& func){
// 		// boost::asio::async_read(
// 		// 	socket,
// 		// 	boost::asio::buffer(&(this->s), sizeof(s)),
// 		// 	boost::asio::transfer_exactly(sizeof(u_int64_t)),
// 		// 	[this, func](auto& ec, auto size){
// 		// 		if(!ec.failed())
// 		// 			func((u_int64_t)ntohl(this->s));
// 		// 	}
// 		// );
// 	}

// 	template<typename T, typename Func>
// 	void async_read(Func&& func){
// 		// async_read_length([&, this, func](auto num_segs){
// 		// 	this->num_segments = num_segs;
// 		// 	segs.reserve(num_segments);
// 		// 	next_segment([&, this, func](){
// 		// 		std::vector<kj::ArrayPtr<const capnp::word>>
// bufs;
// 		// 		for(auto& seg : segs){
// 		// 			bufs.emplace_back((const
// capnp::word*)seg.data(), seg.size());
// 		// 		}

// 		// 		kj::ArrayPtr<const kj::ArrayPtr<const
// capnp::word>> buffer((const kj::ArrayPtr<const capnp::word>*)bufs.data(),
// bufs.size());

// 		// 		capnp::SegmentArrayMessageReader reader(buffer);

// 		// 		func(reader.getRoot<T>());
// 		// 	});
// 		// });
// 	}

// 	template<typename Func>
// 	void next_segment(Func func){
// 		// if(num_segments == segs.size()){
// 		// 	func();
// 		// 	return;
// 		// }

// 		// async_read_length([&, func](auto segement_size){
// 		// 	segs.emplace_back(segement_size);

// 		// 	boost::asio::async_read(
// 		// 		socket,
// 		// 		boost::asio::buffer(segs.back().data(),
// segs.back().size()),
// 		// boost::asio::transfer_exactly(segs.back().size()),
// 		// 		[&, func](auto& ec, auto size){
// 		// 			if(!ec.failed())
// 		// 				next_segment(func);
// 		// 		}
// 		// 		);
// 		// });
// 	}

// 	~MessageReader(){
// 		std::cout << "destroyed" << std::endl;
// 	}
// };

// struct Request{
// 	Socket& socket;
// 	MessageReader reader;

// };

void sendLength(boost::asio::streambuf& buf, u_int64_t length) {
    u_int32_t l = htonl(length);
    buf.sputn((char*)&l, sizeof(l));
}

void send_async(tcp::socket& socket, ::capnp::MallocMessageBuilder& message) {
    const auto& segements = message.getSegmentsForOutput();

    auto buf = std::make_shared<boost::asio::streambuf>();

    sendLength(*buf, segements.size());

    for (auto& segement : segements) {
        auto seg = segement.asChars();
        sendLength(*buf, seg.size());
        buf->sputn(seg.begin(), seg.size());
    }

    boost::asio::async_write(socket, *buf, [buf](auto ec, auto bt) {});
}

// boost::asio::awaitable<u_int64_t> async_read_length(tcp::socket& socket) {
//     u_int64_t s;
//     std::size_t n = co_await boost::asio::async_read(
//         socket, boost::asio::buffer(&s, sizeof(s)),
//         boost::asio::transfer_exactly(sizeof(u_int64_t)),
//         boost::asio::use_awaitable);
//     // co_yeild s;//(u_int64_t)ntohl(s);
// }

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
};

template<typename Handler>
struct MessageReaderCo : boost::asio::coroutine {
    u_int32_t segements;
    u_int32_t segement_size;
    tcp::socket& socket;
    std::vector<std::vector<char>> segs;
    std::vector<kj::ArrayPtr<const capnp::word>> bufs;
	size_t i;
	Handler& handler;
	

    MessageReaderCo(tcp::socket& socket, Handler& handler) : socket(socket), handler(handler) {}

    void readLength(u_int32_t& length) {
        boost::asio::async_read(
            socket, boost::asio::buffer(&length, sizeof(length)),
            boost::asio::transfer_exactly(sizeof(length)), 
			[this, &length](auto ec, auto size){
        		length = ntohl(length);
				(*this)(ec, size);
			});
    }

	void readSegment(std::vector<char>& seg) {
		std::cout << seg.size() << std::endl;
        boost::asio::async_read(
            socket, boost::asio::buffer(seg, seg.size()),
            boost::asio::transfer_exactly(seg.size()), 
			[this](auto ec, auto size){
				(*this)(ec, size);
			});
    }

	void start(){
		(*this)();
	}

    void operator()(boost::system::error_code ec = boost::system::error_code(),
                    std::size_t n = 0) {
		
        if (!ec) reenter(this) {
			yield readLength(segements);
			for(; i < segements; i++){
				yield readLength(segement_size);
				segs.emplace_back(segement_size);
				yield readSegment(segs.back());
			}
			
			for(auto& seg : segs){
				bufs.emplace_back((const capnp::word*)seg.data(),
				seg.size());
			}

			handler();
		}
    }

	capnp::SegmentArrayMessageReader Reader(){
		return capnp::SegmentArrayMessageReader(kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>>(
              (const kj::ArrayPtr<const capnp::word>*)this->bufs.data(),
              this->bufs.size()));
	}
	
};

// boost::asio::awaitable<std::optional<MessageReader>> echo(tcp::socket& socket) {
//     try {
//         u_int64_t s;
//         auto num_segements = co_await async_read_length(socket);
//         std::vector<std::vector<char>> segs;
//         for (size_t i = 0; i < num_segements; i++) {
//             auto segement_size = co_await async_read_length(socket);
//             segs.emplace_back(segement_size);
//         }

//         std::vector<kj::ArrayPtr<const capnp::word>> bufs;
//         for (auto& seg : segs) {
//             bufs.emplace_back((const capnp::word*)seg.data(), seg.size());
//         }
//         // return MessageReader(std::move(segs), std::move(bufs));
//     } catch (std::exception& e) {
		    
//         std::printf("echo Exception: %s\n", e.what());
//     }
//     //   return {};
// }

struct Socket 
: boost::asio::coroutine 
 {
    std::unique_ptr<tcp::socket> socket;
    std::unique_ptr<MessageReaderCo<Socket>> reader;
	MessageType::Type type;


    Socket(std::unique_ptr<tcp::socket> socket)
        : socket(std::move(socket)){}

    void start_reading() {
		(*this)();
    }

    void operator()(boost::system::error_code ec = boost::system::error_code(), std::size_t n = 0) {
		if (!ec) reenter(this) {
			for(;;){
				reader.reset( new MessageReaderCo(*this->socket, *this));
				yield reader->start();
				type = reader->Reader().getRoot<MessageType>().getType();
				reader.reset( new MessageReaderCo(*this->socket, *this));
				yield reader->start();
				process_request(type);
			}
		}
	}

    void process_request(MessageType::Type type) {
        ::capnp::MallocMessageBuilder responseData;
        switch (type)
        {
        case MessageType::Type::PERSON_REQUEST:
			{
				auto request = reader->Reader().getRoot<PersonRequest>();
        		auto response = responseData.getRoot<PersonResponse>();
        		proccess_person(request, response);
        		break;
        	}
        default:
        	return;
        }

    	send_async(*socket, responseData);

    }

    void proccess_person(PersonRequest::Reader& message,
                         ::PersonResponse::Builder& response) {
        std::cout << message.getName().cStr() << std::endl;
        response.setEmail("alice@gmal.com");
    }
};

struct Server {
    tcp::acceptor acceptor;
    boost::asio::io_context& io_context;
    std::list<Socket> sockets;

    Server(boost::asio::io_context& io_context)
        : acceptor(io_context, {tcp::v4(), 7777}), io_context(io_context) {}

    void start_accepting() {
        auto socket = new tcp::socket(io_context);

        acceptor.async_accept(*socket, [socket, this](auto ec) {
            if (ec.failed()) {
                free(socket);
            } else {
                std::cout << "Accepted Connection" << std::endl;
                this->sockets.emplace_back(
                    std::unique_ptr<tcp::socket>(socket));
                auto it = std::prev(sockets.end());
                socket->async_wait(
                    boost::asio::ip::tcp::socket::wait_error,
                    [it, this](const boost::system::error_code& ec) {
                        this->sockets.erase(it);
                    });
                this->sockets.back().start_reading();
                start_accepting();
            }
        });
    }
};

int main() {
    try {
        boost::asio::io_context io_context(1);
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) { io_context.stop(); });

        Server server(io_context);

        server.start_accepting();

        io_context.run();
        std::cout << "exiting";
    } catch (std::exception& e) {
        std::printf("Exception: %s\n", e.what());
    }
}
