#ifndef DEMO_H
#define DEMO_H

#include "appstate.h"

#include <functional>
#include <string>
#include <vector>

/**
 * @brief Partidas de demonstração, no fundo do menu (como o "demo" do Battle City original).
 *
 * Uma partida de um modo sorteado (campanha, duelo, sobrevivência...), com mapa e jogadores
 * sorteados e todos os tanques no computador. O menu a atualiza sem som e a desenha atrás
 * das opções, escurecida (Renderer::setComposing, Renderer::dim).
 *
 * Todo modo entra aqui com uma linha em modes() (regra D1 do MODOS_EXTRAS.md): a fábrica
 * cria o modo com os jogadores do computador (Player::cpu).
 */
namespace Demo
{
    /** Um modo na demonstração: o nome (para o terminal e os testes) e a fábrica da partida. */
    struct Mode
    {
        std::string name;
        std::function<AppState*()> create;
    };

    /** Os modos que a demonstração sorteia. */
    const std::vector<Mode>& modes();

    /**
     * Cria a partida de um modo sorteado, já depois da abertura. O modo marca a partida como
     * demonstração (Game::m_demo), que não desenha as caixas de mensagem por cima do menu.
     */
    AppState* random();
}

#endif // DEMO_H
