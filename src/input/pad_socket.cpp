#include "pad_socket.h"

#include <cstring>
#include <string>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t;
#else
    #include <arpa/inet.h>
    #include <cerrno>
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

namespace
{
#ifdef _WIN32
    bool wouldBlock() { return WSAGetLastError() == WSAEWOULDBLOCK; }
    SOCKET raw(PadSocket::Handle h) { return static_cast<SOCKET>(h); }
    bool valid(SOCKET s) { return s != INVALID_SOCKET; }
#else
    bool wouldBlock() { return errno == EAGAIN || errno == EWOULDBLOCK; }
    int raw(PadSocket::Handle h) { return static_cast<int>(h); }
    bool valid(int s) { return s >= 0; }
#endif

    // Escrever num socket fechado pelo outro lado mataria o processo com SIGPIPE no Linux e
    // no macOS; queremos só o erro de volta
    void noSigPipe(PadSocket::Handle h)
    {
#if defined(SO_NOSIGPIPE)
        int on = 1;
        setsockopt(raw(h), SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof on);
#else
        (void)h;
#endif
    }

    int sendFlags()
    {
#if defined(MSG_NOSIGNAL)
        return MSG_NOSIGNAL;
#else
        return 0;
#endif
    }

    bool resolve(const char* host, int port, sockaddr_in& out)
    {
        std::memset(&out, 0, sizeof out);
        out.sin_family = AF_INET;
        out.sin_port = htons(static_cast<unsigned short>(port));
        if(host == nullptr || *host == '\0' || std::strcmp(host, "0.0.0.0") == 0)
        {
            out.sin_addr.s_addr = htonl(INADDR_ANY);
            return true;
        }
        if(inet_pton(AF_INET, host, &out.sin_addr) == 1) return true;
        addrinfo hints;
        std::memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* found = nullptr;
        if(getaddrinfo(host, nullptr, &hints, &found) != 0 || found == nullptr) return false;
        out.sin_addr = reinterpret_cast<sockaddr_in*>(found->ai_addr)->sin_addr;
        freeaddrinfo(found);
        return true;
    }
}

namespace PadSocket
{

bool startup()
{
#ifdef _WIN32
    static bool done = false;
    if(done) return true;
    WSADATA data;
    done = (WSAStartup(MAKEWORD(2, 2), &data) == 0);
    return done;
#else
    return true;
#endif
}

void setNonBlocking(Handle h)
{
#ifdef _WIN32
    u_long on = 1;
    ioctlsocket(raw(h), FIONBIO, &on);
#else
    int flags = fcntl(raw(h), F_GETFL, 0);
    fcntl(raw(h), F_SETFL, flags | O_NONBLOCK);
#endif
}

void setNoDelay(Handle h)
{
    int on = 1;
    setsockopt(raw(h), IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof on);
}

Handle listenOn(const char* host, int port)
{
    if(!startup()) return INVALID;
    sockaddr_in addr;
    if(!resolve(host, port, addr)) return INVALID;
    auto s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(!valid(s)) return INVALID;
    int on = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&on), sizeof on);
    if(bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0 || listen(s, 4) != 0)
    {
        close(static_cast<Handle>(s));
        return INVALID;
    }
    Handle h = static_cast<Handle>(s);
    setNonBlocking(h);
    return h;
}

Handle acceptPending(Handle listener)
{
    if(listener == INVALID) return INVALID;
    auto s = accept(raw(listener), nullptr, nullptr);
    if(!valid(s)) return INVALID;
    Handle h = static_cast<Handle>(s);
    // Quem aceita lê a cada quadro do jogo, sem esperar (no Linux o socket aceito não herda
    // o O_NONBLOCK do que escuta)
    setNonBlocking(h);
    noSigPipe(h);
    setNoDelay(h);
    return h;
}

Handle connectTo(const char* host, int port)
{
    if(!startup()) return INVALID;
    sockaddr_in addr;
    if(!resolve(host, port, addr)) return INVALID;
    auto s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(!valid(s)) return INVALID;
    if(connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0)
    {
        close(static_cast<Handle>(s));
        return INVALID;
    }
    Handle h = static_cast<Handle>(s);
    noSigPipe(h);
    setNoDelay(h);
    return h;
}

int receive(Handle h, void* buffer, size_t size)
{
    auto n = recv(raw(h), static_cast<char*>(buffer), static_cast<int>(size), 0);
    if(n > 0) return static_cast<int>(n);
    if(n == 0) return CLOSED;
    return wouldBlock() ? WOULD_BLOCK : CLOSED;
}

bool sendAll(Handle h, const void* data, size_t size)
{
    const char* p = static_cast<const char*>(data);
    while(size > 0)
    {
        auto n = send(raw(h), p, static_cast<int>(size), sendFlags());
        if(n <= 0)
        {
            if(n < 0 && wouldBlock()) continue;
            return false;
        }
        p += n;
        size -= static_cast<size_t>(n);
    }
    return true;
}

void close(Handle h)
{
    if(h == INVALID) return;
#ifdef _WIN32
    closesocket(raw(h));
#else
    ::close(raw(h));
#endif
}

}
