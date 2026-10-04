#ifndef PAD_INFO_H
#define PAD_INFO_H

#include <SDL2/SDL.h>

#include <string>

/**
 * @brief O que se sabe de um controle além dos botões: nome, barramento (USB, Bluetooth,
 * virtual) e VID:PID. Serve para diagnóstico (padprobe, mensagens no terminal) e para a
 * ponte contar ao jogo de onde veio cada controle. O jogo NÃO decide nada pelo barramento:
 * um gamepad é um gamepad (ver CONTROLES.md, "arquitetura em camadas").
 */
namespace PadInfo
{
    /** Barramento, como o SDL o grava nos primeiros 16 bits do GUID do joystick. */
    enum Bus : Uint8
    {
        BUS_UNKNOWN   = 0x00,
        BUS_USB       = 0x03,
        BUS_BLUETOOTH = 0x05,
        BUS_VIRTUAL   = 0xFF
    };

    struct Device
    {
        std::string name;
        Bus bus = BUS_UNKNOWN;
        Uint16 vendor = 0;
        Uint16 product = 0;
        std::string guid;          ///< GUID do SDL em texto (para o gamecontrollerdb)
        bool gamepad = false;      ///< o SDL tem mapeamento de gamepad para ele
        bool is_virtual = false;   ///< criado pelo programa (ponte de rede, testes)
    };

    /** Barramento gravado no GUID. */
    Bus busOf(SDL_JoystickGUID guid);

    /** "USB", "Bluetooth", "virtual" ou "?". */
    const char* busName(Bus bus);

    /** Descreve o joystick de índice @a device_index (antes de abri-lo). */
    Device describe(int device_index);

    /** "Wireless Controller [Bluetooth 054c:09cc]": uma linha para o terminal. */
    std::string summary(const Device& d);
}

#endif // PAD_INFO_H
