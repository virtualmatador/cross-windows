//
//  window.h
//  cross
//
//  Created by Ali Asadpoor on 9/21/21.
//  Copyright � 2020 Shaidin. All rights reserved.
//

#ifndef WINDOWS_WINDOW_H
#define WINDOWS_WINDOW_H

#include <filesystem>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include <Windows.h>

#include "web.h"
#include "image.h"

struct PostMessageDispatch
{
    std::int32_t sender;
    const char* id;
    const char* command;
    const char* info;
};

class Window
{
public:
    static Window* window_;

public:
    Window(HINSTANCE hInstance, int nCmdShow);
    ~Window();
    void post_restart_message();
    void async_message(std::int32_t receiver, const char* id, const char* command, const char* info);
    void prepare_web();
    void prepare_image();
    void load_view(const std::int32_t sender, const std::int32_t view_info);
    void close();
    void handle_key(WPARAM wParam);
    void on_need_restart();
    void on_post_message();

private:
    void set_paths();

public:
    std::filesystem::path assets_path_;
    std::filesystem::path config_path_;
    bool started_;
    std::int32_t sender_;
    HWND hwnd_;
    int width_;
    int height_;
    WebWidget web_view_;
    ImageWidget image_view_;

private:
    std::mutex post_message_lock_;
    std::queue<PostMessageDispatch> post_message_queue_;
    std::mutex destroy_stream_lock_;

public:
    static const UINT WM_RESTART_ = WM_USER + __LINE__;
    static const UINT WM_MESSAGE_ = WM_USER + __LINE__;
    static const UINT WM_LOAD_WEB_ = WM_USER + __LINE__;
    static const UINT WM_LOAD_IMAGE_ = WM_USER + __LINE__;
};

#endif // WINDOWS_WINDOW_H
