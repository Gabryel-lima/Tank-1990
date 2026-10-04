#ifndef PAD_SOCKET_H
#define PAD_SOCKET_H

#include <cstddef>
#include <cstdint>

/**
 * @brief TCP mínimo e portátil para a ponte de controles: Winsock no Windows, sockets BSD
 * no Linux e no macOS. Só o que a ponte usa: escutar, aceitar, conectar, enviar e receber
 * sem bloquear.
 */
namespace PadSocket
{
    using Handle = std::intptr_t;
    const Handle INVALID = -1;

    /** Resultado de receive() quando não chegou nada e a conexão continua aberta. */
    const int WOULD_BLOCK = -1;
    /** Resultado de receive() quando a conexão caiu. */
    const int CLOSED = 0;

    /** Inicializa a rede (WSAStartup no Windows; nada nos outros). Pode chamar várias vezes. */
    bool startup();

    /** Socket que escuta em @a host:@a port, sem bloquear. INVALID se falhar. */
    Handle listenOn(const char* host, int port);

    /** Aceita uma conexão pendente (sem bloquear). INVALID se não houver nenhuma. */
    Handle acceptPending(Handle listener);

    /** Conecta a @a host:@a port (bloqueia até conectar ou falhar). INVALID se falhar. */
    Handle connectTo(const char* host, int port);

    /** Deixa o socket sem bloquear nas leituras. */
    void setNonBlocking(Handle h);

    /** Desliga o algoritmo de Nagle: cada estado sai na hora (menos latência). */
    void setNoDelay(Handle h);

    /** Recebe o que houver: > 0 bytes lidos, CLOSED se a conexão caiu, WOULD_BLOCK se nada chegou. */
    int receive(Handle h, void* buffer, size_t size);

    /** Envia tudo (bloqueia se preciso). false se a conexão caiu. */
    bool sendAll(Handle h, const void* data, size_t size);

    void close(Handle h);
}

#endif // PAD_SOCKET_H
