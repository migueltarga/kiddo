/**
 * @file story_scene.cpp
 * @brief Animated picture at the top of story pages that have none
 *
 * The scenes, figures, props and moving bits are images drawn by
 * scripts/gen_scenes.py. The page's words pick them: the first place it
 * names, the people and animals it mentions, the things they play with.
 * Moving bits are small LVGL objects with endless animations, which LVGL
 * removes when the page deletes them.
 */

#include "story_scene.h"
#include <algorithm>
#include "../images/images.h"
#include "../../audio.h"

namespace StoryScene {

namespace {

/** @brief How a scene's small moving bits behave */
enum Effect {
    TWINKLE,    /**< Fade in and out in place: stars, crystals */
    FIREFLIES,  /**< Glowing dots wandering around */
    SHIMMER,    /**< Short light lines sliding on water */
    PETALS,     /**< Falling and swaying */
    MOTES       /**< Faint specks floating up: dust, steam */
};

struct Scene {
    const char* name;
    const lv_image_dsc_t* image;
    Effect effect;
    lv_area_t area;         /**< Where the moving bits live, in scene pixels */
    uint32_t colors[3];
    uint8_t count;
    lv_opa_t night_dim;     /**< Dark veil at night; 0 for scenes that are already dark */
    bool outdoor;           /**< Sky above: stars at night, birds, sunset light */
    int16_t prop_x1, prop_x2, prop_ground;  /**< Where props stand; no props when x1 == x2 */
    lv_area_t water;        /**< Where fish swim and props float; empty for none */
};

const Scene kScenes[] = {
    {"bedroom_night", &scene_bedroom_night, TWINKLE, {26, 14, 68, 46}, {0xFFF3D6, 0xFFF3D6, 0xFFE9A0}, 4, 0, false, 20, 96, 95, {}},
    {"bedroom_day", &scene_bedroom_day, MOTES, {90, 14, 210, 72}, {0xFFFFFF, 0xFFF3D6, 0xFFFFFF}, 5, 0, false, 20, 96, 95, {}},
    {"living_room", &scene_living_room, MOTES, {60, 10, 210, 70}, {0xFFFFFF, 0xFFF3D6, 0xFFFFFF}, 5, LV_OPA_50, false, 54, 120, 95, {}},
    {"kitchen", &scene_kitchen, MOTES, {29, 30, 41, 40}, {0xFFFFFF, 0xFFFFFF, 0xFFFFFF}, 3, LV_OPA_50, false, 62, 124, 63, {}},
    {"pool", &scene_pool, SHIMMER, {12, 60, 104, 88}, {0xFFFFFF, 0xC6F0FF, 0xFFFFFF}, 6, LV_OPA_60, true, 14, 100, 84, {14, 62, 104, 88}},
    {"classroom", &scene_classroom, MOTES, {18, 14, 118, 48}, {0xFFFFFF, 0xF4F1E6, 0xFFFFFF}, 4, LV_OPA_50, false, 24, 90, 71, {}},
    {"library", &scene_library, MOTES, {10, 10, 210, 70}, {0xFFF3D6, 0xFFFFFF, 0xFFF3D6}, 5, LV_OPA_50, false, 10, 112, 96, {}},
    {"playground", &scene_playground, PETALS, {10, 0, 200, 30}, {0xFFFFFF, 0xF5A9C8, 0xFFD45C}, 4, LV_OPA_60, true, 30, 100, 96, {}},
    {"school", &scene_school, MOTES, {10, 10, 210, 60}, {0xFFFFFF, 0xFFFFFF, 0xFFF3D6}, 4, LV_OPA_60, true, 8, 50, 96, {}},
    {"road", &scene_road, MOTES, {100, 10, 210, 50}, {0xFFFFFF, 0xFFFFFF, 0xFFFFFF}, 4, LV_OPA_60, true, 0, 0, 0, {}},
    {"garden", &scene_garden, PETALS, {10, 0, 200, 30}, {0xF5A9C8, 0xFFFFFF, 0xFFD45C}, 5, LV_OPA_60, true, 10, 110, 95, {}},
    {"forest", &scene_forest, FIREFLIES, {10, 30, 210, 80}, {0xFFE680, 0xFFF3A0, 0xE8FF9A}, 7, LV_OPA_30, true, 20, 100, 96, {}},
    {"cave", &scene_cave, TWINKLE, {12, 58, 100, 84}, {0x6FE3F0, 0xF58FD0, 0xA98BF5}, 7, 0, false, 10, 100, 93, {}},
    {"lake", &scene_lake, SHIMMER, {14, 60, 130, 78}, {0xFFFFFF, 0xF4C08A, 0xFFFFFF}, 5, LV_OPA_50, true, 10, 100, 95, {20, 60, 124, 80}},
    {"mountain", &scene_mountain, MOTES, {10, 10, 210, 70}, {0xFFFFFF, 0xFFFFFF, 0xFFFFFF}, 5, LV_OPA_60, true, 10, 100, 96, {}},
    {"sky", &scene_sky, TWINKLE, {6, 6, 214, 70}, {0xFFF6DC, 0xFFECB4, 0xFFF6DC}, 9, 0, true, 10, 100, 94, {}},
    {"beach", &scene_beach, SHIMMER, {6, 54, 214, 72}, {0xFFFFFF, 0xD6F1FB, 0xFFFFFF}, 6, LV_OPA_60, true, 10, 100, 97, {10, 54, 210, 70}},
};

struct Figure {
    const char* name;
    const lv_image_dsc_t* image;
    uint8_t hop;            /**< Jump height when tapped */
    uint16_t idle;          /**< Bob period, so they don't move in step */
    bool travels;           /**< Family, friends and pets come along to the next place; others stay in theirs */
};

const Figure kFigures[] = {
    {"zoe", &figure_zoe, 8, 1100, true}, {"ted", &figure_ted, 16, 1400, true},
    {"girl", &figure_girl, 9, 1150, true}, {"girl2", &figure_girl2, 9, 1250, true}, {"boy", &figure_boy, 10, 1050, true},
    {"mom", &figure_mom, 6, 1500, true}, {"dad", &figure_dad, 6, 1600, true}, {"grandpa", &figure_grandpa, 5, 1700, true},
    {"grandma", &figure_grandma, 5, 1650, true}, {"teacher", &figure_teacher, 6, 1550, false},
    {"dog", &figure_dog, 12, 900, true}, {"cat", &figure_cat, 12, 1000, true}, {"frog", &figure_frog, 14, 800, false},
    {"owl", &figure_owl, 8, 1300, false}, {"bear", &figure_bear, 6, 1800, false}, {"squirrel", &figure_squirrel, 14, 850, false},
    {"sheep", &figure_sheep, 10, 1200, false},
};

struct Sprite {
    const char* name;
    const lv_image_dsc_t* image;
};

const Sprite kProps[] = {
    {"float", &prop_float}, {"ball", &prop_ball}, {"sandcastle", &prop_sandcastle}, {"book", &prop_book},
    {"blocks", &prop_blocks}, {"pancakes", &prop_pancakes}, {"cake", &prop_cake}, {"food", &prop_food},
    {"paints", &prop_paints}, {"chest", &prop_chest}, {"door", &prop_door},
};

const Sprite kExtras[] = {
    {"heart", &fx_heart}, {"note", &fx_note}, {"butterfly", &fx_butterfly}, {"fish", &fx_fish}, {"bird", &fx_bird},
};

/**
 * Words, matched as whole words (Portuguese and English). A place named
 * outright moves the scene there; the first one in the text wins, so "slept
 * under a tree and woke up in the bedroom" is the bedroom. Hints only set
 * the first page's place, since a tree or a star is mentioned anywhere.
 */
const char* const kCave[] = {"caverna", "cavernas", "gruta", "túnel", "cave", "caves", "tunnel", nullptr};
const char* const kBeach[] = {"praia", "mar", "oceano", "beach", "sea", "ocean", nullptr};
const char* const kPool[] = {"piscina", "pool", nullptr};
const char* const kLake[] = {"lago", "lagoa", "rio", "ponte", "lake", "river", "bridge", "pond", nullptr};
const char* const kMountain[] = {"montanha", "montanhas", "mountain", "mountains", nullptr};
const char* const kForest[] = {"floresta", "bosque", "mata", "forest", "woods", nullptr};
const char* const kGarden[] = {"quintal", "jardim", "horta", "garden", "yard", "backyard", nullptr};
const char* const kPlayground[] = {"parquinho", "escorregador", "recreio", "pátio", "gangorra", "playground", "slide", "recess", nullptr};
const char* const kClassroom[] = {"sala de aula", "classroom", nullptr};
const char* const kLibrary[] = {"biblioteca", "library", nullptr};
const char* const kSchool[] = {"escola", "school", nullptr};
const char* const kKitchen[] = {"cozinha", "kitchen", nullptr};
const char* const kBedroom[] = {"quarto", "cama", "dormir", "dormiu", "acordou", "acordar", "acorda", "adormeceu", "adormeceram",
                                "bedroom", "bed", "sleep", "woke", "wake", nullptr};
const char* const kRoad[] = {"carro", "estrada", "viagem", "ônibus", "car", "road", "trip", "bus", nullptr};
const char* const kHome[] = {"sala", "sofá", "campainha", "casa", "living room", "sofa", "couch", "doorbell", "house", "home", nullptr};

const char* const kForestHints[] = {"árvore", "árvores", "coruja", "tree", "trees", "owl", nullptr};
const char* const kGardenHints[] = {"flor", "flores", "borboleta", "borboletas", "balanço", "cerca", "rede",
                                    "flower", "flowers", "butterfly", "butterflies", "swing", "fence", nullptr};
const char* const kBeachHints[] = {"areia", "onda", "ondas", "concha", "conchinhas", "sand", "wave", "waves", "shell", nullptr};
const char* const kLakeHints[] = {"sapo", "sapinho", "frog", nullptr};
const char* const kSkyHints[] = {"céu", "lua", "estrela", "estrelas", "nuvem", "nuvens", "sky", "moon", "star", "stars", "cloud", "clouds", nullptr};
const char* const kClassroomHints[] = {"professora", "professor", "lousa", "teacher", nullptr};
const char* const kKitchenHints[] = {"mesa", "panqueca", "panquecas", "café da manhã", "almoço", "jantar",
                                     "table", "breakfast", "lunch", "dinner", nullptr};
const char* const kHomeHints[] = {"porta", "door", nullptr};

const char* const kNight[] = {"noite", "dormir", "adormeceu", "adormeceram", "lua", "escuro", "escura",
                              "night", "sleep", "moon", "dark", nullptr};
const char* const kDusk[] = {"pôr", "se por", "entardecer", "fim de tarde", "alaranjado", "horizonte", "sunset", "dusk", "evening", nullptr};

/** @brief People and animals; groups bring two figures */
const char* const kMom[] = {"mamãe", "mãe", "mom", "mommy", "mum", "mother", nullptr};
const char* const kDad[] = {"papai", "pai", "dad", "daddy", "father", nullptr};
const char* const kGrandpa[] = {"vovô", "avô", "grandpa", "grandfather", nullptr};
const char* const kGrandma[] = {"vovó", "avó", "grandma", "grandmother", "granny", nullptr};
const char* const kTeacher[] = {"professora", "professor", "teacher", nullptr};
const char* const kFamily[] = {"família", "pais", "family", "parents", nullptr};
const char* const kGirlFriend[] = {"amiga", "coleguinha", nullptr};
const char* const kBoyFriend[] = {"amigo novo", "colega", "menino", "boy", nullptr};
const char* const kGirls[] = {"amigas", "meninas", "girls", nullptr};
const char* const kKids[] = {"amigos", "crianças", "colegas", "friends", "kids", "children", nullptr};
const char* const kDog[] = {"cachorro", "cachorrinho", "cão", "dog", "puppy", nullptr};
const char* const kCat[] = {"gato", "gatinho", "gata", "gatinha", "cat", "kitten", nullptr};
const char* const kFrog[] = {"sapo", "sapinho", "frog", nullptr};
const char* const kOwl[] = {"coruja", "owl", nullptr};
const char* const kBear[] = {"urso", "bear", nullptr};
const char* const kSquirrel[] = {"esquilo", "squirrel", nullptr};
const char* const kSheep[] = {"carneiro", "carneirinho", "ovelha", "ovelhinha", "sheep", "lamb", nullptr};

const char* const kFloat[] = {"boia", "boias", "float", "floaties", nullptr};
const char* const kBall[] = {"bola", "ball", nullptr};
const char* const kSandcastle[] = {"castelo de areia", "sandcastle", nullptr};
const char* const kBook[] = {"livro", "livros", "book", "books", nullptr};
const char* const kBlocks[] = {"blocos", "brinquedos", "blocks", "toys", nullptr};
const char* const kPancakes[] = {"panqueca", "panquecas", "pancake", "pancakes", nullptr};
const char* const kCake[] = {"bolo", "aniversário", "cake", "birthday", nullptr};
const char* const kFood[] = {"lanche", "lancheira", "comida", "maçã", "pão", "café da manhã", "almoço", "jantar",
                             "snack", "food", "breakfast", "lunch", "dinner", nullptr};
const char* const kPaints[] = {"pintar", "pintura", "tintas", "lápis de cor", "paint", "painting", "crayons", nullptr};
const char* const kChest[] = {"baú", "caixa", "chest", "box", nullptr};
const char* const kDoor[] = {"porta mágica", "portal", "magic door", nullptr};

const char* const kLove[] = {"abraço", "abraçou", "abraçar", "abraçaram", "abraçada", "abraçadas", "amo", "ama", "amor", "beijo", "carinho",
                             "hug", "hugged", "love", "kiss", nullptr};
const char* const kMusic[] = {"música", "músicas", "musiquinha", "cantar", "cantou", "canta", "canção", "cantarolando", "instrumentos",
                              "music", "song", "sing", "sang", nullptr};
const char* const kButterfly[] = {"borboleta", "borboletas", "butterfly", "butterflies", nullptr};
const char* const kFish[] = {"peixe", "peixes", "peixinho", "peixinhos", "fish", nullptr};
const char* const kBird[] = {"passarinho", "passarinhos", "pássaro", "pássaros", "gaivota", "gaivotas", "bird", "birds", "seagull", "seagulls", nullptr};

struct Words {
    const char* name;
    const char* const* words;
};

/** @brief Ties (same spot in the text) go to the first row: "sala de aula" before "sala" */
const Words kPlaces[] = {
    {"cave", kCave}, {"beach", kBeach}, {"pool", kPool}, {"lake", kLake}, {"mountain", kMountain},
    {"forest", kForest}, {"garden", kGarden}, {"playground", kPlayground}, {"classroom", kClassroom},
    {"library", kLibrary}, {"school", kSchool}, {"kitchen", kKitchen}, {"bedroom", kBedroom},
    {"road", kRoad}, {"living_room", kHome},
};

const Words kPlaceHints[] = {
    {"forest", kForestHints}, {"garden", kGardenHints}, {"beach", kBeachHints}, {"lake", kLakeHints},
    {"classroom", kClassroomHints}, {"kitchen", kKitchenHints}, {"living_room", kHomeHints}, {"sky", kSkyHints},
};

const Words kCastWords[] = {
    {"mom", kMom}, {"dad", kDad}, {"mom", kFamily}, {"dad", kFamily}, {"grandpa", kGrandpa}, {"grandma", kGrandma},
    {"teacher", kTeacher}, {"girl", kGirlFriend}, {"boy", kBoyFriend}, {"girl", kGirls}, {"girl2", kGirls},
    {"girl", kKids}, {"boy", kKids}, {"dog", kDog}, {"cat", kCat}, {"frog", kFrog}, {"owl", kOwl},
    {"bear", kBear}, {"squirrel", kSquirrel}, {"sheep", kSheep},
};

const Words kPropWords[] = {
    {"door", kDoor}, {"float", kFloat}, {"ball", kBall}, {"sandcastle", kSandcastle}, {"chest", kChest},
    {"pancakes", kPancakes}, {"cake", kCake}, {"food", kFood}, {"book", kBook}, {"paints", kPaints}, {"blocks", kBlocks},
};

const Words kExtraWords[] = {
    {"heart", kLove}, {"note", kMusic}, {"butterfly", kButterfly}, {"fish", kFish}, {"bird", kBird},
};

const int kFigureGround = 92;   /**< Where the figures' feet touch, in scene pixels */
const int kCastRight = 214;     /**< The cast stands right-aligned, ending here */
const int kCastWidth = 160;     /**< At most this wide, so the place still shows */
const int kOverlap = 2;         /**< Neighbours overlap a little, like a group photo */
const size_t kMaxCast = 4;
const size_t kMaxProps = 2;
const size_t kMaxExtras = 2;

template <typename T, size_t N>
const T* find(const T (&table)[N], const String& name) {
    for (const T& row : table)
        if (name == row.name) return &row;
    return nullptr;
}

/**
 * @brief Lowercase ASCII and the accented capitals ("Á" -> "á")
 */
String lowercase(const String& text) {
    String out = text;
    for (unsigned i = 0; i < out.length(); i++) {
        uint8_t c = out[i];
        if (c >= 'A' && c <= 'Z') {
            out.setCharAt(i, c + 32);
        } else if (c == 0xC3 && i + 1 < out.length()) {
            // Latin-1 capitals are C3 80..9E in UTF-8; lowercase is 0x20 above (D7 is "×")
            uint8_t next = out[i + 1];
            if (next >= 0x80 && next <= 0x9E && next != 0x97) out.setCharAt(i + 1, next + 0x20);
            i++;
        }
    }
    return out;
}

bool is_letter(char c) {
    return isalnum((unsigned char)c) || (uint8_t)c >= 0x80;
}

/** @return Where the word first appears on its own, or -1 */
int find_word(const String& text, const char* word) {
    int len = strlen(word);
    for (int at = text.indexOf(word); at >= 0; at = text.indexOf(word, at + 1)) {
        bool starts = at == 0 || !is_letter(text[at - 1]);
        bool ends = at + len >= (int)text.length() || !is_letter(text[at + len]);
        if (starts && ends) return at;
    }
    return -1;
}

/** @return Where the first of the words appears, or -1 */
int find_any(const String& text, const char* const* words) {
    int first = -1;
    for (; *words; words++) {
        int at = find_word(text, *words);
        if (at >= 0 && (first < 0 || at < first)) first = at;
    }
    return first;
}

bool has_any(const String& text, const char* const* words) {
    return find_any(text, words) >= 0;
}

/** @return The row whose word comes first in the text, or nullptr */
template <size_t N>
const char* first_named(const String& text, const Words (&table)[N]) {
    const char* best = nullptr;
    int best_at = -1;
    for (const Words& row : table) {
        int at = find_any(text, row.words);
        if (at >= 0 && (best_at < 0 || at < best_at)) {
            best = row.name;
            best_at = at;
        }
    }
    return best;
}

/**
 * @brief Turn a place name into a scene name, or "" if there is no such scene
 *
 * "bedroom" is the night or the day bedroom; "home" is the living room.
 */
String scene_for(const String& place, bool night) {
    String name = place;
    if (name == "bedroom" || name == "bedroom_night" || name == "bedroom_day")
        name = night ? "bedroom_night" : "bedroom_day";
    else if (name == "home")
        name = "living_room";
    return find(kScenes, name) ? name : "";
}

void add_unique(std::vector<String>& list, const String& name) {
    for (const String& have : list)
        if (have == name) return;
    list.push_back(name);
}

bool contains(const std::vector<String>& list, const String& name) {
    for (const String& have : list)
        if (have == name) return true;
    return false;
}

int cast_width(const std::vector<String>& cast) {
    int width = 0;
    for (const String& name : cast)
        width += find(kFigures, name)->image->header.w - kOverlap;
    return width;
}

// ---------------- Animation ----------------

void set_opa(void* obj, int32_t v) { lv_obj_set_style_opa((lv_obj_t*)obj, v, 0); }
void set_x(void* obj, int32_t v) { lv_obj_set_style_translate_x((lv_obj_t*)obj, v, 0); }
void set_y(void* obj, int32_t v) { lv_obj_set_style_translate_y((lv_obj_t*)obj, v, 0); }

/**
 * @brief Start an endless animation
 * @param back_and_forth Go back to the start value instead of jumping to it
 */
void animate(lv_obj_t* obj, lv_anim_exec_xcb_t cb, int32_t from, int32_t to, uint32_t time,
             uint32_t delay, bool back_and_forth, lv_anim_path_cb_t path = lv_anim_path_ease_in_out) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, cb);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_duration(&a, time);
    lv_anim_set_delay(&a, delay);
    if (back_and_forth) lv_anim_set_reverse_duration(&a, time);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, path);
    lv_anim_start(&a);
}

