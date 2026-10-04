// Verifica os mapas dos modos extras com as mesmas regras que o jogo usa:
// duelo (DuelLayout) e sobrevivência (SurvivalLayout).
//
//   make check-maps                      (compila e roda a partir de build/bin)
//   ./check_duel_maps [arquivo...]       só esses mapas de duelo
//   ./check_duel_maps --survival arquivo...  só esses mapas de sobrevivência
//
// Sem argumentos, verifica todos os mapas listados em duel_levels/maps.txt e em
// survival_levels/maps.txt e avisa sobre arquivos das pastas que não estão na lista.
// Sai com código 1 se algum mapa tiver problema.

#include "../src/app_state/duel_layout.h"
#include "../src/app_state/survival_layout.h"
#include "../src/appconfig.h"

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <functional>
#include <set>
#include <string>
#include <vector>

using Validator = std::function<std::vector<std::string>(const std::vector<std::string>&)>;

static bool check(const std::string& path, const std::string& name, const Validator& validate, bool duel)
{
    std::vector<std::string> grid = DuelLayout::readMap(path);
    std::vector<std::string> problems = validate(grid);
    if(problems.empty())
    {
        if(duel)
            std::printf("ok    %-12s %s  (%d bloco(s) de pedra na zona das bases, protegidos do canhão)\n",
                        name.c_str(), path.c_str(), DuelLayout::protectedStone(grid));
        else
            std::printf("ok    %-12s %s\n", name.c_str(), path.c_str());
        return true;
    }
    std::printf("ERRO  %-12s %s\n", name.c_str(), path.c_str());
    for(const std::string& p : problems) std::printf("        - %s\n", p.c_str());
    return false;
}

// Todos os mapas listados no maps.txt da pasta, mais o aviso dos arquivos que ninguém listou
static bool checkFolder(const std::string& folder, const std::string& title, const Validator& validate, bool duel)
{
    std::printf("== %s (%smaps.txt)\n", title.c_str(), folder.c_str());
    std::string list_path = folder + "maps.txt";
    std::ifstream list(list_path);
    if(!list.is_open())
    {
        std::printf("ERRO  %s não encontrado (rode a partir de build/bin)\n", list_path.c_str());
        return false;
    }
    bool all_ok = true;
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
        all_ok = check(folder + file, name, validate, duel) && all_ok;
    }

    if(DIR* dir = opendir(folder.c_str()))
    {
        while(dirent* entry = readdir(dir))
        {
            std::string file = entry->d_name;
            if(file == "." || file == ".." || listed.count(file)) continue;
            std::printf("aviso %-12s %s%s não está em maps.txt (não aparece no jogo)\n", "", folder.c_str(), file.c_str());
        }
        closedir(dir);
    }
    return all_ok;
}

int main(int argc, char** argv)
{
    bool all_ok = true;
    if(argc > 1)
    {
        bool survival = std::strcmp(argv[1], "--survival") == 0;
        Validator validate = survival ? Validator(SurvivalLayout::validate) : Validator(DuelLayout::validate);
        for(int i = survival ? 2 : 1; i < argc; i++) all_ok = check(argv[i], argv[i], validate, !survival) && all_ok;
        return all_ok ? 0 : 1;
    }

    all_ok = checkFolder(AppConfig::duel_levels_path, "Duelo", DuelLayout::validate, true) && all_ok;
    all_ok = checkFolder(AppConfig::survival_levels_path, "Sobrevivência", SurvivalLayout::validate, false) && all_ok;
    return all_ok ? 0 : 1;
}
