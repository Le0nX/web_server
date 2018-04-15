/*
 * httpServer.cpp
 *
 *  Created on: Apr 12, 2018
 *      Author: le0nx
 */
#include "httpServer.h"
#include <iostream>

HTTP_Server::HTTP_Server(uint32_t port, size_t threads_num=1)
	: _threads_num(threads_num),
	  _endpoint(ip::tcp::v4(), port),	// можно и v6
	  _acceptor(_service, _endpoint)
{}

void HTTP_Server::accept() {
	//Создаем новый сокет для нового подключения.
	//Создаем shared_ptr для передачи временных объектов ассинхронным функциям
	std::shared_ptr<ip::tcp::socket> sock(new ip::tcp::socket(_service));

	_acceptor.async_accept(*sock, [this, sock](const boost::system::error_code& error){
		//сразу же принимаем новые подключения
		accept();

		if (!error) {
			processing_request_respond(sock);
		}
	});

}

void HTTP_Server::init() {
	this->accept();

	//Пулл потокв.
	//Запуск пулла через лямбда-функцию
	for (size_t thrds=1; thrds<_threads_num; thrds++) {
		_threads.emplace_back([this](){
			_service.run();
		});
	}

	//Главный поток
	_service.run();

	//Ждем завершения остальных потоков
	for (std::thread& thr : _threads){
		thr.join();
	}
}

void HTTP_Server::processing_request_respond(std::shared_ptr<ip::tcp::socket> sock) {
	(void) sock;
	// Создаем новый read_buffer для ассинхронного async_read_until()
	// shared_ptr используем для передачи временных объектов функциям.
	std::shared_ptr<boost::asio::streambuf> read_buffer(new boost::asio::streambuf);

	async_read_until(*sock, *read_buffer, "\r\n\r\n",
			[this, sock, read_buffer](const boost::system::error_code& error, size_t bytes_transferred){
			if (!error) {
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
				//read_buffer->size() is not necessarily the same as bytes_transferred, from Boost-docs:
            //"After a successful async_read_until operation, the streambuf may contain additional data beyond the delimiter"
            //The chosen solution is to extract lines from the stream directly when parsing the header. What is left of the
            //read_buffer (maybe some bytes of the content) is appended to in the async_read-function below (for retrieving content).
            size_t total=read_buffer->size();

            //Convert to istream to extract string-lines
            std::istream stream(read_buffer.get());

            std::shared_ptr<request_t> request(new request_t());
            *request=parse_request(stream);

            size_t num_additional_bytes=total-bytes_transferred;

            //If content, read that as well
            if(request->header.count("Content-Length")>0) {
                async_read(*sock, *read_buffer, transfer_exactly(stoull(request->header["Content-Length"])-num_additional_bytes),
                [this, sock, read_buffer, request](const boost::system::error_code& ec, size_t bytes_transferred) {
                    if(!ec) {
                    	(void) bytes_transferred;
                        //Store pointer to read_buffer as istream object
                        request->payload = std::shared_ptr<std::istream>(new std::istream(read_buffer.get()));

                        respond(sock, request);
                    }
                });
            }
            else {
                respond(sock, request);
            }
        }
    });
}

request_t HTTP_Server::parse_request(std::istream& stream) {
	request_t request;

    std::regex e("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");

    std::smatch sm;

    //First parse request method, path, and HTTP-version from the first line
    std::string line;
    getline(stream, line);
    line.pop_back();
    if(regex_match(line, sm, e)) {
        request.method=sm[1];
        request.path=sm[2];
        request.version_of_http=sm[3];

        bool matched;
        e="^([^:]*): ?(.*)$";
        //Parse the rest of the header
        do {
            getline(stream, line);
            line.pop_back();
            matched=regex_match(line, sm, e);
            if(matched) {
                request.header[sm[1]]=sm[2];
            }

        } while(matched==true);
    }

    return request;
}

void HTTP_Server::respond(std::shared_ptr<ip::tcp::socket> socket, std::shared_ptr<request_t> request) {
    //Find path- and method-match, and generate response
    for(auto& res: resources) {
        std::regex e(res.first);
        std::smatch sm_res;
        if(regex_match(request->path, sm_res, e)) {
            for(auto& res_path: res.second) {
                e=res_path.first;
                std::smatch sm_path;
                if(regex_match(request->method, sm_path, e)) {
                    std::shared_ptr<boost::asio::streambuf> write_buffer(new boost::asio::streambuf);
                    std::ostream response(write_buffer.get());
                    res_path.second(response, *request, sm_res);

                    //Capture write_buffer in lambda so it is not destroyed before async_write is finished
                    async_write(*socket, *write_buffer, [this, socket, request, write_buffer](const boost::system::error_code& ec, size_t bytes_transferred) {
                        //HTTP persistent connection (HTTP 1.1):
                    	(void) bytes_transferred;
                        if(!ec && stof(request->version_of_http)>1.05)
                        	processing_request_respond(socket);
                    });
                    return;
                }
            }
        }
    }
}




