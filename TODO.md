# Game Bridge & SDK TODO — Full Modding Platform

Goal: **complete modding platform** — not just read-only. Mods can **read and write** game state, **modify world/player/entities**, **intercept and cancel events**, and eventually add **custom blocks, mobs, items, and dimensions**.

---

## Architecture principle: main-thread writes

- **Reads:** GameBridge getters can be called from the mod (Node) thread (current pattern).
- **Writes:** All state-changing actions must run on the **game main thread**. Implement a **pending action queue**: mod enqueues `SetBlock`, `SetPlayerHealth`, etc.; game drains the queue each tick and applies changes. Same pattern as pending chat.

---

## Phase A: Setters & mutation (player + world) — IMPLEMENTED

### A1. Pending action queue (C++)

- [x] **`ModActionQueue`** (`action_queue.h/cpp`): Thread-safe `std::deque` + mutex. Enqueue from mod thread; **`Minecraft::tick()`** calls `ApplyPendingActions(level, player)` after chat.
- [x] **Actions:** SetBlock, SetPlayerHealth, SetFoodLevel, SetSaturation, SetPosition, SetRotation, SetGameMode, SetLevelTime, SetDayTime, SetRaining, SetThundering, SetDifficulty, GiveExperienceLevels, GiveItem, ClearInventory.

### A2. Player setters (SDK) — on `player.get()` object

- [x] **setHealth**, **setFoodLevel**, **setSaturation**, **setPosition(x,y,z)**, **setRotation(pitch,yaw)**, **setGameMode(modeId)** (0/1/2), **addExperienceLevels**, **giveItem(id, count?, aux?)**, **clearInventory**

### A3. World setters (SDK)

- [x] **world.setBlock(x, y, z, id, data?)**, **world.setTime(ticks)**, **world.setDayTime(ticks)**, **world.setRaining**, **world.setThundering**, **world.setDifficulty**

### A4. Docs & types

- [x] **`globals.d.ts` Player** extended with getters + setters. Setters apply **next game tick** (queued). Multiplayer: host/single-player reliable; clients may desync if server overrides state.

---

## Phase B: Entity list + entity actions

- [ ] **GameBridge:** `GetEntityList(radius?, EntityInfo[] out)` — snapshot entity id, type, position, name; optionally filter by distance from player.
- [ ] **entity.list(radius?)** / **entity.getNearby(radius?)** → array of `{ id, type, x, y, z, name? }`.
- [ ] **entity.get(id)** → same shape (read-only snapshot).
- [ ] **Entity setters (enqueue on main thread):** `entity.teleport(id, x, y, z)`, `entity.setHealth(id, value)`, `entity.remove(id)` / kill. Requires resolving entity by id on main thread and calling Level/Entity APIs.

---

## Phase C: Event interception & cancellable events

### C1. New events (emit from game)

- [ ] **Block break:** `before_break_block`, `after_break_block` (payload: position, block id, player?)
- [ ] **Block place:** `before_place_block`, `after_place_block` (position, block id, player?)
- [ ] **Entity hurt/death:** `entity_hurt`, `entity_death` (entity id, source, damage?)
- [ ] **Player move / tick:** `player_tick` or `player_move` (position delta or new position) — if not too noisy
- [ ] **Item use / interact:** `item_use`, `block_interact` (if hooks exist in engine)

### C2. Cancellable events

- [ ] **Pattern:** For events that should be cancellable (e.g. `before_break_block`), payload includes a **cancel token** or **event object** that mod can call `event.cancel()` on.
- [ ] **Implementation:** Game emits event with a unique token; stores "cancelled(token)" in a set. When game continues, it checks the set; if cancelled, skip the default action (e.g. don’t break block). Requires game code to **check** after emitting (e.g. in the block-break path). So: emit → pump event loop / wait for mod response (or defer and check next frame) → check cancelled(token) → proceed or abort.
- [ ] **Design choice:** Sync wait (game blocks until mod handlers run) vs async (game emits, next frame check cancelled). Async is simpler; some engines use "emit then next tick check".

