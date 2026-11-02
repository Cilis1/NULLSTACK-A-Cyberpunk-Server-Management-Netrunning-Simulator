# Design decisions

## Foundation before breadth
A small deterministic command-driven model is preferable to empty interfaces for every future subsystem. It is a test fixture, not a representation of the final economy.

## Stable reproducibility
The core uses fixed-width integer arithmetic and explicit PRNG and hash algorithms. Replay input order is significant and invalid input is rejected rather than silently repaired.

## Persistence boundary
Persistence is not implemented yet. The intended design isolates SQLite in a persistence library and uses transactional snapshots; replay is a separate reproducibility tool. Schema changes will require explicit migrations before a future schema is accepted.

## Licensing
MIT for code to minimize contribution and integration friction. No third-party art or font is bundled at this stage.
