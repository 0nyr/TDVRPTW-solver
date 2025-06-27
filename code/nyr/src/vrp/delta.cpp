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

} // namespace nyr