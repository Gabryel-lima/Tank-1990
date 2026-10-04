#ifndef POWERS_H
#define POWERS_H

#include "../type.h"

#include <vector>

/**
 * @brief Regras dos poderes dos modos extras (duelo e sobrevivência), num lugar só.
 *
 * Dois tipos de poder:
 * @li **imediatos**: valem na hora em que o jogador pega o bônus (estrela, capacete, granada,
 *     reviver, reparo, trégua, escudo de equipe...);
 * @li **guardáveis**: os que dependem de *onde* ou *quando* são usados (mina, barricada,
 *     torreta, retorno, turbo). Vão para o espaço de poder do jogador e são usados com o
 *     botão de poder (Shift / LB). Enquanto guarda um poder, o jogador não pega nenhum outro
 *     bônus: para pegar outro, precisa usar o que tem.
 */
namespace Powers
{
    /** Um poder e o seu peso no sorteio dos bônus. */
    struct Weight
    {
        SpriteType type;
        int weight;
    };

    /** O poder é guardado (usado com o botão) em vez de valer na hora. */
    bool storable(SpriteType type);

    /** Um dos poderes criados para os modos extras. */
    bool isExtra(SpriteType type);

    /** Nome curto do poder, em inglês, como os nomes dos mapas ("mine", "turbo"...). */
    const char* name(SpriteType type);

    /** Poderes do duelo e pesos (a trégua não entra: no duelo ninguém "surge"). */
    const std::vector<Weight>& duelTable();

    /** Poderes da sobrevivência e pesos (todos os originais e os novos). */
    const std::vector<Weight>& survivalTable();

    /** Sorteia um poder da tabela, proporcional aos pesos. */
    SpriteType draw(const std::vector<Weight>& table);
}

#endif // POWERS_H
