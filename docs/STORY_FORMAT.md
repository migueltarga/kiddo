# Kiddo Story Format

A Kiddo story is one JSON file: pages ("nodes") with text and choices that lead to other pages. A catalog (`index.json`) lists the stories the device can download. This document describes both, plus the optional extras: items and the backpack, scenes, figures and sounds.

Check your files before publishing:

```sh
python3 scripts/validate_stories.py            # checks stories/
python3 scripts/validate_stories.py my/folder  # any folder with an index.json
```

## The catalog

The device downloads the catalog from the URL set in *Settings > Catalog* (or with the devtools `catalog <url>` command). Each story file is fetched from the same folder as the catalog, so a GitHub gist or a static web folder works as is.

```json
{
  "name": "Zoe's Stories",
  "stories": [
    { "name": "Zoe and the Lost Moon", "file": "story_lost_moon.json", "lang": "en" },
    { "name": "Zoe e a Lua Perdida", "file": "story_lua_perdida.json", "lang": "pt-br" }
  ]
}
```

| Field | Required | Notes |
|---|---|---|
| `name` | no | Catalog name shown in Settings. |
| `stories[].name` | yes | Title in the library. |
| `stories[].file` | yes | A plain `.json` file name of at most 30 characters (SPIFFS limit), not `index.json`. |
| `stories[].lang` | yes | `en` or `pt-br`. The library shows the stories in the device's language. |

Once a story is downloaded, the device keeps its copy. If you change the story later, delete it on the device (*Settings > Clear Cache*, or devtools `rm /story_x.json`) to get the new version.

## A story

```json
{
  "id": "zoe_lost_moon",
  "title": "Zoe and the Lost Moon",
  "start": "beginning",
  "night": true,
  "nodes": {
    "beginning": {
      "text": "It was bedtime, but the Moon was gone!\nTed blinked: 'Let's find it!'",
      "choices": [
        { "text": "Look in the kitchen", "next": "kitchen" },
        { "text": "Run outside", "next": "yard" }
      ]
    },
    "kitchen": { "text": "...", "choices": [ ... ] },
    "the_end": { "text": "And Zoe fell asleep smiling.", "end": true }
  }
}
```

