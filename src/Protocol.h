#pragma once

#include <WebSocketsClient.h>
#include <Arduino.h>


struct WebsocketConfig
{
    String websocketUrl;
    int websocketPort;
    String websocketPath;
    void (*onTextMessage)(uint8_t *payload, size_t length);
    void (*onBinaryMessage)(uint8_t *payload, size_t length);
}; typedef struct WebsocketConfig websocketConfig;


class ProtocolHandler
{
public:
    ProtocolHandler(String _websocketUrl, int _port, String _websocketPath)
        : websocketUrl(_websocketUrl), websocketPort(_port), websocketPath(_websocketPath) {}

    String websocketUrl;
    int websocketPort;
    String websocketPath;
    WebSocketsClient webSocket;
    void (*onTextMessage)(uint8_t *payload, size_t length);
    void (*onBinaryMessage)(uint8_t *payload, size_t length);

    /*
     * Setup a unsecure websocket connection.
     */
    void connect()
    {
        webSocket.begin(websocketUrl, websocketPort, websocketPath);
        webSocket.onEvent(std::bind(
            &TinyAdapter::_webSocketEvent, this, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3));
    }

    /*
     * Setup a secure websocket connection.
     */
    void connect_secure()
    {
        webSocket.beginSSL(websocketUrl.c_str(), websocketPort, websocketPath.c_str());
        webSocket.onEvent(std::bind(
            &TinyAdapter::_webSocketEvent, this, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3));
    }

    /*
     * Update the websocket connection.
     */
    void update()
    {
        webSocket.loop();
    }

    /*
     * Send a payload to the server.
     * @param {String} payload
     */
    void sendPayload(String &payload)
    {
        webSocket.sendTXT(payload.c_str(), payload.length() + 1);
    }

    /*
     * Send a binary payload to the server.
     * @param {uint8_t} payload
     * @param {size_t} length
     */
    void sendBinaryPayload(uint8_t *payload, size_t length)
    {
        webSocket.sendBIN(payload, length);
    }


    private:
    /*
     * Callback for websocket events.
     */
    void _webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
    {
        switch (type)
        {
        case WStype_DISCONNECTED:
            TA_LOG("[TA:webSocketEvent] Disconnect!\n");
            break;

        case WStype_CONNECTED:
            TA_LOG("[TA:webSocketEvent] Connected to tunnel server!\n");
            break;

        case WStype_TEXT:
        {
            TA_LOG("[TA:payloadHandler] New message received!\n");
            break;
        }

        case WStype_ERROR:
            TA_LOG("[TA:webSocketEvent] Error!\n");
            break;

        case WStype_FRAGMENT_TEXT_START:
            TA_LOG("[TA:webSocketEvent] Fragment Text Start!\n");
            break;

        case WStype_FRAGMENT_BIN_START:
            TA_LOG("[TA:webSocketEvent] Fragment Bin Start!\n");
            break;

        case WStype_FRAGMENT:
            TA_LOG("[TA:webSocketEvent] Fragment!\n");
            break;

        case WStype_FRAGMENT_FIN:
            TA_LOG("[TA:webSocketEvent] Fragment Fin!\n");
            break;

        case WStype_PING:
            TA_LOG("[TA:webSocketEvent] Ping!\n");
            break;

        case WStype_PONG:
            TA_LOG("[TA:webSocketEvent] Pong!\n");
            break;
        }
    }

};
