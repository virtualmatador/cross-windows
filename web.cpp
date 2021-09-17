//
//  web.cpp
//  cross
//
//  Created by Ali Asadpoor on 10/12/20.
//  Copyright © 2020 Shaidin. All rights reserved.
//

#include <codecvt>
#include <string>
#include <sstream>

#include <wrl/event.h>

#include "extern/core/src/cross.h"

#include "window.h"

#include "web.h"

WebWidget::WebWidget()
    : html_{ nullptr }
{
}

WebWidget::~WebWidget()
{
    destroy();
}

void WebWidget::push_load(const std::int32_t sender, const std::int32_t view_info,
    const char* html)
{
    dispatch_lock_.lock();
    dispatch_queue_.push({sender, view_info, html});
    dispatch_lock_.unlock();
    PostMessageA(Window::window_->hwnd_, Window::WM_LOAD_WEB_, 0, 0);
}

void WebWidget::evaluate(const char* function)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wfunction = converter.from_bytes(function);
    m_webView->ExecuteScript(wfunction.c_str(), nullptr);
}

void WebWidget::destroy()
{
    if (m_controller)
    {
        m_controller->Close();
        m_controller = nullptr;
        m_webView = nullptr;
    }
    m_webViewEnvironment = nullptr;
}

void WebWidget::pop_load()
{
    dispatch_lock_.lock();
    auto dispatch_info = dispatch_queue_.front();
    dispatch_queue_.pop();
    dispatch_lock_.unlock();
    on_load(dispatch_info.sender, dispatch_info.view_info, dispatch_info.html);
}

void WebWidget::resize()
{
    if (m_controller)
    {
        m_controller->put_Bounds(RECT{ 0, 0, Window::window_->width_, Window::window_->height_ });
    }
}

void WebWidget::on_load(const std::int32_t sender, const std::int32_t view_info, const char* html)
{
    destroy();
    Window::window_->prepare_web();
    Window::window_->load_view(sender, view_info);
    html_ = html;

    // @Override public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request)
    // {
    //     Intent intent = new Intent(Intent.ACTION_VIEW, request.getUrl());
    //     view.getContext().startActivity(intent);
    //     return true;
    // }
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    auto hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, Window::window_->config_path_.wstring().c_str(), options.Get(),
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(this, &WebWidget::OnCreateEnvironmentCompleted).Get());
    if (hr != S_OK)
    {
        throw std::runtime_error("CreateCoreWebView2EnvironmentWithOptions");
    }
}

HRESULT WebWidget::OnCreateEnvironmentCompleted(HRESULT errorCode, ICoreWebView2Environment* createdEnvironment)
{
    m_webViewEnvironment = createdEnvironment;

    if (m_webViewEnvironment->CreateCoreWebView2Controller(
        Window::window_->hwnd_,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            this, &WebWidget::OnCreateCoreWebView2ControllerCompleted)
        .Get()) != S_OK)
    {
        throw std::runtime_error("");
    }
    return S_OK;
}

HRESULT WebWidget::OnCreateCoreWebView2ControllerCompleted(HRESULT errorCode, ICoreWebView2Controller* webView)
{
    auto hr = errorCode;
    if (hr == S_OK)
    {
        m_controller = webView;
        Microsoft::WRL::ComPtr<ICoreWebView2> coreWebView2;
        if (m_controller->get_CoreWebView2(&coreWebView2) != S_OK || coreWebView2.As(&m_webView) != S_OK)
        {
            throw std::runtime_error("");
        }
        resize();
        m_webView->add_WebMessageReceived(Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(this, &WebWidget::OnMessageReceived).Get(), nullptr);
        m_webView->add_NavigationCompleted(Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(this, &WebWidget::OnNavigationCompleted).Get(), nullptr);
        std::wstring path = L"file://" + (Window::window_->assets_path_ / "assets" / (std::string(html_) + ".htm")).wstring();
        auto hr = m_webView->Navigate(path.c_str());
    }
    return hr;
}

HRESULT WebWidget::OnNavigationCompleted(ICoreWebView2* core_sender, ICoreWebView2NavigationCompletedEventArgs* args)
{
    BOOL success;
    if (args->get_IsSuccess(&success) == S_OK && success)
    {
        std::wostringstream os;
        os <<
            L"var Handler = window.chrome.webview;"
            L"var Handler_Receiver = "
            << Window::window_->sender_ << ";"
            L"function CallHandler(id, command, info)"
            L"{"
            L"    Handler.postMessage(Handler_Receiver.toString() + \" \" + id + \" \" + command + \" \" + info);"
            L"}";
        core_sender->ExecuteScript(os.str().c_str(), Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>([this](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT
        {
            if (errorCode == S_OK)
            {
                cross::HandleAsync(Window::window_->sender_, "body", "ready", "");
            }
            return errorCode;
        }).Get());
    }
    return S_OK;
}

HRESULT WebWidget::OnMessageReceived(ICoreWebView2* core_sender, ICoreWebView2WebMessageReceivedEventArgs* args)
{
    wchar_t* buffer = nullptr;
    if (args->TryGetWebMessageAsString(&buffer) != S_OK)
    {
        throw std::runtime_error("");
    }
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::string msg = converter.to_bytes(buffer);
    std::istringstream is{ msg };
    std::int32_t sender = 0;
    std::string id, command, info;
    is >> sender;
    is >> id;
    is >> command;
    is.ignore(1);
    std::getline(is, info);
    cross::HandleAsync(sender, id.c_str(), command.c_str(), info.c_str());
    return S_OK;
}
