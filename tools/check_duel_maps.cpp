// Verifica os mapas do modo duelo com as mesmas regras que o jogo usa (DuelLayout).
//
//   make check-maps            (compila e roda a partir de build/bin)
//   ./check_duel_maps [arquivo...]
//
// Sem argumentos, verifica todos os mapas listados em duel_levels/maps.txt e avisa sobre
// arquivos da pasta que não estão na lista. Com argumentos, verifica só esses arquivos.
// Sai com código 1 se algum mapa tiver problema.

#include "../src/app_state/duel_layout.h"
#include "../src/appconfig.h"

#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <set>
#include <string>
#include <vector>

static bool check(const std::string& path, const std::string& name)
{
    std::vector<std::string> grid = DuelLayout::readMap(path);
    std::vector<std::string> problems = DuelLayout::validate(grid);
    if(problems.empty())
    {
        std::printf("ok    %-12s %s  (%d bloco(s) de pedra na zona das bases, protegidos do canhão)\n",
                    name.c_str(), path.c_str(), DuelLayout::protectedStone(grid));
        return true;
    }
    std::printf("ERRO  %-12s %s\n", name.c_str(), path.c_str());
    for(const std::string& p : problems) std::printf("        - %s\n", p.c_str());
    return false;
}

int main(int argc, char** argv)
{
    bool all_ok = true;
    if(argc > 1)
    {
        for(int i = 1; i < argc; i++) all_ok = check(argv[i], argv[i]) && all_ok;
        return all_ok ? 0 : 1;
    }

    std::string list_path = AppConfig::duel_levels_path + "maps.txt";
    std::ifstream list(list_path);
    if(!list.is_open())
    {
        std::printf("ERRO  %s não encontrado (rode a partir de build/bin)\n", list_path.c_str());
        return 1;
    }
    std::set<std::string> listed = {"maps.txt"};
    std::string line;
    while(std::getline(list, line))
    {
        while(!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if(line.empty() || line[0] == '#') continue;
        size_t sep = line.find(';');
        std::string file = line.substr(0, sep);
        std::string name = (sep == std::string::npos) ? file : line.substr(sep + 1);
        listed.insert(file);
        all_ok = check(AppConfig::duel_levels_path + file, name) && all_ok;
    }

    // Arquivos na pasta que ninguém listou (mapa novo esquecido no maps.txt)
    if(DIR* dir = opendir(AppConfig::duel_levels_path.c_str()))
    {
        while(dirent* entry = readdir(dir))
        {
            std::string file = entry->d_name;
            if(file == "." || file == ".." || listed.count(file)) continue;
            std::printf("aviso %-12s %s%s não está em maps.txt (não aparece no jogo)\n", "",
                        AppConfig::duel_levels_path.c_str(), file.c_str());
        }
        closedir(dir);
    }
    return all_ok ? 0 : 1;
}
