//
//  window.cpp
//  cross
//
//  Created by Ali Asadpoor on 9/21/21.
//  Copyright � 2020 Shaidin. All rights reserved.
//

#include <cstring>
#include <fstream>
#include <sstream>
#include <string_view>

#include "extern/core/src/bridge.h"
#include "extern/core/src/cross.h"

#include "window.h"

Window* Window::window_;

LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CHAR:
    {
        Window::window_->handle_key(wParam);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SHOWWINDOW:
        if (wParam)
        {
            cross::Create();
            if (!Window::window_->started_)
            {
                Window::window_->started_ = true;
                cross::Start();
            }
        }
        else
        {
            if (Window::window_->started_)
            {
                Window::window_->started_ = false;
                cross::Stop();
            }
            cross::Destroy();
        }
        break;
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
        {
            if (Window::window_->started_)
            {
                Window::window_->started_ = false;
                cross::Stop();
            }
        }
        else
        {
            if (!Window::window_->started_)
            {
                Window::window_->started_ = true;
                cross::Start();
            }
        }
        Window::window_->width_ = LOWORD(lParam);
        Window::window_->height_ = HIWORD(lParam);
        Window::window_->web_view_.resize();
        Window::window_->image_view_.resize();
        break;
    case Window::WM_RESTART_:
        Window::window_->on_need_restart();
        break;
    case Window::WM_MESSAGE_:
        Window::window_->on_post_message();
        break;
    case Window::WM_LOAD_WEB_:
        Window::window_->web_view_.pop_load();
        break;
    case Window::WM_LOAD_IMAGE_:
        Window::window_->image_view_.pop_load();
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void register_class(HINSTANCE hInstance)
{
    WNDCLASSEXA cex;

    cex.cbSize = sizeof(WNDCLASSEX);

    cex.style = CS_HREDRAW | CS_VREDRAW;
    cex.lpfnWndProc = wnd_proc;
    cex.cbClsExtra = 0;
    cex.cbWndExtra = 0;
    cex.hInstance = hInstance;
    cex.hIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(108));
    cex.hCursor = nullptr;
    cex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cex.lpszMenuName = "";
    cex.lpszClassName = PROJECT_NAME;
    cex.hIconSm = LoadIconA(hInstance, MAKEINTRESOURCEA(108));
    RegisterClassExA(&cex);
}

Window::Window(HINSTANCE hInstance, int nCmdShow)
    : started_{ false }
    , sender_{ 0 }
    , width_{ 0 }
    , height_{ 0 }
{
    window_ = this;
    set_paths();
    cross::Begin();
    register_class(hInstance);
    hwnd_ = CreateWindowA(PROJECT_NAME, PROJECT_NAME, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);
}

Window::~Window()
{
    cross::End();
}

void Window::post_restart_message()
{
    PostMessageA(hwnd_, WM_RESTART_, 0, 0);
}

void Window::async_message(std::int32_t receiver, const char* id, const char* command, const char* info)
{
    post_message_lock_.lock();
    post_message_queue_.push({ receiver, id, command, info });
    post_message_lock_.unlock();
    PostMessageA(hwnd_, WM_MESSAGE_, 0, 0);
}

void Window::prepare_web()
{
    image_view_.destroy();
}

void Window::prepare_image()
{
    web_view_.destroy();
}

void Window::load_view(const std::int32_t sender, const std::int32_t view_info)
{
    sender_ = sender;
}

void Window::set_paths()
{
    char path[MAX_PATH];
    HMODULE hModule = GetModuleHandle(NULL);
    if (hModule != NULL)
    {
        GetModuleFileName(hModule, path, sizeof(path));
    }
    else
    {
        strcpy(path, ".");
    }
    assets_path_ = std::filesystem::path(path).parent_path().parent_path() / "share";
    if (config_path_.empty())
    {
        const char* home_path = getenv("USERPROFILE");
        if (home_path)
        {
            config_path_ = home_path;
        }
    }
    if (config_path_.empty())
    {
        const char* home_drive = getenv("HOMEDRIVE");
        const char* home_path = getenv("HOMEPATH");
        if (home_drive && home_path)
        {
            config_path_ = std::string(home_drive) + std::string(home_path);
        }
    }
    if (config_path_.empty())
    {
        config_path_ = ".";
    }
    config_path_.append(PROJECT_NAME);
    config_path_.append("config");
    std::filesystem::create_directories(config_path_);
}


void Window::on_post_message()
{
    post_message_lock_.lock();
    auto dispatch_info = post_message_queue_.front();
    post_message_queue_.pop();
    post_message_lock_.unlock();
    cross::HandleAsync(dispatch_info.sender, dispatch_info.id, dispatch_info.command, dispatch_info.info);
}

void Window::handle_key(WPARAM wParam)
{
    if (wParam == VK_ESCAPE)
    {
        cross::Escape();
    }
}

void Window::on_need_restart()
{
    cross::Restart();
}

void Window::close()
{
    PostQuitMessage(0);
}
