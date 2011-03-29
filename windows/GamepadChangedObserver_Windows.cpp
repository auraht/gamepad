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

#include "../GamepadChangedObserver.hpp"
#include "../Gamepad.hpp"
#include "GamepadExtra.hpp"
#include "../Exception.hpp"

#include <unordered_map>
#include <vector>
#include <memory>

#include "hidusage.h"
#include "hidsdi.h"
#include "Shared.hpp"
#include <Windows.h>
#include <SetupAPI.h>
#include <Dbt.h>
#include <Windowsx.h>

namespace GP {
    struct GamepadChangedObserver::Impl {
        HWND _hwnd;
        HDEVNOTIFY _notif;
        std::unordered_map<HANDLE, std::shared_ptr<Gamepad>> _active_devices;
        // ^ we need to use a shared_ptr<> instead of unique_ptr<> since
        //   MSVC does not have a proper .emplace().
        Gamepad* _simulated_gamepad;

    public:
        Impl(GamepadChangedObserver* this_);
        ~Impl();

        void create_invisible_listener_window(GamepadChangedObserver* this_);
        void listen_for_hotplugged_devices();

        static LRESULT CALLBACK message_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        void populate_existing_devices(const GUID* phid_guid);
        Gamepad* insert_device_with_path(HWND hwnd, LPCTSTR path);
    };

    GP_EXPORT void GamepadChangedObserver::ImplDeleter::operator() (Impl* impl) const {
        delete impl;
    }

    GP_EXPORT void GamepadChangedObserver::create_impl(void*) {
        _impl = std::unique_ptr<Impl, ImplDeleter>(new Impl(this));
    }

    /// Create an invisible top-level window for listening to device attachment/removal and input report events.
    void GamepadChangedObserver::Impl::create_invisible_listener_window(GamepadChangedObserver* this_) {
        static struct OnceOnlyClassRegistration {
            HINSTANCE registered_hinst;

            OnceOnlyClassRegistration() : registered_hinst(GetModuleHandle(NULL)) {
                WNDCLASSEX clsdef;
                clsdef.cbSize = sizeof(clsdef);
                clsdef.style = 0;
                clsdef.lpfnWndProc = GamepadChangedObserver::Impl::message_handler;
                clsdef.cbClsExtra = 0;
                clsdef.cbWndExtra = sizeof(GamepadChangedObserver*);
                clsdef.hInstance = registered_hinst;
                clsdef.hIcon = NULL;
                clsdef.hCursor = NULL;
                clsdef.hbrBackground = NULL;
                clsdef.lpszMenuName = NULL;
                clsdef.lpszClassName = TEXT("GamepadReceiverWindow_gdwxfsqd7ocl");
                clsdef.hIconSm = NULL;

                RegisterClassEx(&clsdef);
            }
        } once;

        _hwnd = CreateWindowEx(
            0,  // dwExStyle
            TEXT("GamepadReceiverWindow_gdwxfsqd7ocl"), // class name
            TEXT("Gamepad receiver window - Do not close"), // window name
            0,  // dwStyle
            0, 0, 0, 0, // x, y, width, height
            NULL,   // parent
            NULL,   // menu
            once.registered_hinst,  // hInst
            NULL);  // lpVoid

        SetWindowLongPtr(_hwnd, 0, reinterpret_cast<LONG_PTR>(this_));
    }

    
    GamepadChangedObserver::Impl::Impl(GamepadChangedObserver* this_) : _simulated_gamepad(NULL) {
        this->create_invisible_listener_window(this_);
        
        // listen for hotplugged devices
        DEV_BROADCAST_DEVICEINTERFACE filter;
        filter.dbcc_size = sizeof(filter);
        filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        filter.dbcc_reserved = 0;
        filter.dbcc_classguid = HID::get_hid_guid();
        _notif = checked( RegisterDeviceNotification(_hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE) );

        // find all already plugged-in devices.
        this->populate_existing_devices(&filter.dbcc_classguid);
    }

    GamepadChangedObserver::Impl::~Impl() {
        _active_devices.clear();
        if (_notif) {
            UnregisterDeviceNotification(_notif);
        }
        if (_hwnd) {
            checked(DestroyWindow(_hwnd));
        }
    }

    Gamepad* GamepadChangedObserver::Impl::insert_device_with_path(HWND hwnd, LPCTSTR path) {
        GamepadData data = {hwnd, path, INVALID_HANDLE_VALUE};
        auto gamepad = std::make_shared<Gamepad>(&data);
        
        if (path == NULL || data.device_handle != INVALID_HANDLE_VALUE) {
            //## TODO: Do we need a lock here?
            _active_devices.insert(std::make_pair(data.device_handle, gamepad));
            if (path == NULL)
                _simulated_gamepad = gamepad.get();
            return gamepad.get();
        } else
            return NULL;
    }

    LRESULT CALLBACK GamepadChangedObserver::Impl::message_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        auto this_ = reinterpret_cast<GamepadChangedObserver*>( GetWindowLongPtr(hwnd, 0) );

