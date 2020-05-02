#include "interface.hh"

#include <include/cef_origin_whitelist.h>

#include <chrono>    // std::chrono::seconds
#include <iostream>  // std::cout, std::endl
#include <thread>    // std::this_thread::sleep_for

#include "app.hh"
#include "client.hh"

extern "C" RustRefApp cef_interface_add_ref_app(MyApp* ptr) {
  ptr->AddRef();

  RustRefApp r;
  r.ptr = ptr;
  return r;
}
extern "C" int cef_interface_release_ref_app(MyApp* app_ptr) {
  app_ptr->Release();
  return 0;
}

extern "C" RustRefClient cef_interface_add_ref_client(MyClient* ptr) {
  ptr->AddRef();

  RustRefClient r;
  r.ptr = ptr;
  return r;
}
extern "C" int cef_interface_release_ref_client(MyClient* client_ptr) {
  client_ptr->Release();
  return 0;
}

extern "C" RustRefBrowser cef_interface_add_ref_browser(CefBrowser* ptr) {
  ptr->AddRef();

  RustRefBrowser r;
  r.ptr = ptr;
  return r;
}
extern "C" int cef_interface_release_ref_browser(CefBrowser* browser) {
  browser->Release();
  return 0;
}

extern "C" RustRefApp cef_interface_create_app(Callbacks callbacks) {
  CefRefPtr<MyApp> app = new MyApp(callbacks);

  return cef_interface_add_ref_app(app);
}

extern "C" int cef_interface_initialize(MyApp* app_ptr) {
  // Structure for passing command-line arguments.
  // The definition of this structure is platform-specific.
  CefMainArgs main_args;

  // Populate this structure to customize CEF behavior.
  CefSettings settings;
  // sandboxing needs you to "use the same executable for the browser process
  // and all sub-processes" so we disable it
  settings.no_sandbox = true;
  settings.windowless_rendering_enabled = true;

  // fixes cef firing winproc events that cc catches
  settings.external_message_pump = true;

  // We need to have the main thread process work
  // so that it can paint
  settings.multi_threaded_message_loop = false;

  CefString(&settings.log_file).FromASCII("cef-binary.log");

#if defined(_WIN64) || defined(_WIN32)
  const char* cef_exe_path = "cef.exe";
#else
  const char* cef_exe_path = "cef";
#endif

  // Specify the path for the sub-process executable.
  CefString(&settings.browser_subprocess_path).FromASCII(cef_exe_path);

  // Initialize CEF in the main process.
  if (!CefInitialize(main_args, settings, app_ptr, NULL)) {
    return -1;
  }
  return 0;
}

extern "C" int cef_interface_execute_process() {
  rust_print("cef_interface_execute_process");

  // Provide CEF with command-line arguments.
  CefMainArgs main_args(GetModuleHandle(NULL));

  // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
  // that share the same executable. This function checks the command-line and,
  // if this is a sub-process, executes the appropriate logic.
  int exit_code = CefExecuteProcess(main_args, nullptr, nullptr);
  if (exit_code >= 0) {
    // The sub-process has completed so return here.
    rust_print("cef_interface_execute_process sub-process has completed");
    return exit_code;
  } else {
    rust_print("cef_interface_execute_process ???");
    return 0;
  }
}

// Browser

extern "C" int cef_interface_create_browser(MyClient* client_ptr,
                                            const char* startup_url) {
  // Create the browser window.
  CefWindowInfo windowInfo;
  windowInfo.SetAsWindowless(0);

  const CefString& url = startup_url;
  CefBrowserSettings settings;
  settings.windowless_frame_rate = 30;

  CefRefPtr<CefDictionaryValue> extra_info = CefDictionaryValue::Create();
  extra_info->SetInt("bap", 23);

  bool browser = CefBrowserHost::CreateBrowser(windowInfo, client_ptr, url,
                                               settings, extra_info, NULL);

  if (browser) {
    return 0;
  } else {
    return -1;
  }
}

extern "C" int cef_interface_browser_get_identifier(CefBrowser* browser) {
  return browser->GetIdentifier();
}

extern "C" int cef_interface_browser_load_url(CefBrowser* browser,
                                              const char* url) {
  auto frame = browser->GetMainFrame();
  frame->LoadURL(url);

  return 0;
}

extern "C" int cef_interface_browser_execute_javascript(CefBrowser* browser,
                                                        const char* code) {
  CefRefPtr<CefFrame> frame = browser->GetMainFrame();
  if (!frame) {
    return -1;
  }

  frame->ExecuteJavaScript(code, frame->GetURL(), 0);

  return 0;
}

extern "C" int cef_interface_browser_send_click(CefBrowser* browser,
                                                int x,
                                                int y) {
  auto browser_host = browser->GetHost();

  CefMouseEvent event = CefMouseEvent();
  event.x = x;
  event.y = y;

  browser_host->SendMouseClickEvent(
      event, CefBrowserHost::MouseButtonType::MBT_LEFT, false, 1);

  browser_host->SendMouseClickEvent(
      event, CefBrowserHost::MouseButtonType::MBT_LEFT, true, 1);

  return 0;
}

extern "C" int cef_interface_browser_send_text(CefBrowser* browser,
                                               const char* text) {
  auto browser_host = browser->GetHost();

  for (const char* c = text; *c; ++c) {
    CefKeyEvent event = CefKeyEvent();
    event.type = KEYEVENT_CHAR;
    event.character = *c;
    event.unmodified_character = *c;
    event.windows_key_code = *c;
    event.native_key_code = *c;

    browser_host->SendKeyEvent(event);
  }

  return 0;
}

extern "C" int cef_interface_browser_reload(CefBrowser* browser) {
  browser->Reload();
  return 0;
}

extern "C" int cef_interface_browser_was_resized(CefBrowser* browser) {
  browser->GetHost()->WasResized();
  return 0;
}

extern "C" int cef_interface_browser_close(CefBrowser* browser) {
  auto browser_host = browser->GetHost();

  // force_close: true because we don't want popups!
  browser_host->CloseBrowser(true);

  return 0;
}

extern "C" int cef_interface_step() {
  CefDoMessageLoopWork();
  return 0;
}

extern "C" int cef_interface_shutdown() {
  CefShutdown();
  return 0;
}
