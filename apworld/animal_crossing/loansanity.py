# Loansanity: paying off each loan sends checks (split by debt), and every house upgrade needs a Progressive
# House. The loan amounts are patched even with Loansanity off. The Statue goal comes after the last loan.
# WARN: the client mirrors the ids (PC_AP_ITEM_PROGRESSIVE_HOUSE, PC_AP_LOC_LOAN_BASE in pc_ap_loansanity.h)!
from BaseClasses import ItemClassification
from worlds.generic.Rules import add_rule

from . import timesanity

PROGRESSIVE_HOUSE = "Progressive House"
ITEMS = {PROGRESSIVE_HOUSE: 0x10000}

# Loans in the order they happen (house upgrades go Medium -> Basement -> Large -> Upper).
LOANS = ["Starting", "Medium", "Basement", "Large", "Upper"]
LOAN_CHECKS_MIN = 1    # every loan sends at least one check
LOAN_CHECKS_MAX = 196  # total_loan_checks max (200) minus the other four loans' minimum
LOAN_BASE_ID = 0x10000  # loan k, check j (1-based): LOAN_BASE_ID + k * 0x1000 + j


def loan_location_name(loan: int, check: int) -> str:
    return f"{LOANS[loan]} Loan {check}"


LOCATIONS = {loan_location_name(k, j): LOAN_BASE_ID + k * 0x1000 + j
             for k in range(len(LOANS)) for j in range(1, LOAN_CHECKS_MAX + 1)}


def split_loan_checks(loans: list[int], total: int) -> list[int]:
    """
    Split `total` checks over the loans: one each, the rest by each loan's share of the total debt.
    Largest remainder, so the result always sums to `total`. Ties go to the earlier loan.
    """
    extra = total - LOAN_CHECKS_MIN * len(loans)
    debt = sum(loans)
    shares = [extra * amount / debt for amount in loans]
    counts = [int(share) for share in shares]
    left = extra - sum(counts)
    by_remainder = sorted(range(len(loans)), key=lambda i: counts[i] - shares[i])
    for i in by_remainder[:left]:
        counts[i] += 1
    return [LOAN_CHECKS_MIN + c for c in counts]


def setup(world) -> None:
    o = world.options
    # Same order as LOANS
    world.loans = [o.starting_loan.value, o.medium_loan.value, o.basement_loan.value, o.large_loan.value,
                   o.upper_loan.value]
    if o.loansanity:
        world.loan_checks = split_loan_checks(world.loans, o.total_loan_checks.value)
    else:
        world.loan_checks = [0] * len(LOANS)
    if world.ut is not None:
        world.loans = world.ut.get("loans", world.loans)
        world.loan_checks = world.ut.get("loan_checks", world.loan_checks)


def locations(world) -> list[str]:
    return [loan_location_name(k, j) for k in range(len(LOANS)) for j in range(1, world.loan_checks[k] + 1)]


def items(world) -> list[str]:
    return [PROGRESSIVE_HOUSE] * (len(LOANS) - 1) if world.options.loansanity else []


def classify(world, name: str):
    return ItemClassification.progression if name == PROGRESSIVE_HOUSE else None


def rules(world) -> None:
    # Loan k only exists after k house upgrades (Progressive House x k); paying needs Bells, so selling needs an
    # open shop. Without Loansanity there are no loan checks (loan_checks is all 0).
    shop = timesanity.shop_open_rule(world)
    for k in range(len(LOANS)):
        for j in range(1, world.loan_checks[k] + 1):
            location = world.get_location(loan_location_name(k, j))
            if k > 0:
                add_rule(location, lambda state, k=k: state.has(PROGRESSIVE_HOUSE, world.player, k))
            if shop:
                add_rule(location, shop)


def statue_goal(world):
    # The statue comes after the last loan, which needs every house upgrade
    return lambda state: state.has(PROGRESSIVE_HOUSE, world.player, len(LOANS) - 1)


def slot_data(world) -> dict:
    return {
        "loans": world.loans,
        "loansanity": world.options.loansanity.value,
        "loan_checks": world.loan_checks,
    }
