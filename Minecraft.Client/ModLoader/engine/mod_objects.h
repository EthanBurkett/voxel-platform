#pragma once

#include <node_api.h>
#include <vector>

// Shared helpers to create SDK object shapes for N-API.
namespace ModObjects {

struct ItemSlot {
  int id = 0;
  int count = 0;
  int auxValue = 0;
};

// Creates a Location instance { x, y, z, distanceTo(other) }.
napi_value CreateLocation(napi_env env, double x, double y, double z);

// Creates a World instance { name }.
napi_value CreateWorld(napi_env env, const char *nameUtf8);

// Creates an ItemStack instance { id, count, auxValue }.
napi_value CreateItemStack(napi_env env, int id, int count, int auxValue);

// Creates an Inventory instance { slots: ItemStack[], selectedSlot: number, armor: ItemStack[] }.
napi_value CreateInventory(napi_env env, const std::vector<ItemSlot> &slots,
                           int selectedSlot, const std::vector<ItemSlot> &armor);

}  // namespace ModObjects
