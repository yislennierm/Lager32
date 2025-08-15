#include "web_server.h"
#include <ESPAsyncWebServer.h>
#include "sd_card.h"
#include "status_led.h"
#include "WebContent.h"  // INDEX_HTML + optional ELRS_CSS/MUI_JS/SCAN_JS

static AsyncWebServer server(80);
static StreamBufferHandle_t sBuf;
static QueueHandle_t sCmdQ;

static void sendGz(AsyncWebServerRequest* req, const char* mime,
                   const uint8_t* data, size_t len) {
  auto* res = req->beginResponse_P(200, mime, data, len);
  res->addHeader("Content-Encoding", "gzip");
  res->addHeader("Cache-Control", "no-store");
  req->send(res);
}

void WebUi::begin(StreamBufferHandle_t buf, QueueHandle_t cmdQ) {
  sBuf = buf; sCmdQ = cmdQ;

  // Root HTML
  server.on("/", [](AsyncWebServerRequest* r){
    sendGz(r, "text/html", (const uint8_t*)INDEX_HTML, sizeof(INDEX_HTML));
  });

  // Optional assets (only register if compiled in)

  server.on("/elrs.css", [](AsyncWebServerRequest* r){
    sendGz(r, "text/css", (const uint8_t*)ELRS_CSS, sizeof(ELRS_CSS));
  });


  server.on("/mui.js", [](AsyncWebServerRequest* r){
    sendGz(r, "application/javascript", (const uint8_t*)MUI_JS, sizeof(MUI_JS));
  });


  server.on("/scan.js", [](AsyncWebServerRequest* r){
    sendGz(r, "application/javascript", (const uint8_t*)SCAN_JS, sizeof(SCAN_JS));
  });


  // Controls
  server.on("/log/start", HTTP_POST, [](AsyncWebServerRequest* r){
    SdLogCommand c{SdLogCommand::Start};
    xQueueSend(sCmdQ, &c, 0);
    LedStatus::setMode(LedMode::LOGGING);
    r->send(200);
  });
  server.on("/log/stop", HTTP_POST, [](AsyncWebServerRequest* r){
    SdLogCommand c{SdLogCommand::Stop};
    xQueueSend(sCmdQ, &c, 0);
    LedStatus::setMode(LedMode::AP_READY);
    r->send(200);
  });
  server.on("/log/mark", HTTP_POST, [](AsyncWebServerRequest* r){
    SdLogCommand c{SdLogCommand::Mark};
    xQueueSend(sCmdQ, &c, 0);
    r->send(200);
  });

  // A tiny status endpoint for your front-end
  server.on("/status.json", HTTP_GET, [](AsyncWebServerRequest* r){
    r->send(200, "application/json",
            SdLog::isLogging() ? "{\"logging\":true}" : "{\"logging\":false}");
  });

  // Favicon (avoid 500s in logs)
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* r){ r->send(204); });

  server.begin();
}
