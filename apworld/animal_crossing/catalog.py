# Catalogsanity: N pools of M catalog items; registering a pool item in the catalog sends its check.
# Pool 1 is open at the start, pool k needs k - 1 Progressive Catalog Pool items.
# The eligible items are generated (catalog_data.py); this file is hand-written.
# WARN: the client mirrors the location base (pc side, step 2).
import re
from random import Random

from .catalog_data import CATALOG_ITEMS
from .items import ITEM_NAME_TO_ID

CATALOG_LOCATION_BASE_ID = 0x40000  # "Catalog: X" = base + game item id (in the location range of locations.py)
CATALOG_ELIGIBLE = sum(len(ids) for ids in CATALOG_ITEMS.values())

_ID_TO_NAME = {item_id: name for name, item_id in ITEM_NAME_TO_ID.items()}


def catalog_location_name(item_id: int) -> str:
    # Stationery (not eligible now) is sold in stacks of 4 ("airmail paper x4"); the entry is the paper itself
    return "Catalog: " + re.sub(r" x\d+$", "", _ID_TO_NAME[item_id])


CATALOG_LOCATION_NAME_TO_ID = {catalog_location_name(item_id): CATALOG_LOCATION_BASE_ID + item_id
                               for ids in CATALOG_ITEMS.values() for item_id in ids}
assert len(CATALOG_LOCATION_NAME_TO_ID) == CATALOG_ELIGIBLE, "duplicate catalog location names"


def make_pools(random: Random, pools: int, size: int) -> list[list[int]]:
    """
    `pools` lists of `size` game item ids, no item twice. Categories get their share of all picks by size
    (largest remainder), and every pool gets a similar mix: picks are spread evenly over the pools.
    """
    total = pools * size
    shares = {cat: total * len(ids) / CATALOG_ELIGIBLE for cat, ids in CATALOG_ITEMS.items()}
    counts = {cat: int(share) for cat, share in shares.items()}
    left = total - sum(counts.values())
    for cat in sorted(shares, key=lambda c: counts[c] - shares[c])[:left]:
        counts[cat] += 1

    # Pick j of n in a category sits at (j + 0.5) / n along the pool order, so each pool gets its part
    spread = []
    for cat, ids in CATALOG_ITEMS.items():
        picks = random.sample(ids, counts[cat])
        spread += [((j + 0.5) / len(picks), random.random(), item_id) for j, item_id in enumerate(picks)]
    order = [item_id for _, _, item_id in sorted(spread)]
    return [order[k * size:(k + 1) * size] for k in range(pools)]
