/**
 * \mainpage
 * \par Tanks
 * Jogo de tanques inspirado em Battle City / Tank 1990, permitindo jogar sozinho ou em dupla.
 * \author Krystian Kałużny
 * \date 12.05.215
 * \version 1.0
 * \par Contato:
 * \a k.kaluzny141@gmail.com
 */

#include "app.h"

// Função principal do programa.
// Inicializa a aplicação e executa o loop principal do jogo.
int main(int argc, char* args[])
{
    // Cria a instância principal da aplicação
    App app;
    // Inicia o loop principal do jogo
    app.run();

    // Retorna 0 indicando execução bem-sucedida
    return 0;
}
