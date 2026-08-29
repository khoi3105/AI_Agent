#pragma once

struct HttpConfig {
    long postTimeout = 60;
    long getTimeout = 10;
    int rpm = 0; // 0 = unlimited, >0 = giới hạn request / phút
};