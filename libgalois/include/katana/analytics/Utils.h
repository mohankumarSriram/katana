#ifndef KATANA_LIBGALOIS_KATANA_ANALYTICS_UTILS_H_
#define KATANA_LIBGALOIS_KATANA_ANALYTICS_UTILS_H_

#include <algorithm>
#include <random>

#include "katana/ErrorCode.h"
#include "katana/Properties.h"
#include "katana/PropertyGraph.h"
#include "katana/Result.h"

namespace katana::analytics {

// TODO(amp): This file should be disbanded and its functions moved to
// PropertyGraph.h or other more specific places.

//! Used to pick random non-zero degree starting points for search algorithms
//! This code has been copied from GAP benchmark suite
//! (https://github.com/sbeamer/gapbs/blob/master/src/benchmark.h)
class KATANA_EXPORT SourcePicker {
  const PropertyGraph& graph;

public:
  explicit SourcePicker(const PropertyGraph& g) : graph(g) {}

  uint32_t PickNext();
};

//! Used to determine if a graph has power-law degree distribution or not
//! by sampling some of the vertices in the graph randomly
//! This code has been copied from GAP benchmark suite
//! (https://github.com/sbeamer/gapbs/blob/master/src/tc.cc WorthRelabelling())
KATANA_EXPORT bool IsApproximateDegreeDistributionPowerLaw(
    const PropertyGraph& graph);

template <typename Props>
std::vector<std::string>
DefaultPropertyNames() {
  auto num_tuple_elem = std::tuple_size<Props>::value;
  std::vector<std::string> names(num_tuple_elem);

  for (size_t i = 0; i < names.size(); ++i) {
    names[i] = "Column_" + std::to_string(i);
  }
  return names;
}

template <typename NodeProps>
inline katana::Result<void>
ConstructNodeProperties(
    PropertyGraph* pg,
    const std::vector<std::string>& names = DefaultPropertyNames<NodeProps>()) {
  auto res_table = katana::AllocateTable<NodeProps>(pg->num_nodes(), names);
  if (!res_table) {
    return res_table.error();
  }

  return pg->AddNodeProperties(res_table.value());
}

template <typename EdgeProps>
inline katana::Result<void>
ConstructEdgeProperties(
    PropertyGraph* pg,
    const std::vector<std::string>& names = DefaultPropertyNames<EdgeProps>()) {
  auto res_table = katana::AllocateTable<EdgeProps>(pg->num_edges(), names);
  if (!res_table) {
    return res_table.error();
  }

  return pg->AddEdgeProperties(res_table.value());
}

class TemporaryPropertyGuard {
  katana::PropertyGraph* pfg_;
  std::string name_;

  std::string GetPropertyName() {
    // Use this as part of the property name since this will delete the property
    // when it is deconstructed so this name should be unique at any given time.
    return fmt::format(
        "__katana_temporary_property_{}", reinterpret_cast<uintptr_t>(this));
  }

public:
  TemporaryPropertyGuard(PropertyGraph* pg, std::string name)
      : pfg_(pg), name_(name) {}

  explicit TemporaryPropertyGuard(katana::PropertyGraph* pg)
      : TemporaryPropertyGuard(pg, GetPropertyName()) {}

  const TemporaryPropertyGuard& operator=(const TemporaryPropertyGuard&) =
      delete;
  TemporaryPropertyGuard(const TemporaryPropertyGuard&) = delete;

  std::string name() const { return name_; }

  ~TemporaryPropertyGuard() {
    if (auto r = pfg_->RemoveNodeProperty(name_); !r) {
      if (r.error() != katana::ErrorCode::PropertyNotFound) {
        // Log an error if something goes wrong other than the property not
        // existing.
        KATANA_LOG_WARN("Failed to remove temporary property: {}", r.error());
      }
    }
  }
};

}  // namespace katana::analytics

#endif
