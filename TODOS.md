# Mod SDK — Player, Inventory, Items, Menus

Tracking implementation of the full player/inventory/item/menu API for the TypeScript mod SDK.

---

## Phase 1: Player core (read-only)

- [x] **Player: name, id** — already via `player.get()`
- [x] **Player: getWorld(), getLocation()** — already implemented
- [x] **Player: getHealth()** — float 0..20
- [x] **Player: getGameMode()** — "survival" | "creative" | "adventure" | "unknown"
- [x] **Player: getFoodLevel(), getSaturation()** — hunger (0..20), saturation (float)
- [x] **Player: getExperienceLevel(), getTotalExperience()** — XP (int, int)
- [ ] **Player: isAlive()** — boolean (optional; mods can use getHealth() > 0)
- [x] **Type generator** — extend `Player` class in globals.d.ts with new methods

---

## Phase 2: Inventory & items

- [x] **ItemStack** — C++ → JS: `{ id: number, count: number, auxValue: number }`
- [x] **ModObjects::CreateItemStack, CreateInventory** — N-API helpers
- [x] **Player: getInventory()** — returns Inventory object
- [x] **Inventory** — `{ slots: ItemStack[], selectedSlot: number, armor: ItemStack[] }` (36 main + 4 armor)
- [x] **GameBridge** — GetLocalPlayerInventory
- [x] **Type generator** — `ItemStack`, `Inventory` types; `Player.getInventory(): Inventory`

---

## Phase 3: Menus / screens (read-only where possible)

- [ ] **Current screen** — e.g. `player.getCurrentScreen()` or `ui.getCurrentScreen()` — string or enum ("inventory", "crafting", "pause", "none", …)
- [ ] **GameBridge** — hook into Minecraft’s current screen/session (if exposed)
- [ ] **Types** — Screen name type or constants

---

## Phase 4: Menus / UI (actions — if feasible)

- [ ] **Open inventory** — e.g. `player.openInventory()` or `ui.openInventory()` — only if game exposes it
- [ ] **Close current screen** — if safe and exposed
- [ ] **Menu events** — e.g. `events.on("inventory_opened", …)` if we can hook

---

## Phase 5: Polish & docs

- [ ] **Errors** — return null/sentinel when not in world; document in .d.ts
- [ ] **Stub fromUuid** — document as placeholder until multi-player API exists
- [ ] **README or SDK doc** — list Player / Inventory / Item / events

---

## Events: strictly typed

- [x] **Event payload types** — `PlayerJoinEvent`, `PlayerChatEvent`, `WorldLoadedEvent`, `TickEvent`, `JoinWorldEvent` in globals.d.ts
- [x] **events.on() overloads** — `events.on("player_join", (data) => …)` infers `data: PlayerJoinEvent`; autocomplete on event names
- [x] **join_world event** — C++ emits when player joins world; payload `{ playerId, worldName }`
- [x] **events.onJoinWorld(cb)** — `(player: Player, world: World) => void`; bootstrap shim subscribes to `join_world` and calls `cb(player.get(), player.get().getWorld())`

---

## Notes

- **Thread safety**: All GameBridge reads from game state on the mod (Node) thread; ensure no main-thread-only pointers are used unsafely.
- **Inventory size**: `Inventory::getContainerSize()` (36 main + 4 armor in MC); hotbar selected is `inventory->selected`.
- **Game type**: `level->getLevelData()->getGameType()->getId()` (0=survival, 1=creative, 2=adventure).
- **Menus**: Minecraft client uses `Screen` classes; need to find how current screen is exposed (e.g. `Minecraft::getScreen()` or similar).
