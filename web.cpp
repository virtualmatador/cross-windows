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
#include <shlwapi.h>

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
    PostMessageA(Window::window_->hwnd_, Window::WM_LOAD_, 0, 0);
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
    Window::window_->load_view(sender, view_info);
    html_ = html;
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
        m_webView->SetVirtualHostNameToFolderMapping(L"asset.cross.com", (Window::window_->assets_path_ / L"assets").wstring().c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
        m_webView->AddWebResourceRequestedFilter(L"cross://*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
        m_webView->add_WebResourceRequested(Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>(this, &WebWidget::OnResourceRequested).Get(), nullptr);
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
            L"}"
            "var cross_asset_domain_ = 'https://asset.cross.com/';"
            "var cross_asset_async_ = true;"
            "var cross_pointer_type_ = 'mouse';"
            "var cross_pointer_upsidedown_ = false;"
            ;
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

HRESULT WebWidget::OnResourceRequested(ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args)
{
    Microsoft::WRL::ComPtr<ICoreWebView2WebResourceRequest> request;
    LPWSTR uri;
    Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> response;
    Microsoft::WRL::ComPtr<IStream> stream;
    int status = 0;
    if (args->get_Request(&request) == S_OK && request->get_Uri(&uri) == S_OK)
    {
        std::string uria;
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        uria = converter.to_bytes(uri);
        if (uria.rfind("cross://") == 0)
        {
            void* data = nullptr;
            std::size_t size = 0;
            cross::FeedUri(uria.c_str(), [&](const std::vector<unsigned char>& input)
            {
                size = input.size();
                data = GlobalAlloc(GMEM_FIXED, size);
                if (data)
                {
                    std::memcpy(data, input.data(), size);
                }
                else
                {
                    size = 0;
                }
            });
            if (data)
            {
                HGLOBAL hg = GlobalHandle(data);
                if (CreateStreamOnHGlobal(hg, TRUE, &stream) == S_OK)
                {
                    status = 200;
                }
                else
                {
                    status = 500;
                    GlobalFree(data);
                }
            }
            else
            {
                status = 500;
            }
        }
    }
    m_webViewEnvironment->CreateWebResourceResponse(stream.Get(), status, L"OK", nullptr, &response);
    return args->put_Response(response.Get());
}
