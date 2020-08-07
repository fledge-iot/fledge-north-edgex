/*
 * Fledge EdgeX  north plugin.
 *
 * Copyright (c) 2020 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: 
 */
#include <plugin_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <logger.h>
#include <plugin_exception.h>
#include <iostream>
#include <edgex.h>
#include <simple_https.h>
#include <config_category.h>
#include <storage_client.h>
#include <rapidjson/document.h>
#include <version.h>

using namespace std;
using namespace rapidjson;

#define PLUGIN_NAME "EdgeX"

/**
 * Plugin specific default configuration
 */
static const char *default_config = QUOTE({
		"plugin": {
			"description": "EdgeX North",
			"type": "string",
			"default": PLUGIN_NAME,
			"readonly": "true"
			},
		"host": {
			"description": "The hostname of the EdgeX service",
			"type": "string",
			"default": "localhost",
			"order": "1",
			"displayName": "Hostname"
			},
		"port": {
			"description": "The port of the EdgeX core-data service",
			"type": "integer",
			"default": "48080",
			"order": "2",
			"displayName": "Port"
			},
		"username": {
			"description": "The username within EdgeX",
			"type": "string",
			"default": "",
			"order": "3",
			"displayName": "Username"
			},
		"password": {
			"description": "The password for this user",
			"type": "password",
			"default": "",
			"order": "4",
			"displayName": "Password"
			},
		"source": {
			"description": "Defines the source of the data to be sent on the stream",
			"type": "enumeration",
			"default": "readings",
			"options": ["readings", "statistics"],
		       	"order": "5",
			"displayName": "Source"
			}
	});


/**
 * The EdgeX plugin interface
 */
extern "C" {

/**
 * The C API plugin information structure
 */
static PLUGIN_INFORMATION info = {
	PLUGIN_NAME,			// Name
	VERSION,			// Version
	0,				// Flags
	PLUGIN_TYPE_NORTH,		// Type
	"1.0.0",			// Interface version
	default_config			// Configuration
};

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info()
{
	return &info;
}

/**
 * Initialise the plugin with configuration.
 *
 * This function is called to get the plugin handle.
 */
PLUGIN_HANDLE plugin_init(ConfigCategory* configData)
{
	string host = configData->getValue("host");
	int port = strtol(configData->getValue("port").c_str(), NULL, 10);
	EdgeX *edgeX = new EdgeX();
	edgeX->setHost(host);
	edgeX->setPort(port);
	edgeX->connect();
	//edgeX->authenticate(configData->getValue("username"), configData->getValue("password"));

	Logger::getLogger()->info("EdgeX plugin configured: host=%s, port=%d",
				  host.c_str(), port);

	return (PLUGIN_HANDLE)edgeX;
}

/**
 * Send Readings data to historian server
 */
uint32_t plugin_send(const PLUGIN_HANDLE handle,
		     const vector<Reading *>& readings)
{
EdgeX	*edgeX = (EdgeX *)handle;

	return edgeX->send(readings);
}

/**
 * Shutdown the plugin
 *
 * Delete allocated data
 *
 * @param handle    The plugin handle
 */
void plugin_shutdown(PLUGIN_HANDLE handle)
{
EdgeX	*edgeX = (EdgeX *)handle;

        delete edgeX;
}

// End of extern "C"
};
