#pragma once

#include <mutex>
#include "esp_http_server.h"
#include <atomic>

esp_err_t get_frame_handler(httpd_req_t *req);
