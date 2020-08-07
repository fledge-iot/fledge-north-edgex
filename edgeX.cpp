/*
 * Fledge EdgeX north plugin.
 *
 * Copyright (c) 2020 EdgeX
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */
#include <edgex.h>
#include <logger.h>
#include <crypto.hpp>

using namespace	std;

/**
 * Construct the EdgeX interfaceobject
 */
EdgeX::EdgeX() :
	m_https(0), m_http(0), m_isHTTPS(false)
{
	m_headers.push_back(pair<string, string>("Content-Type", "application/json"));
}

/**
 * Destructor for the plugin
 */
EdgeX::~EdgeX()
{
	if (m_https)
	{
		delete m_https;
	}
	if (m_http)
	{
		delete m_http;
	}
}

/**
 * Generate the basic authentication credentials and append to the headers
 *
 * @param user	The username
 * @param password	The password for the user
 */
void
EdgeX::authenticate(const string& user, const string& password)
{
        string credentials;

        credentials = SimpleWeb::Crypto::Base64::encode(user + ":" + password);
	string method = "b";
	if (m_isHTTPS)
	{
		m_https->setAuthMethod(method);
		m_https->setAuthBasicCredentials(credentials);
	}
	else
	{
		m_http->setAuthMethod(method);
		m_http->setAuthBasicCredentials(credentials);
	}
}

/**
 * Create the HTTP connection to the EdgeX core-data REST API
 */
void
EdgeX::connect()
{
	char buf[80];
	snprintf(buf, sizeof(80), "%s:%d", m_host.c_str(), m_port);
	string hostAndPort = buf;
	if (m_isHTTPS)
	{
		m_https  = new SimpleHttps(hostAndPort);
	}
	else
	{
		m_http  = new SimpleHttp(hostAndPort);
	}
	m_url = "http//" + hostAndPort;
	m_url += "/v1/event";
}

/**
 * Send the readings to the EdgeX channel
 *
 * This method does the majority of the work converting the Fledge reading structure
 * to the EdgeX core-data payload for the POST operation.
 *
 * @param readings	The Readings to send
 * @return		The number of readings sent
 */
uint32_t
EdgeX::send(const vector<Reading *> readings)
{
set<string>	assets;
ostringstream	payload;
int sent = 0;

	/**
	 * Build a list of all the unique asset names we have in the current
	 * block of data we are sending to EdgeX
	 */
	for (auto it = readings.cbegin(); it != readings.cend(); ++it)
	{
		string assetName = (*it)->getAssetName();
		if (assets.find(assetName) == assets.end())
		{
			assets.emplace(assetName);
		}
	}
	/**
	 * Loop over the assets sends a POST request for all the readinfs we have for each
	 * asset.
	 *
	 * TODO Need to underatnad the tiemstamp usage inthe asset information
	 */
	for (auto itr = assets.cbegin(); itr != assets.cend(); ++itr)
	{
		string asset = *itr;
		bool	first = true;
		for (auto it = readings.cbegin(); it != readings.cend(); ++it)
		{
			string assetName = (*it)->getAssetName();
			unsigned long id = (*it)->getId();
			if (assetName.compare(asset) == 0)
			{
				if (first)
				{
					// Write the asset information once
					payload << "{ ";
					payload << "\"created\":\"0\",";
					payload << "\"device\":\"" + asset + "\",";
					payload << "\"id\":\"" << id;
				        payload << "\",";
					payload << "\"modified\":\"0\",";
					payload << "\"origin\":\"0\",";
					payload << "\"pushed\":\"0\",";
					payload << "\"readings\":[";
					first = false;
				}
				else
				{
					payload << ",";
				}
				struct timeval tm;
				(*it)->getUserTimestamp(&tm);
				vector<Datapoint *> datapoints = (*it)->getReadingData();
				for (auto dit = datapoints.cbegin(); dit != datapoints.cend();
							++dit)
				{
					string dpname = (*dit)->getName();
					DatapointValue dpvalue = (*dit)->getData();
					payload << "{";
					payload << "\"id\" : ";
					payload << (*it)->getId();
					char idStr[80];
					snprintf(idStr, sizeof(idStr), "\"%s%lu\",", dpname.c_str(), id);
					payload << idStr;
					payload << "\"origin\" : \"" << nSecs(&tm) << "\", ";
					payload << "\"pushed\" : \"0\", ";
					payload << "\"name\" : \"" << dpname << "\",";
					payload << "\"value\" : \"" << dpvalue.toString() << "\"";
					payload << "}";
				}
			}
		}
		if (!first)
		{
			// We wrote some readings so end the array of readings and post
			payload << "]}";

			if (post(payload.str()))
			{
				sent += readings.size();
			}
		}
	}
	return sent;
}

/**
 * POST a request to the EdgeX REST API
 *
 * @param payload	The payload to send
 * @return 		Returns true if the POST was accepted
 */
bool
EdgeX::post(const string& payload)
{
	try {
		int errorCode;
		if (m_isHTTPS)
		{
			errorCode = m_https->sendRequest("POST", m_url, m_headers, payload);
		}
		else
		{
			errorCode = m_http->sendRequest("POST", m_url, m_headers, payload);
		}
		if (errorCode == 200 || errorCode == 202)
		{
			return true;
		}
		else
		{
			Logger::getLogger()->error("Failed to send to EdgeX %s, errorCode %d", m_url.c_str(), errorCode);
			if (m_isHTTPS)
				Logger::getLogger()->error("HTTP response: %s for payload %s", m_https->getHTTPResponse().c_str(), payload.c_str());
			else
				Logger::getLogger()->error("HTTP response: %s for payload %s", m_http->getHTTPResponse().c_str(), payload.c_str());
		}
	} catch (runtime_error& re) {
		Logger::getLogger()->error("Failed to send to EdgeX %s, runtime error: %s", m_url.c_str(), re.what());
	} catch (exception& e) {
		Logger::getLogger()->error("Failed to send to EdgeX %s, exception occured: %s", m_url.c_str(), e.what());
	} catch (...) {
		Logger::getLogger()->error("Failed to send to EdgeX %s, unexpected exception occured", m_url.c_str());
	}
	return false;
}
