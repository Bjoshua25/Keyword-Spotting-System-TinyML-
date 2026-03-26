#include "WitAiChunkedUploader.h"
#include <ArduinoJson.h>

WitAiChunkedUploader::WitAiChunkedUploader(const char *access_key)
{
    m_wifi_client = new WiFiClientSecure();
    m_wifi_client->setInsecure(); // Required for ESP32 SSL bypass
    
    if (m_wifi_client->connect("api.wit.ai", 443)) {
        char authorization_header[100];
        snprintf(authorization_header, 100, "authorization: Bearer %s", access_key);
        
        m_wifi_client->println("POST /speech?v=20240304 HTTP/1.1");
        m_wifi_client->println("host: api.wit.ai");
        m_wifi_client->println(authorization_header);
        m_wifi_client->println("content-type: audio/raw;encoding=signed-integer;bits=16;rate=16000;endian=little");
        m_wifi_client->println("transfer-encoding: chunked");
        m_wifi_client->println();
    }
}

bool WitAiChunkedUploader::connected()
{
    return m_wifi_client && m_wifi_client->connected();
}

void WitAiChunkedUploader::startChunk(int size_in_bytes)
{
    m_wifi_client->printf("%X\r\n", size_in_bytes);
}

void WitAiChunkedUploader::sendChunkData(const uint8_t *data, int size_in_bytes)
{
    m_wifi_client->write(data, size_in_bytes);
}

void WitAiChunkedUploader::finishChunk()
{
    m_wifi_client->print("\r\n");
}

Intent WitAiChunkedUploader::getResults()
{
    // Finalize the request
    m_wifi_client->print("0\r\n\r\n");

    int status = -1;
    // Skip headers and get status
    while (m_wifi_client->connected())
    {
        String line = m_wifi_client->readStringUntil('\n');
        if (line == "\r") break; 
        if (line.startsWith("HTTP/1.1")) {
            status = line.substring(9, 12).toInt();
        }
    }

    Serial.printf("HTTP Status: %d\n", status);
    Intent results;

    if (status == 200)
    {
        StaticJsonDocument<1024> filter; // Increased size for safety
        filter["entities"] = true;
        filter["text"] = true;
        filter["is_final"] = true;

        DynamicJsonDocument doc(4096);
        deserializeJson(doc, *m_wifi_client, DeserializationOption::Filter(filter));

        if (!doc["text"].isNull()) {
            results.text = doc["text"].as<std::string>();
        }

        // Mapping based on your Entity requirements
        JsonObject entities = doc["entities"];
        if (entities.containsKey("name:name")) {
            results.intent_name = "NAME_QUERY"; 
        } 
        else if (entities.containsKey("you:you")) {
            results.intent_name = "STATUS_QUERY";
        }
        else if (entities.containsKey("time:time")) {
            results.intent_name = "TIME_QUERY";
        }
    }
    return results;
}

WitAiChunkedUploader::~WitAiChunkedUploader()
{
    if (m_wifi_client) {
        m_wifi_client->stop();
        delete m_wifi_client;
    }
}