int32_t rnd(int32_t lo, int32_t hi) {
    return (int32_t)lv_rand(lo, hi);
}

lv_obj_t* make_dot(lv_obj_t* parent, int w, int h, uint32_t color, bool glowing) {
    lv_obj_t* dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, w, h);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    if (glowing) {
        lv_obj_set_style_shadow_width(dot, 6, 0);
        lv_obj_set_style_shadow_color(dot, lv_color_hex(color), 0);
        lv_obj_set_style_shadow_opa(dot, LV_OPA_70, 0);
    }
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    return dot;
}

lv_obj_t* make_sprite(lv_obj_t* parent, const lv_image_dsc_t* image, int x, int y) {
    lv_obj_t* sprite = lv_image_create(parent);
    lv_image_set_src(sprite, image);
    lv_obj_set_pos(sprite, x, y);
    lv_obj_remove_flag(sprite, LV_OBJ_FLAG_CLICKABLE);
    return sprite;
}

lv_obj_t* make_veil(lv_obj_t* banner, const Scene& scene, uint32_t color, lv_opa_t opa) {
    // Inset by the rounded corners, which stay paper colored
    lv_obj_t* veil = lv_obj_create(banner);
    lv_obj_remove_style_all(veil);
    lv_obj_set_size(veil, scene.image->header.w, scene.image->header.h);
    lv_obj_set_style_radius(veil, 10, 0);
    lv_obj_set_style_bg_color(veil, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(veil, opa, 0);
    lv_obj_remove_flag(veil, LV_OBJ_FLAG_CLICKABLE);
    return veil;
}

void add_effect(lv_obj_t* banner, const Scene& scene) {
    const lv_area_t& r = scene.area;
    for (int i = 0; i < scene.count; i++) {
        uint32_t color = scene.colors[i % 3];
        int x = rnd(r.x1, r.x2), y = rnd(r.y1, r.y2);
        lv_obj_t* dot;
        switch (scene.effect) {
        case TWINKLE:
            dot = make_dot(banner, i % 3 == 0 ? 3 : 2, i % 3 == 0 ? 3 : 2, color, scene.image == &scene_cave);
            animate(dot, set_opa, LV_OPA_10, LV_OPA_COVER, rnd(700, 1500), rnd(0, 1200), true);
            break;
        case FIREFLIES:
            dot = make_dot(banner, 3, 3, color, true);
            animate(dot, set_opa, LV_OPA_0, LV_OPA_COVER, rnd(900, 1600), rnd(0, 1500), true);
            animate(dot, set_x, -rnd(4, 10), rnd(4, 10), rnd(2200, 3600), 0, true);
            animate(dot, set_y, -rnd(3, 8), rnd(3, 8), rnd(1800, 3000), 0, true);
            break;
        case SHIMMER:
            dot = make_dot(banner, rnd(5, 10), 1, color, false);
            animate(dot, set_opa, LV_OPA_0, LV_OPA_80, rnd(900, 1600), rnd(0, 1200), true);
            animate(dot, set_x, 0, rnd(6, 12), rnd(1800, 2800), 0, true);
            break;
        case PETALS:
            dot = make_dot(banner, 3, 3, color, false);
            animate(dot, set_y, 0, 95 - y, rnd(5000, 8000), rnd(0, 4000), false, lv_anim_path_linear);
            animate(dot, set_x, -6, 6, rnd(1200, 2000), 0, true);
            break;
        case MOTES:
        default:
            dot = make_dot(banner, 2, 2, color, false);
            animate(dot, set_opa, LV_OPA_0, LV_OPA_60, rnd(1500, 2500), rnd(0, 1500), true);
            animate(dot, set_y, 0, -rnd(8, 16), rnd(3000, 4500), 0, true);
            break;
        }
        lv_obj_set_pos(dot, x, y);
    }
}

/**
 * @brief Sprite that crosses from right to left over and over, fading in and out
 */
void drift_across(lv_obj_t* sprite, int distance, uint32_t time, uint32_t delay) {
    animate(sprite, set_x, distance / 2, -distance / 2, time, delay, false, lv_anim_path_linear);
    animate(sprite, set_opa, LV_OPA_0, LV_OPA_COVER, time / 2, delay, true);
}

/**
 * @brief Hearts, notes, butterflies, fish, birds
 * @param left Where the cast starts; hearts and notes rise above it
 */
void add_extra(lv_obj_t* banner, const Scene& scene, const Sprite& extra, int left) {
    const lv_image_dsc_t* image = extra.image;
    String name = extra.name;
    if (name == "heart" || name == "note") {
        int x1 = left < kCastRight - 20 ? left : 120;
        for (int i = 0; i < 3; i++) {
            lv_obj_t* s = make_sprite(banner, image, rnd(x1, kCastRight - 10), rnd(30, 46));
            uint32_t time = rnd(1800, 2400), delay = i * 700;
            animate(s, set_y, 0, -24, time, delay, false, lv_anim_path_ease_out);
            animate(s, set_opa, LV_OPA_COVER, LV_OPA_0, time, delay, false, lv_anim_path_ease_in);
            if (name == "note") animate(s, set_x, -3, 3, time / 3, 0, true);
        }
    } else if (name == "butterfly") {
        for (int i = 0; i < 2; i++) {
            lv_obj_t* s = make_sprite(banner, image, rnd(40, 160), rnd(10, 36));
            animate(s, set_x, -rnd(20, 36), rnd(20, 36), rnd(3000, 4200), 0, true);
            animate(s, set_y, -6, 6, rnd(700, 1000), 0, true);
        }
    } else if (name == "fish") {
        const lv_area_t& w = scene.water;
        if (w.x2 <= w.x1) return;
        for (int i = 0; i < 2; i++) {
            lv_obj_t* s = make_sprite(banner, image, (w.x1 + w.x2) / 2, rnd(w.y1 + 2, w.y2 - 10));
            drift_across(s, w.x2 - w.x1 - 16, rnd(3000, 4200), i * 1500);
        }
    } else if (name == "bird") {
        if (!scene.outdoor) return;
        for (int i = 0; i < 2; i++) {
            lv_obj_t* s = make_sprite(banner, image, 100, rnd(8, 24));
            drift_across(s, 180, rnd(5000, 7000), i * 2200);
            animate(s, set_y, -2, 2, 600, 0, true);
        }
    }
}

// ---------------- Figures ----------------

void start_idle(lv_obj_t* figure) {
    const Figure* f = (const Figure*)lv_obj_get_user_data(figure);
    animate(figure, set_y, 0, -2, f->idle, 0, true);
}

void jump_done(lv_anim_t* a) {
    start_idle((lv_obj_t*)a->var);
}

/**
 * @brief Hop once, then go back to the idle bob
 * @param delay Wait before the hop
 */
void jump(lv_obj_t* figure, uint32_t delay) {
    const Figure* f = (const Figure*)lv_obj_get_user_data(figure);
    lv_anim_delete(figure, set_y);
    lv_obj_set_style_translate_y(figure, 0, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, figure);
    lv_anim_set_exec_cb(&a, set_y);
    lv_anim_set_values(&a, 0, -(int32_t)f->hop);
    lv_anim_set_duration(&a, 220);
    lv_anim_set_reverse_duration(&a, 260);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_completed_cb(&a, jump_done);
    lv_anim_start(&a);
}

void sparkle_done(lv_anim_t* a) {
    lv_obj_delete_async((lv_obj_t*)a->var);
}

/**
 * @brief A few golden dots flying up from a point and fading
 */
void burst(lv_obj_t* banner, int x, int y) {
    for (int i = 0; i < 5; i++) {
        lv_obj_t* dot = make_dot(banner, 3, 3, i % 2 ? 0xFFD45C : 0xFFF3D6, true);
        lv_obj_set_pos(dot, x, y);
        for (int k = 0; k < 3; k++) {
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, dot);
            lv_anim_set_duration(&a, 650);
            lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
            if (k == 0) {
                lv_anim_set_exec_cb(&a, set_x);
                lv_anim_set_values(&a, 0, (i - 2) * 7);
            } else if (k == 1) {
                lv_anim_set_exec_cb(&a, set_y);
                lv_anim_set_values(&a, 0, -rnd(16, 28));
            } else {
                lv_anim_set_exec_cb(&a, set_opa);
                lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_0);
                lv_anim_set_completed_cb(&a, sparkle_done);
            }
            lv_anim_start(&a);
        }
    }
}

