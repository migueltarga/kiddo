#pragma once

#include <Arduino.h>

// Saved copies of the bundled Portuguese stories predate the narration field.
// These keys match the recordings shipped in stories/audio/ and let those
// copies show a play button without asking families to clear their library.
namespace builtin_narration {

inline String path_for(const String &story_file, const String &story_id, const String &node_key)
{
    static const char *const adventure[] = {
        "inicio", "bosque", "flores", "borboleta_fala", "ajuda_borboleta",
        "fim_brinquedos", "sozinhos", "lago", "fim_lago", "pedras", "sapo",
        "resposta_certa", "fim_jardim", "resposta_errada", "fim_descanso"
    };
    static const char *const moon[] = {
        "inicio", "cozinha", "cozinha_pesado", "quintal", "nozes",
        "nozes_errou", "nozes_acertou", "floresta", "atalho", "caverna",
        "caverna_lanterna", "caverna_pena", "cristais", "cristais_banana",
        "cristais_lua", "pocao", "coruja", "coruja_errou", "coruja_acertou",
        "ponte", "ponte_mel", "ponte_pena", "ponte_pes", "urso_adivinha",
        "urso_errou", "urso_acertou", "lago", "sapo", "sapo_errou",
        "sapo_acertou", "montanha", "portao", "escada", "escada_errou",
        "escada_acertou", "topo", "topo_errou", "final_brilho", "final_musica",
        "final_abraco"
    };
    static const char *const key[] = {
        "inicio", "ver_mapa", "caminho_esquerda", "pegar_espada",
        "caminho_direita", "passagem_secreta", "inventario_guardiao",
        "derrotar_guardiao", "tentar_porta", "inventario_porta"
    };

    const char *folder = nullptr;
    const char *const *names = nullptr;
    size_t count = 0;
    if (story_file == "/story_adventure_pt.json" && story_id == "zoe_aventura") {
        folder = "story_adventure_pt";
        names = adventure;
        count = sizeof(adventure) / sizeof(adventure[0]);
    } else if (story_file == "/story_lua_perdida.json" && story_id == "zoe_lua_perdida") {
        folder = "story_lua_perdida";
        names = moon;
        count = sizeof(moon) / sizeof(moon[0]);
    } else if (story_file == "/story_the_key_pt.json" && story_id == "aventura_inventario") {
        folder = "story_the_key_pt";
        names = key;
        count = sizeof(key) / sizeof(key[0]);
    }
    for (size_t i = 0; i < count; ++i) {
        if (node_key == names[i])
            return String("audio/") + folder + "/" + node_key + ".wav";
    }
    return "";
}

} // namespace builtin_narration
