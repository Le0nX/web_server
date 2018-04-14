/*
 * httpServer.cpp
 *
 *  Created on: Apr 12, 2018
 *      Author: le0nx
 */
#include "httpServer.h"


HTTP_Server::HTTP_Server(uint32_t port, size_t threads_num)
	: _threads_num(threads_num),
	  _acceptor(_service, _endpoint),
	  _endpoint(ip::tcp::v4(), port)	// можно и v6
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

}