lv_obj_t* add_figure(lv_obj_t* banner, const Figure& f, int x) {
    lv_obj_t* figure = make_sprite(banner, f.image, x, kFigureGround - (int)f.image->header.h);
    // Marks it as a figure for the tap, and keeps its hop and bob pace
    lv_obj_set_user_data(figure, (void*)&f);
    start_idle(figure);
    return figure;
}

/**
 * @brief A tap anywhere on the scene: everyone jumps, right to left
 */
void on_tap(lv_event_t* e) {
    lv_obj_t* banner = (lv_obj_t*)lv_event_get_current_target(e);
    uint32_t delay = 0;
    bool first = true;
    for (int i = lv_obj_get_child_count(banner) - 1; i >= 0; i--) {
        lv_obj_t* child = lv_obj_get_child(banner, i);
        if (!lv_obj_get_user_data(child)) continue;
        if (first) {
            lv_anim_t* a = lv_anim_get(child, set_y);
            if (a && a->repeat_cnt != LV_ANIM_REPEAT_INFINITE) return;  // still in the air
            audio::play(audio::Sound::Boing);
            burst(banner, lv_obj_get_x(child) + lv_obj_get_width(child) / 2, lv_obj_get_y(child) + 4);
            first = false;
        }
        jump(child, delay);
        delay += 110;
    }
}

}  // namespace