### C3. Event list (target set)

- [ ] `before_break_block`, `after_break_block` (cancellable: before)
- [ ] `before_place_block`, `after_place_block` (cancellable: before)
- [ ] `entity_hurt`, `entity_death`
- [ ] `player_damage` (cancellable?)
- [ ] `item_use`, `block_interact` (when engine hooks exist)

---

## Phase D: Custom blocks

- [ ] **Block registry:** Engine has a fixed set of block IDs. Option A: reserve an "external" range for mod blocks (e.g. 1000–1999). Option B: dynamic registry that assigns IDs at load.
- [ ] **Registration:** Mod calls `blocks.register(id, name, options?)`. C++ stores mapping; when world tries to get/set block in that range, use mod-defined behavior or a **placeholder** (e.g. generic solid block that looks like stone until we have custom rendering).
- [ ] **Placeholder behavior:** Custom block ID → default tile (e.g. stone) for rendering; optional callback on place/break/neighbor changed (event-based).
- [ ] **Rendering:** Long-term: allow mod to supply texture path or atlas coords; engine draws. Short-term: use existing tile as stand-in.

---

## Phase E: Custom items

- [ ] **Item registry:** Reserve item ID range for mods; register name, max stack, etc.
- [ ] **Behavior:** On use (e.g. right-click): fire `item_use` event with item id so mod can run logic. No custom mesh/icon initially; use placeholder texture.
- [ ] **Crafting:** Optional: register recipe that produces custom item (if recipe system is extensible).

---

## Phase F: Custom mobs / entities

- [ ] **Entity type registry:** Register "entity type" (id, name). Spawn returns entity id; engine treats as generic or uses a default model (e.g. pig/cube).
- [ ] **Spawning:** `entity.spawn(typeId, x, y, z)` enqueued; main thread creates entity and adds to level.
- [ ] **AI/behavior:** Initially minimal (static or copy of existing mob). Later: scriptable AI (e.g. mod provides "tick" callback for the entity).
- [ ] **Rendering:** Placeholder model; later custom model/texture if engine supports.

---

## Phase G: New dimensions (stretch)

- [ ] **Dimension registry:** Game has overworld, nether, end. Adding dimensions requires world creation, chunk source, and teleport support. Likely large engine changes.
- [ ] **Plan:** Document as stretch goal; investigate whether Level/Dimension can be extended or we only support "sub-worlds" (e.g. arenas in same dimension).

---

## Already done (reference)

- **Player (read):** name, id, getWorld, getLocation, health, gameMode, food, saturation, XP, inventory, isSneaking, isSprinting, getSelectedItem, dimension, pitch, yaw, isInWater, isOnFire, isUsingItem, isFlying, canFly.
- **World (read):** getBlock, getTime, getDayTime, isRaining, isThundering, getDifficulty, getSeed, getName.
- **Chat:** player_chat event, chat.send(message).
- **Commands:** command event on `/cmd args`; optional sync registration later.
- **Location:** worldLocation.create(x,y,z), distanceTo.
- **Engine:** log, getTime; types generated for modules and events.

---

## Implementation notes

- **Thread safety:** Reads from mod thread; writes via pending queue to main thread.
- **Multiplayer:** Some setters may only apply to local player or host; document. Custom content (blocks, items, entities) must sync with other players (packets/registry sync) — separate track.
- **Types:** Keep `.d.ts` and docs in sync with new modules and event payloads.
- **Errors:** Return `null` / sentinel when not in world; document. Queue actions that fail (e.g. invalid coords) and optionally report via callback or next-tick error event.

---

## Summary order of work

| Order | Phase | Focus |
|-------|--------|--------|
| 1 | A | Pending action queue + player/world setters |
| 2 | B | Entity list + entity teleport/health/remove |
| 3 | C | New events + cancellable before/after |
| 4 | D | Custom block registry + placeholder behavior |
| 5 | E | Custom item registry + use event |
| 6 | F | Custom entity type + spawn |
| 7 | G | Dimensions (investigation + design) |