        switch (msg) {
        default:
            break;

        case device_attached_message: 
            {
                auto detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(lparam);
                Gamepad* gamepad = detail 
                    ? this_->_impl->insert_device_with_path(hwnd, detail->DevicePath) 
                    : this_->_impl->_simulated_gamepad;
                if (gamepad != NULL)
                    this_->handle_event(*gamepad, GamepadState::attached);
                free(detail);
            }
            return TRUE;

        case report_arrived_message:
            {
                GamepadReport report = {GamepadReport::input_report};
                report.input.nanoseconds_elapsed = wparam;
                reinterpret_cast<Gamepad*>(lparam)->perform_impl_action(&report);
            }
            return TRUE;

        case WM_DEVICECHANGE:
            {
                auto event_data = reinterpret_cast<PDEV_BROADCAST_HDR>(lparam);
                switch (wparam) {
                default:
                    break;

                case DBT_DEVICEARRIVAL:
                    // plugged in.
                    if (event_data->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                        auto dev_path = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(event_data)->dbcc_name;
                        Gamepad* gamepad = this_->_impl->insert_device_with_path(hwnd, dev_path);
                        this_->handle_event(*gamepad, GamepadState::attached);
                    }
                    break;

                case DBT_DEVICEREMOVECOMPLETE:
                    // unplugged.
                    if (event_data->dbch_devicetype == DBT_DEVTYP_HANDLE) {
                        HANDLE hdevice = reinterpret_cast<PDEV_BROADCAST_HANDLE>(event_data)->dbch_handle;
                        //## TODO: Do we need a lock here?
                        auto it = this_->_impl->_active_devices.find(hdevice);
                        this_->handle_event(*(it->second), GamepadState::detaching);
                        this_->_impl->_active_devices.erase(it);
                    }
                    break;
                }
                break;
            }
        }

        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    void GamepadChangedObserver::Impl::populate_existing_devices(const GUID* phid_guid) {
        // RAII structure to get and destroy the list of interfaces.
        struct DevInfo {
            HDEVINFO handle;
            DevInfo(const GUID* phid_guid) {
                handle = checked(SetupDiGetClassDevs(phid_guid, NULL, NULL, DIGCF_PRESENT|DIGCF_INTERFACEDEVICE), INVALID_HANDLE_VALUE);
            }
            ~DevInfo() { 
                SetupDiDestroyDeviceInfoList(handle);
            }
        } dev_info (phid_guid);

        SP_DEVICE_INTERFACE_DATA device_info_data;
        device_info_data.cbSize = sizeof(device_info_data);

        SP_DEVINFO_DATA devinfo;
        devinfo.cbSize = sizeof(devinfo);

        for (int i = 0; 
            checked(SetupDiEnumDeviceInterfaces(dev_info.handle, NULL, phid_guid, i, &device_info_data), FALSE, ERROR_NO_MORE_ITEMS); 
            ++ i) 
        {
            // get the required length to for the detail (file path) of this device.
            DWORD required_size = 0, expected_size = 0;
            checked(SetupDiGetDeviceInterfaceDetail(dev_info.handle, &device_info_data, NULL, 0, &expected_size, NULL),
                    FALSE, ERROR_INSUFFICIENT_BUFFER);

            auto detail = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(expected_size));
            detail->cbSize = sizeof(*detail);
            checked(SetupDiGetDeviceInterfaceDetail(dev_info.handle, &device_info_data, detail, expected_size, &required_size, NULL));

            checked(PostMessage(_hwnd, device_attached_message, 0, reinterpret_cast<LPARAM>(detail)));
        }
    }

    GP_EXPORT Gamepad* GamepadChangedObserver::attach_simulated_gamepad() {
        return NULL;
    }



    struct GamepadChangedObserver::const_iterator::Impl {
        std::unordered_map<HANDLE, std::shared_ptr<Gamepad>>::const_iterator cit;
        
        Impl(const std::unordered_map<HANDLE, std::shared_ptr<Gamepad>>::const_iterator& it)
            : cit(it) {}
    };

    GP_EXPORT void GamepadChangedObserver::const_iterator::ImplDeleter::operator() (Impl* impl) const {
        delete impl;
    }
    
    GP_EXPORT auto GamepadChangedObserver::cbegin() const -> const_iterator {
        return const_iterator(new const_iterator::Impl(_impl->_active_devices.cbegin()));
    }
    
    GP_EXPORT auto GamepadChangedObserver::cend() const -> const_iterator {
        return const_iterator(new const_iterator::Impl(_impl->_active_devices.cend()));
    }

    GP_EXPORT auto GamepadChangedObserver::const_iterator::operator++() -> const_iterator& {
        ++ _impl->cit;
        return *this;
    }
    
    GP_EXPORT auto GamepadChangedObserver::const_iterator::operator++(int) -> const_iterator {
        auto newImpl = new Impl(_impl->cit);
        ++ _impl->cit;
        return const_iterator(newImpl);
    }
    
    GP_EXPORT Gamepad& GamepadChangedObserver::const_iterator::operator*() const {
        return *(_impl->cit->second);
    }
    
    GP_EXPORT GamepadChangedObserver::const_iterator::const_iterator(const const_iterator& other)
        : _impl(new Impl(other._impl->cit)) {}
    
    GP_EXPORT GamepadChangedObserver::const_iterator::const_iterator(const_iterator&& other)
        : _impl(std::move(other._impl)) {}

    GP_EXPORT auto GamepadChangedObserver::const_iterator::operator=(const const_iterator& other) -> const_iterator& {
        if (this != &other) {
            _impl = std::unique_ptr<Impl, ImplDeleter>(new Impl(other._impl->cit));
        }
        return *this;
    }
    GP_EXPORT auto GamepadChangedObserver::const_iterator::operator=(const_iterator&& other) -> const_iterator& {    
        if (this != &other) {
            _impl = std::move(other._impl);
        }
        return *this;
    }
    
    GP_EXPORT bool GamepadChangedObserver::const_iterator::operator==(const const_iterator& other) const {
        return _impl->cit == other._impl->cit;
    }
    
    GP_EXPORT bool GamepadChangedObserver::const_iterator::operator!=(const const_iterator& other) const {
        return _impl->cit != other._impl->cit;
    }
}

