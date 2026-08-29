#pragma once

struct HttpConfig {
    long postTimeout = 120;
    long getTimeout = 10;
    int rpm = 0; // 0 = unlimited, >0 = giới hạn request / phút
};