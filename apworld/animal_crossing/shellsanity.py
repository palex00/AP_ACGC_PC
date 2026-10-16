# Shellsanity: town shells sometimes wash up as AP items for the current month; picking one up sends the next
# unsent "Beach: <Month> k". Checks per month 0-20 (0 = off).
# WARN: the client mirrors the ids and the max (PC_AP_LOC_BEACH_BASE, AP_BEACH_PER_MONTH_MAX)!
from worlds.generic.Rules import add_rule

from . import timesanity
from .timesanity import MONTHS

BEACH_PER_MONTH_MAX = 20  # Shellsanity max
BEACH_BASE_ID = 0x50000  # month m (0-11), check k (1-based): BEACH_BASE_ID + m * 0x100 + k


def beach_location_name(month: int, check: int) -> str:
    return f"Beach: {MONTHS[month]} {check}"


LOCATIONS = {beach_location_name(m, k): BEACH_BASE_ID + m * 0x100 + k
             for m in range(len(MONTHS)) for k in range(1, BEACH_PER_MONTH_MAX + 1)}


def locations(world) -> list[str]:
    per_month = world.options.shellsanity.value
    return [beach_location_name(m, k) for m in range(len(MONTHS)) for k in range(1, per_month + 1)]


def rules(world) -> None:
    # An item washes up for the current month only
    for m in range(len(MONTHS)):
        month = timesanity.month_rule(world, [MONTHS[m]])
        if month:
            for k in range(1, world.options.shellsanity.value + 1):
                add_rule(world.get_location(beach_location_name(m, k)), month)


def slot_data(world) -> dict:
    return {
        "shellsanity": world.options.shellsanity.value,
        "shellsanity_item_chance": world.options.shellsanity_item_chance.value,
    }
