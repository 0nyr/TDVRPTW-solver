#include "nyr/vrp/delta.h"

namespace nyr {

ARTFs make_artfs(const VRPInstance& instance) {
    size_t nb_vertices = instance.D.NbVertices();
    ARTFs artfs(nb_vertices, nb_vertices);
    for (auto& e: instance.D.Arcs()) {
        // Get the arrival time function for arc e.
        // After Lera's preprocessing, this "arrival time function"
        // is the same an ARTF (Arc Ready Time Function).
        auto& arr = instance.arr[e.tail][e.head];
        artfs[e.tail][e.head] = NDCPWLF(arr);
    }
    return artfs;
}

NDCPWLF perform_tree_chain_composition(
    const VRPInstance& instance,
    const ARTFs& deltas,
    const goc::GraphPath& path
) {
    size_t nb_traversed_arcs = path.size() - 1;
    #ifndef NDEBUG
    if (nb_traversed_arcs <= 0) {
        throw std::invalid_argument("Path must contain at least one arc.");
    }
    #endif

    // First init loop done manually over refs of ARTFs
    size_t nb_composed_functions = (size_t) path.size() / 2;
    std::vector<nyr::NDCPWLF> composed_functions;
    composed_functions.reserve(nb_composed_functions);
    size_t k = (size_t) (path.size() - 1) / 2;
    for (size_t i = 0; i < k; ++i) {
        auto& g = deltas[path[2*i]][path[2*i + 1]];
        auto& f = deltas[path[2*i + 1]][path[2*i + 2]];
        composed_functions.push_back(
            f.compose(g)
        );
    }
    if (nb_composed_functions % 2 == 1) {
        // If odd, append the last element to the next list of composed functions
        composed_functions.push_back(
            // Copy of the last arc ARTF
            deltas[path[path.size() - 2]][path[path.size() - 1]]
        );
    }

    // Loop of tree compositions
    while (composed_functions.size() > 1) {
        size_t nb_next_composed_functions = (composed_functions.size() + 1) / 2;
        std::vector<nyr::NDCPWLF> next_composed_functions;
        next_composed_functions.reserve(nb_next_composed_functions);
        k = (composed_functions.size()) / 2;
        for (size_t i = 0; i < k; ++i) {
            auto& g = composed_functions[2*i];
            auto& f = composed_functions[2*i + 1];
            next_composed_functions.push_back(
                f.compose(g)
            );
        }
        if (composed_functions.size() % 2 == 1) {
            next_composed_functions.push_back(
                // Move last function into next vector
                std::move(composed_functions.back())
            );
        }
        composed_functions = next_composed_functions; 
    }

    return composed_functions[0];
}

} // namespace nyr