std::vector<String> leads(const Story_t& story) {
    std::vector<String> out;
    if (story.cast_set) {
        for (const String& name : story.cast)
            if (find(kFigures, name)) add_unique(out, name);
        return out;
    }
    bool zoe = false, ted = false;
    for (const auto& node : story.nodes) {
        zoe = zoe || find_word(node.second.text, "Zoe") >= 0;
        ted = ted || find_word(node.second.text, "Ted") >= 0;
    }
    if (zoe) out.push_back("zoe");
    if (ted) out.push_back("ted");
    return out;
}

Spec pick(const Story_t& story, const Node_t& node, const std::vector<String>& leads, const Spec& previous) {
    Spec spec;
    String text = lowercase(node.text);
    // "The sun was setting and night was coming" is still the sunset
    spec.dusk = !story.night && has_any(text, kDusk);
    spec.night = !spec.dusk && (story.night || has_any(text, kNight));

    // Where: the page's own scene, a place it names, else where we were
    bool first_page = !find(kScenes, previous.place);
    spec.place = scene_for(node.scene, spec.night);
    if (!spec.place.length()) {
        const char* named = first_named(text, kPlaces);
        if (!named && first_page) named = first_named(text, kPlaceHints);
        if (named) spec.place = scene_for(named, spec.night);
    }
    if (!spec.place.length() && !first_page)
        spec.place = scene_for(previous.place.startsWith("bedroom") ? "bedroom" : previous.place, spec.night);
    if (!spec.place.length()) spec.place = spec.night ? "sky" : "garden";
    bool same_place = spec.place == previous.place;

    // Who: the page's cast, or whoever it mentions plus the leads. When it
    // mentions no one, family and friends come along; the frog stays by its lake.
    if (node.cast_set) {
        for (const String& name : node.cast)
            if (find(kFigures, name) && spec.cast.size() < kMaxCast) add_unique(spec.cast, name);
    } else {
        // In the order the page names them, so the first ones get the room
        std::vector<std::pair<int, String>> named;
        for (const auto& character : story.characters) {
            int at = find_word(text, lowercase(character.first).c_str());
            if (at >= 0 && find(kFigures, character.second)) named.push_back({at, character.second});
        }
        for (const Words& row : kCastWords) {
            int at = find_any(text, row.words);
            if (at >= 0) named.push_back({at, row.name});
        }
        std::stable_sort(named.begin(), named.end(),
                         [](const std::pair<int, String>& a, const std::pair<int, String>& b) { return a.first < b.first; });
        std::vector<String> company;
        for (const auto& n : named) add_unique(company, n.second);
        if (company.empty()) {
            for (const String& name : previous.cast)
                if (same_place || find(kFigures, name)->travels) company.push_back(name);
        }
        for (const String& name : company)
            if (!contains(leads, name)) add_unique(spec.cast, name);
        while (!spec.cast.empty() && (spec.cast.size() + leads.size() > kMaxCast ||
                                      cast_width(spec.cast) + cast_width(leads) > kCastWidth))
            spec.cast.pop_back();
        for (const String& name : leads) add_unique(spec.cast, name);
    }

    // What: things the page mentions, or the ones already there
    if (node.props_set) {
        for (const String& name : node.props)
            if (find(kProps, name)) add_unique(spec.props, name);
    } else {
        for (const Words& row : kPropWords)
            if (has_any(text, row.words)) add_unique(spec.props, row.name);
        if (spec.props.empty() && same_place) spec.props = previous.props;
    }
    if (spec.props.size() > kMaxProps) spec.props.resize(kMaxProps);

    for (const Words& row : kExtraWords)
        if (spec.extras.size() < kMaxExtras && has_any(text, row.words)) spec.extras.push_back(row.name);
    return spec;
}

