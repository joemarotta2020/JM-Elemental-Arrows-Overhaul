# JM Elemental Arrows Overhaul

Compatibility/balance patch and native SKSE support for **Elemental Arrows by Fuma**.

Target runtime: **Skyrim SE 1.5.97 / SKSE 2.0.20**.

## Soul-gem crafting

The ESP adds Gem Arrow recipes for Petty, Lesser, Common, Greater, Grand, and Black soul gems.

The DLL handles player-filled Soul Trap gems correctly. Skyrim keeps the empty gem's base FormID and stores the trapped soul on that inventory instance. The DLL snapshots the real soul data before crafting, lets Skyrim complete the recipe normally, then compares the post-craft inventory and adds only the missing Gem Arrows for the soul gems actually consumed.

This also works with Fuma's original 20/40/60-arrow multi-gem recipes.

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
| Black gem with Grand/NPC soul | 120 |

A filled gem never yields less than that container's empty value. For example, a Greater Soul Gem containing a Petty soul still yields 20.

Azura's Star and the Black Star are not consumed by these recipes.

## Mechanical shock shutdown

Shock and Thunderbolt arrows keep Fuma's normal effects.

Targets carrying Skyrim's vanilla `ActorTypeDwarven` keyword receive an additional **5-second mechanical shutdown/paralysis**. This covers normal Dwemer constructs and lets Chain Beasts follow the same rule when their actor/race setup uses `ActorTypeDwarven`.

No polling or actor scanning is used.

## Installation

Required:
- Elemental Arrows by Fuma
- SKSE64 2.0.20
- Address Library for Skyrim SE 1.5.97

Deploy:

```
JM_ElementalArrows_Overhaul.esp
SKSE\Plugins\JM_ElementalArrows.dll
```

Load `JM_ElementalArrows_Overhaul.esp` after `ElementalArrows.esp`.

The ESP is ESL-flagged.

## Logging

```
Documents\My Games\Skyrim Special Edition\SKSE\JM_ElementalArrows.log
```

For filled-gem crafting, the log records the consumed gem/soul tier, vanilla output, expected output, and bonus arrows added.

## Design

The DLL does not replace the crafting menu and does not hook the Craft button. It uses Skyrim's own `ItemCrafted` event plus inventory instance data, keeping the integration narrow and compatible with Fuma's existing crafting flow.
