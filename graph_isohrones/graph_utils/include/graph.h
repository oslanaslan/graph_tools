#pragma once

#include <unordered_map>

namespace graph {
template <typename Vertex>
class Graph {
    using WeightsMap = std::unordered_map<Vertex, std::unordered_map<Vertex, float>>;
    WeightsMap weights_;

   public:
    Graph() = default;
    Graph(const Graph&) = delete;
    explicit Graph(WeightsMap&& weights_map)
        : weights_(std::move(weights_map)){};
    const std::unordered_map<Vertex, float>& get_neighbors(const Vertex& v) const {
        return this->weights_.at(v);
    }
};
}  // namespace graph