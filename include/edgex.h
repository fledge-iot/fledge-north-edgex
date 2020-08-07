#ifndef _EDGEX_H
#define _EDGEX_H
/*
 * Fledge EdgeX north plugin.
 *
 * Copyright (c) 2020 
 *      
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */
#include <string>
#include <vector>
#include <set>
#include <reading.h>
#include <simple_https.h>
#include <simple_http.h>

/**
 * The class that is used to implement a north plugin that can send data to
 * the EdgeX REST API and insert Fledge data into tables within EdgeX.
 */
class EdgeX
{
	public:
		EdgeX();
		~EdgeX();
		void		authenticate(const std::string& user, const std::string& password);
		void		connect();
		uint32_t	send(const std::vector<Reading *> readings);
		void		setHost(std::string host) { m_host = host; };
		void		setPort(int port) { m_port = port; };
	private:
		bool		createTable(const std::string& table);
		bool		createSchema(const std::string& schema);
		bool		post(const std::string& payload);
		unsigned long	nSecs(struct timeval *tv) { return tv->tv_sec * 1000 + tv->tv_usec; };
		bool		m_isHTTPS;
		std::string	m_url;
		SimpleHttps	*m_https;
		SimpleHttp	*m_http;
		std::string	m_host;
		int		m_port;
		std::vector<std::pair<std::string, std::string> >
				m_headers;
};
#endif
