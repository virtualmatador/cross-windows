//
//  web.h
//  cross
//
//  Created by Ali Asadpoor on 9/21/21.
//  Copyright © 2020 Shaidin. All rights reserved.
//

#ifndef WINDOWS_WEB_H
#define WINDOWS_WEB_H

#include <mutex>
#include <queue>

#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>

struct LoadWebViewDispatch
{
    const std::int32_t sender;
    const std::int32_t view_info;
    const char* html;
};

class WebWidget
{
public:
    WebWidget();
    ~WebWidget();
    void push_load(const std::int32_t sender, const std::int32_t view_info, const char* html);
    void evaluate(const char* function);
    void destroy();
    void pop_load();
    void resize();

private:
    void on_load(const std::int32_t sender, const std::int32_t view_info, const char* html);
    HRESULT OnCreateEnvironmentCompleted(HRESULT errorCode,
        ICoreWebView2Environment* createdEnvironment);
    HRESULT OnCreateCoreWebView2ControllerCompleted(HRESULT errorCode,
        ICoreWebView2Controller* webView);
    HRESULT OnNavigationCompleted(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args);
    HRESULT OnMessageReceived(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args);
    HRESULT OnResourceRequested(ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args);

private:
    std::mutex dispatch_lock_;
    std::queue<LoadWebViewDispatch> dispatch_queue_;
    Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_webViewEnvironment;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_controller;
    Microsoft::WRL::ComPtr<ICoreWebView2_3> m_webView;
    const char* html_;
};

#endif // WINDOWS_WEB_H
