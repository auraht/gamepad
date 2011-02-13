/*

GamepadChangedObserver_Windows.cpp ... Implementation of GamepadChangedObserver
for Windows.

Copyright (c) 2011  aura Human Technology Ltd.  <rnd@auraht.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, 
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
* Neither the name of "aura Human Technology Ltd." nor the names of its
  contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "GamepadChangedObserver_Windows.hpp"
#include "Gamepad_Windows.hpp"
#include "SimulatedGamepad_Windows.hpp"
#include "../Exception.hpp"

#include <unordered_map>
#include <vector>

#include "hidusage.h"
#include "hidsdi.h"
#include "Shared.hpp"
#include <SetupAPI.h>
#include <Dbt.h>
#include <tchar.h>


static struct HIDDLL2 {
    HMODULE _module;
    decltype(&::HidD_GetHidGuid) HidD_GetHidGuid;

    HIDDLL2() {
        memset(this, 0, sizeof(*this));
        _module = LoadLibrary("hid.dll");
        if (_module) {
            HidD_GetHidGuid = reinterpret_cast<decltype(HidD_GetHidGuid)>(GetProcAddress(_module, "HidD_GetHidGuid"));
        }
    }

    ~HIDDLL2() {
        if (_module)
            FreeLibrary(_module);
    }
} hid;


namespace GP {

    //---------------------------------------------------------------------------------------------------------------------

    void GamepadChangedObserver_Windows::insert_device_with_path(HWND hwnd, LPCTSTR path) {
        auto gamepad_ptr = Gamepad_Windows::insert(hwnd, path);
        if (gamepad_ptr) {
            //## TODO: Do we need a lock here?
            std::shared_ptr<Gamepad_Windows> gamepad (gamepad_ptr);
            HANDLE hdevice = gamepad_ptr->device_handle();
            _active_devices.insert(std::make_pair(hdevice, gamepad));
            this->handle_event(gamepad_ptr, GamepadState::attached);
        }
    }

    void GamepadChangedObserver_Windows::insert_simulated_gamepad(HWND hwnd) {
        _simulated_gamepad = new SimulatedGamepad_Windows(hwnd);
        this->handle_event(_simulated_gamepad, GamepadState::attached);
    }

    LRESULT CALLBACK GamepadChangedObserver_Windows::message_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        GamepadChangedObserver_Windows* this_;
        WNDPROC lastWndProc;
        
#define GET_THIS this_ = static_cast<GamepadChangedObserver_Windows*>( GetProp(hwnd, _T("com.auraHT.gamepad.this_")) )

        switch (msg) {
        case WM_NCDESTROY:
            lastWndProc = static_cast<WNDPROC>( RemoveProp(hwnd, _T("com.auraHT.gamepad.lastWndProc")) );
            this_ = static_cast<GamepadChangedObserver_Windows*>( RemoveProp(hwnd, _T("com.auraHT.gamepad.this_")) );
            this_->_hwnd = NULL;
            UnregisterDeviceNotification(this_->_notif);
            break;

        case WM_USER + 0x493f:
            reinterpret_cast<Gamepad_Windows*>(lparam)->handle_input_report(wparam);
            return TRUE;

        case WM_USER + 0x493e:
            {
                auto detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(lparam);
                GET_THIS;
                if (detail != NULL) {
                    this_->insert_device_with_path(hwnd, detail->DevicePath);
                    free(detail);
                } else {
                    this_->insert_simulated_gamepad(hwnd);
                }
                return TRUE;
            }

        case WM_KEYDOWN:
            if (lparam & 1<<30)
                // ^ make sure we don't catch autorepeated keydowns.
                goto default_case;
            else {
                // fallthrough
            }

        case WM_KEYUP:
            GET_THIS;
            if (this_->_simulated_gamepad)
                this_->_simulated_gamepad->handle_key_event(wparam, msg == WM_KEYDOWN);
            goto default_case;

        case WM_DEVICECHANGE:
        {
            PDEV_BROADCAST_HDR event_data = reinterpret_cast<PDEV_BROADCAST_HDR>(lparam);
            switch (wparam) {
            default:
                break;

            case DBT_DEVICEARRIVAL:
                // plugged in.
                if (event_data->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                    GET_THIS;
                    auto dev_path = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(event_data)->dbcc_name;
                    this_->insert_device_with_path(hwnd, dev_path);
                    return TRUE;
                }
                break;

            case DBT_DEVICEREMOVECOMPLETE:
                // unplugged.
                if (event_data->dbch_devicetype == DBT_DEVTYP_HANDLE) {
                    HANDLE hdevice = reinterpret_cast<PDEV_BROADCAST_HANDLE>(event_data)->dbch_handle;
                    //## TODO: Do we need a lock here?
                    GET_THIS;
                    auto it = this_->_active_devices.find(hdevice);
                    this_->handle_event(it->second.get(), GamepadState::detaching);
                    this_->_active_devices.erase(it);
                    return TRUE;
                }
                break;
            }
        }
        // fall-through
        
        default_case:
        default:
            lastWndProc = static_cast<WNDPROC>( GetProp(hwnd, _T("com.auraHT.gamepad.lastWndProc")) );
            break;
        }

        return CallWindowProc(lastWndProc, hwnd, msg, wparam, lparam);
    }



    void GamepadChangedObserver_Windows::populate_existing_devices(const GUID* phid_guid) {
        // RAII structure to get and destroy the list of interfaces.
        struct DevInfo {
            HDEVINFO handle;
            DevInfo(const GUID* phid_guid) 
                : handle(SetupDiGetClassDevs(phid_guid, NULL, NULL, DIGCF_PRESENT|DIGCF_INTERFACEDEVICE)) {}
            ~DevInfo() { 
                if (handle != INVALID_HANDLE_VALUE)
                    SetupDiDestroyDeviceInfoList(handle);
            }
        } dev_info (phid_guid);

        if (dev_info.handle != INVALID_HANDLE_VALUE) {
            SP_DEVICE_INTERFACE_DATA device_info_data;
            device_info_data.cbSize = sizeof(device_info_data);

            SP_DEVINFO_DATA devinfo;
            devinfo.cbSize = sizeof(devinfo);

            for (int i = 0; SetupDiEnumDeviceInterfaces(dev_info.handle, NULL, phid_guid, i, &device_info_data); ++ i) {
                // get the required length to for the detail (file path) of this device.
                DWORD required_size = 0, expected_size = 0;
                SetupDiGetDeviceInterfaceDetail(dev_info.handle, &device_info_data, NULL, 0, &expected_size, NULL);

                auto detail = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(expected_size));

                // populate the list of devices.
                detail->cbSize = sizeof(*detail);
                if (SetupDiGetDeviceInterfaceDetail(dev_info.handle, &device_info_data, detail, expected_size, &required_size, NULL)) {
                    PostMessage(_hwnd, WM_USER + 0x493e, 0, reinterpret_cast<LPARAM>(detail));
                }
            }
        }
    }

    void GamepadChangedObserver_Windows::observe_impl() {
        GUID hid_guid;
        hid.HidD_GetHidGuid(&hid_guid);

        // Hook to the WndProc of the window, so that we can handle the device changed events.
        WNDPROC _orig_wndproc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(_hwnd, GWLP_WNDPROC));

        SetProp(_hwnd, _T("com.auraHT.gamepad.lastWndProc"), _orig_wndproc);
        SetProp(_hwnd, _T("com.auraHT.gamepad.this_"), this);
        //## TODO: Throw if the aboves are already set.

        SetWindowLongPtr(_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(GamepadChangedObserver_Windows::message_handler));

        // find all already plugged-in devices.
        this->populate_existing_devices(&hid_guid);

        // insert simulated device.
        PostMessage(_hwnd, WM_USER + 0x493e, 0, 0);

        // listen for hot-plugged devices
        DEV_BROADCAST_DEVICEINTERFACE filter;
        filter.dbcc_size = sizeof(filter);
        filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        filter.dbcc_reserved = 0;
        filter.dbcc_classguid = hid_guid;

        _notif = RegisterDeviceNotification(_hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
    }

    void GamepadChangedObserver_Windows::unobserve_impl() {
        if (_hwnd) {
            WNDPROC _orig_wndproc = reinterpret_cast<WNDPROC>(RemoveProp(_hwnd, _T("com.auraHT.gamepad.lastWndProc")));
            SetWindowLongPtr(_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(_orig_wndproc));

            RemoveProp(_hwnd, _T("com.auraHT.gamepad.this_"));
        }
        delete _simulated_gamepad;
        _simulated_gamepad = NULL;
    }

    __declspec(dllexport) GamepadChangedObserver* GamepadChangedObserver::create_impl(void* self, Callback callback, void* eventloop) {
        return new GamepadChangedObserver_Windows(self, callback, static_cast<HWND>(eventloop));
    }
}

