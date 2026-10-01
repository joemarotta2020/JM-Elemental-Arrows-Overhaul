# JM Elemental Arrows Overhaul

Compatibility/balance patch and native SKSE support for **Elemental Arrows by Fuma**.

Target runtime: **Skyrim SE 1.5.97 / SKSE 2.0.20**.

## What it changes

### Soul-gem crafting

The ESP adds Gem Arrow recipes for Petty, Lesser, Common, Greater, Grand, and Black soul gems.

The native DLL solves Skyrim's instance-data problem for player-filled soul gems. A gem filled by Soul Trap normally keeps the empty gem's base FormID and stores the trapped soul on that inventory instance. The DLL snapshots the actual soul data before crafting, lets Skyrim perform the recipe normally, then compares the post-craft inventory and adds only the missing Gem Arrows for the soul(s) that were actually consumed.

This works with Fuma's original multi-gem 20/40/60-arrow recipes as well as the added single-gem recipes.

Yield rules:

| Gem / soul | Gem Arrows |
| --- | ---: |
| Petty empty | 3 |
| Lesser empty | 5 |
| Common empty | 10 |
| Greater empty | 20 |
| Grand empty | 30 |
| Black empty | 30 |
| Petty soul | 10 |
| Lesser soul | 20 |
| Common soul | 40 |
| Greater soul | 75 |
| Grand soul | 100 |
| Black gem with a Grand/NPC soul | 120 |

A filled gem never yields less than that gem container's empty value. For example, a Greater Soul Gem containing a Petty soul still yields 20 rather than dropping below the Greater gem's empty value.

No reusable soul gem such as Azura's Star is used by these recipes.

### Mechanical shock shutdown

Shock and Thunderbolt arrows retain Fuma's normal effects.

Against actors with Skyrim's vanilla \`ActorTypeDwarven\` keyword they additionally apply a **5-second mechanical shutdown/paralysis**.

That covers ordinary Dwemer constructs and also lets Skyrim Chain Beasts follow the same rule when their actor/race setup uses \`ActorTypeDwarven\`.

No polling or actor scanning is used.

## Installation

Required:
- Elemental Arrows by Fuma
- SKSE64 2.0.20
- Address Library appropriate for Skyrim SE 1.5.97

Deploy:

\`\`\`
JM_ElementalArrows_Overhaul.esp
SKSE\Plugins\JM_ElementalArrows.dll
\`\`\`

Load \`JM_ElementalArrows_Overhaul.esp\` after \`ElementalArrows.esp\`.

The ESP is ESL-flagged.

## Logging

Native log:

\`\`\`
Documents\My Games\Skyrim Special Edition\SKSE\JM_ElementalArrows.log
\`\`\`

On a successful Gem Arrow craft using a filled player-created gem, the log records the consumed gem/soul tier, vanilla output, expected output, and any bonus arrows added.

## Design notes

The DLL does not replace the crafting menu and does not intercept the player's Craft button. It uses Skyrim's own \`ItemCrafted\` event and inventory instance data. This deliberately minimizes compatibility risk and leaves Fuma's normal recipes and crafting flow intact.
