# Fossil Availability: which fossils can be dug up. All / Season Locked (each fossil only in its season, logic
# needs a month of it under Timesanity) / Progressive (one item per dinosaur series, "Single Fossils" for the
# rest; nothing is buried before the first). Progressive only with fossil checks, else the client buries all.
# WARN: the client mirrors the series order and ids (pc_ap_museumsanity_fossil_series, PC_AP_ITEM_FOSSIL_SERIES_BASE)
# and has a copy of the seasons for offline play (ap_slotdata.cpp default_fossil_seasons)!
# Reads: world.museum_modes (museumsanity.setup).
from BaseClasses import ItemClassification

from . import options, timesanity
from .museum_data import FOSSILS
from .museumsanity import MUSEUM_FOSSIL, add_museum_rule
from .timesanity import MONTHS

# Season Locked: the season each fossil comes out in, dinosaurs kept together.
SEASON_MONTHS = [(2, 3, 4), (5, 6, 7), (8, 9, 10), (11, 0, 1)]  # Spring, Summer, Autumn, Winter: months 0-11
_SEASON_OF_FOSSIL = {
    # Spring: early life, hatching, flyers coming back
    "Trilobite": 0, "Ammonite": 0, "Dinosaur Egg": 0, "Ptera Skull": 0, "Ptera Right Wing": 0, "Ptera Left Wing": 0,
    # Summer: the Jurassic giants
    "Apato Skull": 1, "Apato Tail": 1, "Apato Torso": 1, "Stego Skull": 1, "Stego Tail": 1, "Stego Torso": 1,
    # Autumn: the Late Cretaceous, the dinosaurs' "fall"
    "T-rex Skull": 2, "T-rex Tail": 2, "T-rex Torso": 2, "Tricera Skull": 2, "Tricera Tail": 2, "Tricera Torso": 2,
    # Winter: ice age, cold seas, tracks in the snow
    "Mammoth Skull": 3, "Mammoth Torso": 3, "Plesio Skull": 3, "Plesio Neck": 3, "Plesio Torso": 3,
    "Amber": 3, "Dinosaur Track": 3,
}
FOSSIL_SEASONS = [_SEASON_OF_FOSSIL[name] for name in FOSSILS]  # museum fossil index -> season

# Progressive: one item per series opens its fossils; "Single Fossils" opens the rest (Amber, Dinosaur Track,
# Ammonite, Dinosaur Egg, Trilobite).
FOSSIL_SERIES = ["Tricera", "T-rex", "Apato", "Stego", "Ptera", "Plesio", "Mammoth", "Single"]
FOSSIL_SERIES_ID_BASE = 0x10030  # series s = base + s


def fossil_series_item_name(series: str) -> str:
    return f"{series} Fossils"


FOSSIL_SERIES_ITEMS = [fossil_series_item_name(series) for series in FOSSIL_SERIES]
ITEMS = {name: FOSSIL_SERIES_ID_BASE + s for s, name in enumerate(FOSSIL_SERIES_ITEMS)}
# museum fossil index -> its series item name ("T-rex Skull" -> "T-rex Fossils", "Amber" -> "Single Fossils")
FOSSIL_SERIES_OF = []
for _name in FOSSILS:
    _series = _name.split(" ")[0]
    FOSSIL_SERIES_OF.append(fossil_series_item_name(_series if _series in FOSSIL_SERIES else "Single"))


def setup(world) -> None:
    world.fossil_series_items = []
    if (world.options.fossil_availability == options.FossilAvailability.option_progressive
            and world.museum_modes[MUSEUM_FOSSIL]):
        world.fossil_series_items = list(FOSSIL_SERIES_ITEMS)


def items(world) -> list[str]:
    return world.fossil_series_items


def classify(world, name: str):
    return ItemClassification.progression if name in ITEMS else None


def rules(world) -> None:
    # Season Locked (dig up and donate): a month of its season
    if world.options.fossil_availability == options.FossilAvailability.option_season_locked:
        for name, season in zip(FOSSILS, FOSSIL_SEASONS):
            months = timesanity.month_rule(world, [MONTHS[m] for m in SEASON_MONTHS[season]])
            if months:
                add_museum_rule(world, MUSEUM_FOSSIL, name, months)
    # Progressive: a fossil needs its series item
    if world.fossil_series_items:
        for name, item in zip(FOSSILS, FOSSIL_SERIES_OF):
            add_museum_rule(world, MUSEUM_FOSSIL, name, lambda state, item=item: state.has(item, world.player))


def slot_data(world) -> dict:
    return {
        "fossil_availability": world.options.fossil_availability.value,
        "fossil_seasons": FOSSIL_SEASONS,
    }
