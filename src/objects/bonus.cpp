#include "bonus.h"
#include "../appconfig.h"

// Construtor padrão: inicializa o bônus na posição (0,0) com o tipo padrão (estrela)
Bonus::Bonus()
    : Object(0, 0, ST_BONUS_STAR)
{
    m_bonus_show_time = 0; // Tempo de exibição do bônus começa em 0
    m_show = true;         // O bônus começa visível
}

// Construtor parametrizado: inicializa o bônus na posição (x, y) e com o tipo especificado
Bonus::Bonus(double x, double y, SpriteType type)
    : Object(x, y, type)
{
    m_bonus_show_time = 0; // Tempo de exibição do bônus começa em 0
    m_show = true;         // O bônus começa visível
}

// Desenha o bônus na tela se ele estiver visível
void Bonus::draw()
{
    if(m_show) Object::draw(); // Chama o método de desenho da classe base se m_show for verdadeiro
}

// Atualiza o estado do bônus
void Bonus::update(Uint32 dt)
{
    Object::update(dt); // Atualiza o estado base do objeto

    m_bonus_show_time += dt; // Incrementa o tempo de exibição do bônus

    // Se o tempo de exibição excedeu o limite configurado, marca para remoção
    if(m_bonus_show_time > AppConfig::bonus_show_time)
        to_erase = true;

    // Controla o efeito de "piscar" do bônus:
    // Nos primeiros 3/4 do tempo, pisca mais devagar; no último 1/4, pisca mais rápido
    if(m_bonus_show_time / (m_bonus_show_time < AppConfig::bonus_show_time / 4 * 3 ? AppConfig::bonus_blink_time : AppConfig::bonus_blink_time / 2) % 2)
        m_show = true;
    else
        m_show = false;
}
