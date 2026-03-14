#include "stdafx.h"
#include "module_registry.h"

void ModuleRegistry::Register(ModuleDefinition def) {
  Modules().push_back(std::move(def));
}

const std::vector<ModuleDefinition> &ModuleRegistry::GetAll() {
  return Modules();
}

std::vector<ModuleDefinition> &ModuleRegistry::Modules() {
  static std::vector<ModuleDefinition> modules;
  return modules;
}