lv_obj_t* create(lv_obj_t* parent, const Spec& spec) {
    const Scene* scene = find(kScenes, spec.place);
    if (!scene) scene = &kScenes[0];

    lv_obj_t* banner = lv_image_create(parent);
    lv_image_set_src(banner, scene->image);
    lv_obj_remove_flag(banner, LV_OBJ_FLAG_SCROLLABLE);

    if (spec.night && scene->night_dim) {
        // Daytime pictures turned to night: a dark veil, and stars in the sky
        make_veil(banner, *scene, 0x141A3D, scene->night_dim);
        for (int i = 0; scene->outdoor && i < 6; i++) {
            lv_obj_t* star = make_dot(banner, 2, 2, 0xFFF6DC, false);
            lv_obj_set_pos(star, rnd(8, 210), rnd(4, 34));
            animate(star, set_opa, LV_OPA_10, LV_OPA_COVER, rnd(700, 1500), rnd(0, 1200), true);
        }
    } else if (spec.dusk && scene->outdoor && scene->night_dim) {
        make_veil(banner, *scene, 0xF28C28, LV_OPA_20);
    }
    add_effect(banner, *scene);

    // Cast, right-aligned
    int left = kCastRight;
    std::vector<const Figure*> cast;
    for (const String& name : spec.cast)
        if (const Figure* f = find(kFigures, name)) cast.push_back(f);
    for (auto it = cast.rbegin(); it != cast.rend(); ++it)
        left -= (*it)->image->header.w - kOverlap;

    // Props stand in the scene's spot, left of the cast, centered
    int x1 = scene->prop_x1, x2 = min((int)scene->prop_x2, left - 2);
    std::vector<const Sprite*> props;
    int width = 0;
    for (const String& name : spec.props) {
        const Sprite* p = find(kProps, name);
        if (!p) continue;
        int w = p->image->header.w + (props.empty() ? 0 : 6);
        if (width + w > x2 - x1) break;
        props.push_back(p);
        width += w;
    }
    int x = x1 + (x2 - x1 - width) / 2;
    const lv_area_t& water = scene->water;
    for (const Sprite* p : props) {
        int h = p->image->header.h;
        lv_obj_t* prop = make_sprite(banner, p->image, x, scene->prop_ground - h);
        int cx = x + p->image->header.w / 2, foot = scene->prop_ground - 2;
        if (cx > water.x1 && cx < water.x2 && foot > water.y1 && foot < water.y2)
            animate(prop, set_y, -1, 1, rnd(900, 1200), 0, true);  // floating
        x += p->image->header.w + 6;
    }

    int fx = left;
    for (const Figure* f : cast) {
        add_figure(banner, *f, fx);
        fx += f->image->header.w - kOverlap;
    }
    if (!cast.empty()) {
        lv_obj_add_flag(banner, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(banner, on_tap, LV_EVENT_CLICKED, nullptr);
    }

    for (const String& name : spec.extras)
        if (const Sprite* extra = find(kExtras, name)) add_extra(banner, *scene, *extra, left);

    lv_obj_fade_in(banner, 250, 0);
    return banner;
}

}