| Field | Required | Notes |
|---|---|---|
| `id` | yes | Unique id. Saved progress is keyed by it, so don't change it once published. |
| `title` | yes | Shown in the page header. |
| `start` | yes | Key of the first page. |
| `lang` | no | Filled in from the catalog when missing. |
| `night` | no | `true` if the whole story happens at night: every scene is shown dark. |
| `has_inventory` | no | `true` to use items and the backpack, see [Items](#items-and-the-backpack). |
| `item_definitions`, `initial_inventory` | no | See [Items](#items-and-the-backpack). |
| `cast`, `characters` | no | See [Scenes](#scenes). |
| `nodes` | yes | Pages by key. Keys are any text; the order doesn't matter. |

### Pages

| Field | Notes |
|---|---|
| `text` | The page text. `\n` starts a new line. Add a picture with `[img]https://.../picture.jpg[/img]`, see [Pictures](#pictures). |
| `choices` | Buttons shown at the end of the page, see below. |
| `end` | `true` for an ending. A page needs either choices or `end`. |
| `gives_item` | Item id added to the backpack when the page opens. |
| `scene`, `cast`, `props` | Override the animated scene, see [Scenes](#scenes). |
| `sound` | Sound played when the page opens, see [Sounds](#sounds). |
| `inventory_choice`, `correct_item`, `success_next`, `failure_next` | A backpack puzzle, see below. |

A choice:

| Field | Notes |
|---|---|
| `text` | Button label. Keep it short: two lines at most fit well. |
| `next` | Key of the page it leads to. |
| `gives_item` | Item id added to the backpack when chosen. |
| `required_item` | Only usable with this item in the backpack. The item is used up. |
| `hidden_without_item` | `true` hides the choice (instead of showing it disabled) without the item. |

### Writing the text

- The font covers Latin-1: accents work (á, ç, ã, ü), emoji and curly quotes don't. Use straight quotes `'` and `"`.
- Pages scroll, but short pages read better aloud: 3 to 6 sentences.
- Line breaks are kept, extra spaces are dropped.

## Items and the backpack

Set `"has_inventory": true`, define the items, and optionally start with some:

```json
"has_inventory": true,
"item_definitions": [
  { "id": "flashlight", "name": "Flashlight", "description": "Lights up the dark." },
  { "id": "honey", "name": "Honey jar", "description": "A bear's favorite." }
],
"initial_inventory": [ { "id": "flashlight" } ]
```

| Field | Notes |
|---|---|
| `id` | Used by `gives_item`, `required_item` and `correct_item`. |
| `name` | Shown in the backpack and on the "found" card. |
| `description` | Shown when the child taps the item. |
| `icon_url` | Accepted but not used yet: the device doesn't download item icons. |

These ids have built-in icons: `flashlight`, `honey`, `feather`, `shell`, `key`/`golden_key`, `map`/`old_map`, `potion`/`magic_potion`, `chest`/`treasure`, `sword`, `compass`, `backpack`. Other items get a colored square with their first letter.

The backpack holds 5 items. Items found on a page show as a card at its end.

### Backpack puzzles

A page with `inventory_choice` asks the child to pick an item from the backpack:

```json
"bridge": {
  "text": "A big bear is sleeping on the bridge. What could wake him up gently?",
  "inventory_choice": true,
  "correct_item": "honey",
  "success_next": "bridge_honey",
  "failure_next": "bridge_wrong"
}
```

The right item is used up and leads to `success_next`. A wrong item stays in the backpack and leads to `failure_next`, which should let the child try again.

## Pictures

`[img]url[/img]` in the text shows a JPEG picture there. Keep pictures small, about 200 px wide, as baseline (not progressive) JPEG: the decoder needs a contiguous block of memory for the whole picture. The device downloads them while the page is read and keeps them for next time.

Pages without a picture show an animated scene instead.

## Scenes

A page without a picture gets a 220x100 animated scene at the top: a place, the people and animals in it, a few things, and moving bits. Tapping the scene makes everyone jump. Everything is picked from the page's words, in Portuguese or English, so most stories need nothing extra. The fields below fix the pages where the words mislead (a "sea of adventures" in a pool).

### How the scene is picked

- **Place**: the first place the page names. "slept under a tree and woke up in the bedroom" is the bedroom, because "tree" is only a hint, used on the first page. A page that names no place stays where the last one was.
- **People and animals**: everyone the page names, in the order named. Groups count: "friends", "amigas" and "family" bring two figures. When a page names no one, family, friends and pets come along to the next place, while the frog stays by its lake.
- **Leads**: Zoe and Ted are on every page of a story that names them. Set the story's `cast` to choose other leads.
- **Things**: things the page mentions (a float, a cake, a book), at most 2. They stay while the place does.
- **Moving bits**: hearts for hugs and love, notes for songs, butterflies, fish (in water) and birds (outdoors).
- **Time**: night words ("night", "sleep", "moon", "noite"...) or a `night` story make scenes dark with stars. Sunset words ("sunset", "pôr do sol", "fim de tarde") warm outdoor scenes.

At most 4 figures fit. Leads always stay; the others are kept in the order the page names them.

### Overrides

On the story:

| Field | Example | Notes |
|---|---|---|
| `cast` | `["zoe", "ted"]` | Figures on every page. `[]` for none. |
| `characters` | `{"Bela Maria": "girl", "Pipoca": "squirrel"}` | Names in the text and the figure each one gets. |

On a page:

| Field | Example | Notes |
|---|---|---|
| `scene` | `"pool"` | The place. |
| `cast` | `["grandpa", "zoe", "ted"]` | Exactly these figures, left to right. `[]` for none. |
| `props` | `["cake"]` | Exactly these things. `[]` for none. |

```json
"boia": {
  "text": "The girls lay on their floats, pretending to sail a sea of adventures...",
  "scene": "pool"
}
```

### Places

| Scene | Picked by words like |
|---|---|
| `bedroom` | quarto, cama, dormir, acordou; bedroom, bed, sleep, woke. Becomes `bedroom_night` or `bedroom_day`. |
| `living_room` (or `home`) | sala, sofá, campainha, casa; living room, sofa, doorbell, house, home. Has the front door. |
| `kitchen` | cozinha; kitchen. Hints: mesa, almoço, panqueca; table, lunch. |
| `pool` | piscina; pool |
| `classroom` | sala de aula; classroom. Hints: professora; teacher |
| `library` | biblioteca; library |
| `playground` | parquinho, escorregador, recreio, pátio; playground, slide, recess |
| `school` | escola; school. The front, with a door and a gate. |
| `road` | carro, estrada, viagem; car, road, trip |
| `garden` | quintal, jardim; garden, yard. Hints: flores, borboleta; flowers, butterfly |
| `forest` | floresta, bosque; forest, woods. Hints: árvore, coruja; tree, owl |
| `cave` | caverna, gruta, túnel; cave, tunnel |
| `lake` | lago, rio, ponte; lake, river, bridge. Has a wooden bridge. |
| `mountain` | montanha; mountain |
| `sky` | Hints: céu, lua, estrelas; sky, moon, stars |
| `beach` | praia, mar, oceano; beach, sea, ocean |

A first page that names no place shows the garden, or the night sky in a `night` story.

### Figures

| Figure | Picked by words like |
|---|---|
| `zoe`, `ted` | The leads, see above. |
| `mom`, `dad` | mamãe, papai; mom, dad. "família"/"family" brings both. |
| `grandpa`, `grandma` | vovô, vovó; grandpa, grandma |
| `teacher` | professora; teacher |
| `girl`, `girl2`, `boy` | amiga, colega; "amigas"/"meninas"/"girls" bring both girls, "amigos"/"crianças"/"friends"/"kids" a girl and a boy. |
| `dog`, `cat` | cachorro, gato; dog, cat |
| `frog`, `owl`, `bear`, `squirrel`, `sheep` | sapo, coruja, urso, esquilo, carneirinho; frog, owl, bear, squirrel, sheep. "ursinho" is Ted, not the bear. |

### Props

| Prop | Picked by words like |
|---|---|
| `float` | boia; float |
| `ball` | bola; ball |
| `sandcastle` | castelo de areia; sandcastle |
| `book` | livro; book |
| `blocks` | blocos, brinquedos; blocks, toys |
| `pancakes` | panqueca; pancake |
| `cake` | bolo, aniversário; cake, birthday |
| `food` | lanche, almoço, maçã, pão; snack, lunch, food |
| `paints` | pintar, tintas, lápis de cor; paint, crayons |
| `chest` | baú, caixa; chest, box |
| `door` | porta mágica, portal; magic door |

In the pool, props float in the water. In the kitchen they're on the table, and in the classroom on the desk.

The full word lists are in `src/ui/components/story_scene.cpp`. The art is drawn by `scripts/gen_scenes.py` (`--preview` writes a sheet with everything).

## Sounds

With a speaker attached, pages play a sound when they open: a fanfare on endings, a chime when an item is found. Set `sound` on a page to choose one:

| Sound | Good for |
|---|---|
| `success` | A puzzle solved. |
| `oops` | A wrong answer, try again. |
| `chime` | Something magical. |
| `fanfare` | A big moment. |
| `boing` | Something funny. |
| `page`, `click` | Quiet ticks. |

## A complete example

`stories/story_lua_perdida.json` uses almost everything: a night story, items with built-in icons, backpack puzzles, riddles with `success`/`oops` sounds, `characters` for Pipoca the squirrel and `scene` fixes for figurative words.
