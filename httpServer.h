/**
 * httpServer.h
 *
 * Простой Web Server.
 *
 *  Created on: Apr 12, 2018
 *      Author: le0nx
 *
 *  Необходимая информация:
 *  	https://habrahabr.ru/post/50147/  		Общая информация по REST API
 *  	http://codeforces.com/blog/entry/4710	Общая информация по хеш-таблицам.
 *		https://habrahabr.ru/post/192284/		Общая информация по Boos::asio Network Programming
 */

#ifndef WEB_SERVER_HTTPSERVER_H_
#define WEB_SERVER_HTTPSERVER_H_

#include <thread>
#include <vector>
#include <stdint.h>
#include <unordered_map>
#include <regex>
#include <boost/asio.hpp>

#define BOOST_ASIO_SEPARATE_COMPILATION // ускоряем сборку.

using namespace boost::asio;

/**
 * Структура самого запроса.
 */
struct request_t {
	/* METHOD URI HTTP/VERSION*/
	std::string method;
	std::string path;
	std::string version_of_http;

	std::shared_ptr<std::istream> payload;
	/* HEADERS */
	std::unordered_map<std::string, std::string> header;
};


class HTTP_Server {
public:
	/**
	 * Конструктор класса.
	 * \param[in] port			Порт.
	 * \param[in] treads_num	Количество потоков.
	 */
	HTTP_Server(uint32_t port, size_t threads_num);

	/**
	 * Запуск сервера.
	 */
	void init();

	/**
	 * Хештаблица, в которой лежат строка и хеш-таблица, в которой лежат строка и функция-хендлер
	 */
	std::unordered_map<std::string,
					   std::unordered_map<std::string,
					   	   	   	   	   	  std::function<void(std::ostream&,
					   	   	   	   	   			  	  	  	 const request_t&,
															 const std::smatch&)
													   >
										 >
					  > resources;
private:

	void accept();

	void respond(std::shared_ptr<ip::tcp::socket> sock, std::shared_ptr<request_t> request);

	request_t parse_request(std::istream& stream);

	void processing_request_respond(std::shared_ptr<ip::tcp::socket> sock);

	size_t _threads_num;
	std::vector <std::thread> _threads;

	io_service			_service;		///< Сервис ввода/вывода
	ip::tcp::endpoint 	_endpoint;		///< Куда подключаемся.
	ip::tcp::acceptor	_acceptor;		///< Приемник клиентских соединений соединений

};

#endif /* WEB_SERVER_HTTPSERVER_H_ */
