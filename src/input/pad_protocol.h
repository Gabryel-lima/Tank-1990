#ifndef PAD_PROTOCOL_H
#define PAD_PROTOCOL_H

#include <SDL2/SDL.h>

#include <cstring>
#include <string>
#include <vector>

/**
 * @brief Protocolo da ponte de controles (padbridge → jogo), sobre TCP.
 *
 * A ponte lê os controles com o SDL num sistema (o Windows, quando o jogo roda no WSL) e
 * manda o estado padronizado de cada um, já no formato de gamepad do SDL: o botão i é o
 * SDL_GameControllerButton i e o eixo i é o SDL_GameControllerAxis i. Do outro lado, o jogo
 * cria um joystick virtual do SDL para cada controle (NetPad) e o resto do jogo o vê como um
 * controle comum. Ver CONTROLES.md.
 *
 * Cada mensagem: tipo (1 byte), vaga (1 byte), tamanho do conteúdo (2 bytes) e o conteúdo.
 * Números em little-endian. Um receptor ignora tipos que não conhece (pula o conteúdo), então
 * versões novas podem acrescentar mensagens sem quebrar as antigas.
 *
 *  - HELLO  (vaga 0):  "TKPD" + versão (1 byte). Primeira mensagem da conexão.
 *  - ATTACH (vaga n):  barramento (1 byte: PadInfo::Bus), VID (2), PID (2), nome (o resto, até 64)
 *  - STATE  (vaga n):  botões (4 bytes, bit i = botão i) + 6 eixos (2 bytes cada, com sinal)
 *  - DETACH (vaga n):  sem conteúdo
 */
namespace PadProtocol
{
    const char MAGIC[4] = {'T', 'K', 'P', 'D'};
    const Uint8 VERSION = 1;

    /** Porta padrão da ponte (escolhida fora das faixas de serviços conhecidos). */
    const int DEFAULT_PORT = 47990;

    /** Controles por conexão. */
    const int MAX_PADS = 8;

    /** Tamanho máximo do nome de um controle. */
    const size_t MAX_NAME = 64;

    /** Tamanho do cabeçalho de cada mensagem. */
    const size_t HEADER = 4;

    /** Os 6 eixos do gamepad do SDL (analógicos e gatilhos). */
    const int AXES = SDL_CONTROLLER_AXIS_MAX;

    enum Type : Uint8
    {
        MSG_HELLO  = 0,
        MSG_ATTACH = 1,
        MSG_STATE  = 2,
        MSG_DETACH = 3
    };

    /** Estado padronizado de um controle. */
    struct State
    {
        Uint32 buttons = 0;           ///< bit i = SDL_GameControllerButton i apertado
        Sint16 axes[AXES] = {0};      ///< SDL_GameControllerAxis i (gatilhos de 0 a 32767)

        bool operator==(const State& o) const
        {
            return buttons == o.buttons && std::memcmp(axes, o.axes, sizeof axes) == 0;
        }
        bool operator!=(const State& o) const { return !(*this == o); }
    };

    /** Mensagem já separada do fluxo de bytes. */
    struct Message
    {
        Uint8 type = 0;
        Uint8 slot = 0;
        std::vector<Uint8> body;
    };

    // ------------------------------------------------------------------ escrita

    inline void put16(std::string& out, Uint16 v)
    {
        out.push_back(static_cast<char>(v & 0xFF));
        out.push_back(static_cast<char>(v >> 8));
    }

    inline void put32(std::string& out, Uint32 v)
    {
        put16(out, static_cast<Uint16>(v & 0xFFFF));
        put16(out, static_cast<Uint16>(v >> 16));
    }

    inline std::string frame(Uint8 type, Uint8 slot, const std::string& body)
    {
        std::string out;
        out.push_back(static_cast<char>(type));
        out.push_back(static_cast<char>(slot));
        put16(out, static_cast<Uint16>(body.size()));
        return out + body;
    }

    inline std::string hello()
    {
        return frame(MSG_HELLO, 0, std::string(MAGIC, 4) + static_cast<char>(VERSION));
    }

    inline std::string attach(Uint8 slot, Uint8 bus, Uint16 vendor, Uint16 product, std::string name)
    {
        if(name.size() > MAX_NAME) name.resize(MAX_NAME);
        std::string body;
        body.push_back(static_cast<char>(bus));
        put16(body, vendor);
        put16(body, product);
        return frame(MSG_ATTACH, slot, body + name);
    }

    inline std::string state(Uint8 slot, const State& s)
    {
        std::string body;
        put32(body, s.buttons);
        for(int i = 0; i < AXES; i++) put16(body, static_cast<Uint16>(s.axes[i]));
        return frame(MSG_STATE, slot, body);
    }

    inline std::string detach(Uint8 slot)
    {
        return frame(MSG_DETACH, slot, std::string());
    }

    // ------------------------------------------------------------------ leitura

    inline Uint16 get16(const Uint8* p) { return static_cast<Uint16>(p[0] | (p[1] << 8)); }
    inline Uint32 get32(const Uint8* p) { return get16(p) | (static_cast<Uint32>(get16(p + 2)) << 16); }

    /**
     * Junta os bytes que chegam do socket (em pedaços de qualquer tamanho) e devolve as
     * mensagens completas.
     */
    class Reader
    {
    public:
        void feed(const Uint8* data, size_t size) { m_buffer.insert(m_buffer.end(), data, data + size); }

        /** Próxima mensagem completa em @a out. @return false se ainda falta chegar algo */
        bool next(Message& out)
        {
            if(m_buffer.size() < HEADER) return false;
            size_t size = get16(&m_buffer[2]);
            if(m_buffer.size() < HEADER + size) return false;
            out.type = m_buffer[0];
            out.slot = m_buffer[1];
            out.body.assign(m_buffer.begin() + HEADER, m_buffer.begin() + HEADER + size);
            m_buffer.erase(m_buffer.begin(), m_buffer.begin() + HEADER + size);
            return true;
        }

    private:
        std::vector<Uint8> m_buffer;
    };

    /** O HELLO é desta ponte e numa versão que este receptor entende. */
    inline bool parseHello(const Message& m)
    {
        return m.type == MSG_HELLO && m.body.size() >= 5 && std::memcmp(m.body.data(), MAGIC, 4) == 0 &&
               m.body[4] == VERSION;
    }

    inline bool parseAttach(const Message& m, Uint8& bus, Uint16& vendor, Uint16& product, std::string& name)
    {
        if(m.type != MSG_ATTACH || m.body.size() < 5) return false;
        bus = m.body[0];
        vendor = get16(&m.body[1]);
        product = get16(&m.body[3]);
        name.assign(m.body.begin() + 5, m.body.end());
        if(name.size() > MAX_NAME) name.resize(MAX_NAME);
        return true;
    }

    inline bool parseState(const Message& m, State& s)
    {
        if(m.type != MSG_STATE || m.body.size() < 4 + 2 * AXES) return false;
        s.buttons = get32(&m.body[0]);
        for(int i = 0; i < AXES; i++) s.axes[i] = static_cast<Sint16>(get16(&m.body[4 + 2 * i]));
        return true;
    }
}

#endif // PAD_PROTOCOL_H
