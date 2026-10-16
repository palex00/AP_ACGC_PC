# Fish Availability: All / Progressive. Progressive: a fish spawns once any of its items is received (FISH_PASSES,
# generated from the game's spawn areas and shadow sizes: river by size, ponds + the river's lake, sea). Only with
# fish checks, else the client spawns every fish. Island fish are untouched.
# WARN: the client mirrors the order and ids (PC_AP_FISH_PASS_*, PC_AP_ITEM_FISH_PASS_BASE)!
# Reads: world.museum_modes (museumsanity.setup).
from BaseClasses import ItemClassification

from . import options
from .museum_data import FISH, FISH_PASSES
from .museumsanity import MUSEUM_FISH, add_museum_rule

FISH_PASS_ITEMS = ["Small River Fish", "Medium River Fish", "Large River Fish", "Pond Fish", "Sea Fish"]
FISH_PASS_ID_BASE = 0x10040  # pass p = base + p
ITEMS = {name: FISH_PASS_ID_BASE + p for p, name in enumerate(FISH_PASS_ITEMS)}
# museum fish name -> its item names (any one opens it)
FISH_PASS_OF = {name: [FISH_PASS_ITEMS[p] for p in passes] for (name, _), passes in zip(FISH, FISH_PASSES)}


def setup(world) -> None:
    world.fish_pass_items = []
    if (world.options.fish_availability == options.FishAvailability.option_progressive
            and world.museum_modes[MUSEUM_FISH]):
        world.fish_pass_items = list(FISH_PASS_ITEMS)


def items(world) -> list[str]:
    return world.fish_pass_items


def classify(world, name: str):
    return ItemClassification.progression if name in ITEMS else None


def rules(world) -> None:
    # A fish (catch and donate) needs any of its items
    if world.fish_pass_items:
        for name, passes in FISH_PASS_OF.items():
            add_museum_rule(world, MUSEUM_FISH, name, lambda state, p=passes: state.has_any(p, world.player))


def slot_data(world) -> dict:
    return {"fish_availability": world.options.fish_availability.value}
