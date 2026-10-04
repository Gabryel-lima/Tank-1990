#include "pad_info.h"

#include <cstdio>

namespace PadInfo
{

Bus busOf(SDL_JoystickGUID guid)
{
    // Little-endian, como o SDL grava (SDL_CreateJoystickGUID)
    Uint16 bus = static_cast<Uint16>(guid.data[0] | (guid.data[1] << 8));
    switch(bus)
    {
    case BUS_USB:       return BUS_USB;
    case BUS_BLUETOOTH: return BUS_BLUETOOTH;
    case BUS_VIRTUAL:   return BUS_VIRTUAL;
    default:            return BUS_UNKNOWN;
    }
}

const char* busName(Bus bus)
{
    switch(bus)
    {
    case BUS_USB:       return "USB";
    case BUS_BLUETOOTH: return "Bluetooth";
    case BUS_VIRTUAL:   return "virtual";
    default:            return "?";
    }
}

Device describe(int device_index)
{
    Device d;
    const char* name = SDL_JoystickNameForIndex(device_index);
    d.name = name ? name : "?";
    SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(device_index);
    d.bus = busOf(guid);
    d.vendor = SDL_JoystickGetDeviceVendor(device_index);
    d.product = SDL_JoystickGetDeviceProduct(device_index);
    char text[33];
    SDL_JoystickGetGUIDString(guid, text, sizeof text);
    d.guid = text;
    d.gamepad = SDL_IsGameController(device_index) == SDL_TRUE;
#if SDL_VERSION_ATLEAST(2, 0, 14)
    d.is_virtual = SDL_JoystickIsVirtual(device_index) == SDL_TRUE;
#endif
    return d;
}

std::string summary(const Device& d)
{
    char ids[16];
    std::snprintf(ids, sizeof ids, "%04x:%04x", d.vendor, d.product);
    return d.name + " [" + busName(d.bus) + " " + ids + "]";
}

}
