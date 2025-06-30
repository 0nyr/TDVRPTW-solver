#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

#include <goc/goc.h>
#include <nyr/nyr.h>
#include <solver.h>

#include "pybind11_json.hpp"

namespace py = pybind11;

PYBIND11_MODULE(kairos_tdvrptw, m) {
    m.doc() = "TDVRPTW library bindings";

    // ==================== GOC NAMESPACE ====================
    py::module_ goc = m.def_submodule("goc", "GOC namespace functions and classes");
    
    // Utility functions
    goc.def("fail", &goc::fail, "Throw an exception with the given message", 
            py::arg("message"), py::arg("exit_code") = 1);
    
    goc.def("string_contains", &goc::string_contains, "Check if container string contains contained string",
            py::arg("container"), py::arg("contained"));
    
    goc.def("remove", &goc::remove, "Remove character from string",
            py::arg("s"), py::arg("c"));
    
    goc.def("split", &goc::split, "Split string by delimiter",
            py::arg("s"), py::arg("delimiter") = ',');
    
    goc.def("trim", &goc::trim, "Remove whitespace from beginning and end of string",
            py::arg("s"));
    
    // Epsilon comparison functions
    goc.def("epsilon_equal", &goc::epsilon_equal, "Check if two doubles are equal within epsilon",
            py::arg("x"), py::arg("y"));
    
    goc.def("epsilon_different", &goc::epsilon_different, "Check if two doubles are different within epsilon",
            py::arg("x"), py::arg("y"));
    
    goc.def("epsilon_smaller", &goc::epsilon_smaller, "Check if x < y within epsilon",
            py::arg("x"), py::arg("y"));
    
    goc.def("epsilon_smaller_equal", &goc::epsilon_smaller_equal, "Check if x <= y within epsilon",
            py::arg("x"), py::arg("y"));
    
    goc.def("epsilon_bigger", &goc::epsilon_bigger, "Check if x > y within epsilon",
            py::arg("x"), py::arg("y"));
    
    goc.def("epsilon_bigger_equal", &goc::epsilon_bigger_equal, "Check if x >= y within epsilon",
            py::arg("x"), py::arg("y"));
    
    // Sum functions
    goc.def("sum",
        static_cast<double (*)(const std::vector<double>&)>(&goc::sum),
        "Sum all numbers in vector",
        py::arg("numbers"));

    // Constants
    goc.attr("EPS") = goc::EPS;
    goc.attr("EPS_SLOPE_ZERO") = goc::EPS_SLOPE_ZERO; 
    goc.attr("INFTY") = goc::INFTY;

    // ==================== GraphPath CLASS ==================== 
    // Bind GraphPath (alias for std::vector<Vertex> i.e. vector<int>)
    py::class_<goc::GraphPath>(goc, "GraphPath")
        .def(py::init<>(), "Create an empty path")
        .def("__len__", [](const goc::GraphPath &p){ return p.size(); })
        .def("__getitem__",
            [](const goc::GraphPath &p, size_t i) {
                if (i >= p.size()) throw py::index_error();
                return p[i];
            })
        .def("append", [](goc::GraphPath &p, int v){ p.push_back(v); },
            py::arg("vertex"), "Append a vertex to the end of the path")
        .def(
            "__eq__",
            [](const goc::GraphPath &a, const goc::GraphPath &b) {
                return a == b;  // calls your operator==
            },
            py::arg("other"),
            "True if two paths have the same sequence of vertices")
        .def(
            "__ne__",
            [](const goc::GraphPath &a, const goc::GraphPath &b) {
                return a != b;
            },
            py::arg("other"),
            "True if two paths have different sequences of vertices")
        .def("__repr__", [](const goc::GraphPath &p){
            std::ostringstream os;
            os << "[";
            for (size_t i = 0; i < p.size(); ++i) {
                if (i) os << ", ";
                os << p[i];
            }
            os << "]";
            return os.str();
        });

    goc.def("has_cycle",
        &goc::has_cycle,
        py::arg("path"),
        py::arg("max_size") = INT_MAX,
        R"pbdoc(
        Returns true if `path` contains a cycle of size <= max_size.
        )pbdoc"
    );

    // ==================== INTERVAL CLASS ====================
    py::class_<goc::Interval>(goc, "Interval")
        .def(py::init<>(), "Create empty interval")
        .def(py::init<double, double>(), "Create interval [left, right]",
            py::arg("left"), py::arg("right"))
        .def_readwrite("left", &goc::Interval::left, "Left boundary of interval")
        .def_readwrite("right", &goc::Interval::right, "Right boundary of interval")
        .def("empty", &goc::Interval::Empty, "Check if interval is empty")
        .def("includes", &goc::Interval::Includes, "Check if value is in interval",
            py::arg("value"))
        .def("is_included_in", &goc::Interval::IsIncludedIn, "Check if this interval is included in other",
            py::arg("other"))
        .def("intersects", &goc::Interval::Intersects, "Check if intervals intersect",
            py::arg("other"))
        .def("intersection", &goc::Interval::Intersection, "Get intersection with other interval",
            py::arg("other"))
        .def("is_point", &goc::Interval::IsPoint, "Check if interval is a single point")
        .def("union", &goc::Interval::Union, "Get union with other interval",
            py::arg("other"))
        .def("__eq__", &goc::Interval::operator==)
        .def("__ne__", &goc::Interval::operator!=)
        .def("__repr__", [](const goc::Interval& i) {
            std::ostringstream oss;
            i.Print(oss);
            return oss.str();
        })
        .def("memory_footprint_bytes", &goc::Interval::memory_footprint_bytes, "Get total memory footprint in bytes");

    // ==================== POINT2D CLASS ====================
    py::class_<goc::Point2D>(goc, "Point2D")
        .def(py::init<double, double>(), "Create 2D point",
            py::arg("x") = 0.0, py::arg("y") = 0.0)
        .def_readwrite("x", &goc::Point2D::x, "X coordinate")
        .def_readwrite("y", &goc::Point2D::y, "Y coordinate")
        .def("__repr__", [](const goc::Point2D& p) {
            std::ostringstream oss;
            p.Print(oss);
            return oss.str();
        })
        .def("memory_footprint_bytes", &goc::Point2D::memory_footprint_bytes, "Get total memory footprint in bytes");

    // ==================== LINEAR FUNCTION CLASS ====================
    py::class_<goc::LinearFunction>(goc, "LinearFunction")
        .def(py::init<>(), "Create default linear function")
        .def(py::init<const goc::Point2D&, const goc::Point2D&>(), 
            "Create linear function from two points",
            py::arg("p1"), py::arg("p2"))
        .def_readwrite("domain", &goc::LinearFunction::domain, "Function domain")
        .def_readwrite("image", &goc::LinearFunction::image, "Function image")
        .def_readwrite("slope", &goc::LinearFunction::slope, "Function slope")
        .def_readwrite("intercept", &goc::LinearFunction::intercept, "Function y-intercept")
        .def("value", &goc::LinearFunction::Value, "Evaluate function at x",
            py::arg("x"))
        .def("__call__", &goc::LinearFunction::operator(), "Evaluate function at x",
            py::arg("x"))
        .def("pre_value", &goc::LinearFunction::PreValue, "Get x such that f(x) = y",
            py::arg("y"))
        .def("intersects", &goc::LinearFunction::Intersects, "Check if functions intersect",
            py::arg("other"))
        .def("intersection", &goc::LinearFunction::Intersection, "Get intersection point with other function",
            py::arg("other"))
        .def("inverse", &goc::LinearFunction::Inverse, "Get inverse function")
        .def("restrict_domain", &goc::LinearFunction::RestrictDomain, "Restrict function domain",
            py::arg("domain"))
        .def("restrict_image", &goc::LinearFunction::RestrictImage, "Restrict function image",
            py::arg("image"))
        .def("__eq__", &goc::LinearFunction::operator==)
        .def("__ne__", &goc::LinearFunction::operator!=)
        .def("__repr__", [](const goc::LinearFunction& f) {
            std::ostringstream oss;
            f.Print(oss);
            return oss.str();
        })
        .def("memory_footprint_bytes", &goc::LinearFunction::memory_footprint_bytes, "Get total memory footprint in bytes");

    // Linear function standalone functions
    goc.def("dom", py::overload_cast<const goc::LinearFunction&>(&goc::dom), "Get function domain");

    // ==================== PWL FUNCTION CLASS ====================
    py::class_<goc::PWLFunction>(goc, "PWLFunction")
        .def(py::init<>(), "Create empty PWL function")
        .def(py::init<const std::vector<goc::LinearFunction>&>(), 
            "Create PWL function from pieces",
            py::arg("pieces"))
        .def(py::init<const std::vector<double>&, const std::vector<double>&>(),
            "Create PWL function from breakpoints and values",
            py::arg("breakpoints"), py::arg("values"))
        .def_static("constant_function", &goc::PWLFunction::ConstantFunction,
                   "Create constant function", py::arg("a"), py::arg("domain"))
        .def_static("identity_function", &goc::PWLFunction::IdentityFunction,
                   "Create identity function", py::arg("domain"))
        .def("add_piece", &goc::PWLFunction::AddPiece, "Add piece to function",
            py::arg("piece"))
        .def("pop_piece", &goc::PWLFunction::PopPiece, "Remove last piece")
        .def("empty", &goc::PWLFunction::Empty, "Check if function is empty")
        .def("piece_count", &goc::PWLFunction::PieceCount, "Get number of pieces")
        .def("pieces", &goc::PWLFunction::Pieces, "Get all pieces", 
            py::return_value_policy::reference_internal)
        .def("piece", &goc::PWLFunction::Piece, "Get i-th piece",
            py::arg("i"), py::return_value_policy::reference_internal)
        .def("__getitem__", &goc::PWLFunction::operator[], "Get i-th piece",
            py::arg("i"), py::return_value_policy::reference_internal)
        .def("first_piece", &goc::PWLFunction::FirstPiece, "Get first piece",
            py::return_value_policy::reference_internal)
        .def("last_piece", &goc::PWLFunction::LastPiece, "Get last piece",
            py::return_value_policy::reference_internal)
        .def("piece_including", &goc::PWLFunction::PieceIncluding, "Get piece index that includes x",
            py::arg("x"))
        .def("domain", &goc::PWLFunction::Domain, "Get function domain")
        .def("image", &goc::PWLFunction::Image, "Get function image")
        .def("value", &goc::PWLFunction::Value, "Evaluate function at x",
            py::arg("x"))
        .def("__call__", &goc::PWLFunction::operator(), "Evaluate function at x",
            py::arg("x"))
        .def("pre_value", &goc::PWLFunction::PreValue, "Get x such that f(x) = y",
            py::arg("y"))
        .def("compose", &goc::PWLFunction::Compose, "Compose with other function",
            py::arg("g"))
        .def("inverse", &goc::PWLFunction::Inverse, "Get inverse function")
        .def("restrict_domain", &goc::PWLFunction::RestrictDomain, "Restrict function domain",
            py::arg("domain"))
        .def("restrict_image", &goc::PWLFunction::RestrictImage, "Restrict function image",
            py::arg("image"))
        .def("check_invariant", &goc::PWLFunction::check_invariant, "Check function invariant")
        .def("check_normalization", &goc::PWLFunction::check_normalization, "Check function normalization")
        .def("copy_breakpoints_and_values", &goc::PWLFunction::copy_breakpoints_and_values,
            "Get breakpoints and values as a pair of vectors",
            py::return_value_policy::reference_internal)
        .def("__eq__", &goc::PWLFunction::operator==)
        .def("__ne__", &goc::PWLFunction::operator!=)
        .def("__radd__", [](const goc::PWLFunction& f, double a) { return a + f; })
        .def("__rsub__", [](const goc::PWLFunction& f, double a) { return a - f; })
        .def("__rmul__", [](const goc::PWLFunction& f, double a) { return a * f; })
        .def("__repr__", [](const goc::PWLFunction& f) {
            std::ostringstream oss;
            f.Print(oss);
            return oss.str();
        })
        .def("memory_footprint_bytes", &goc::PWLFunction::memory_footprint_bytes, "Get total memory footprint in bytes")
        .def("compute_area", &goc::PWLFunction::compute_area, "Compute area under the function");

    // PWL function standalone functions
    goc.def("dom", py::overload_cast<const goc::PWLFunction&>(&goc::dom), "Get function domain");
    goc.def("img", py::overload_cast<const goc::PWLFunction&>(&goc::img), "Get function image");
    goc.def("to_string", &goc::to_string, "Convert PWL function to string", py::arg("f"));
    
    // Max/Min functions
    goc.def("max", py::overload_cast<const goc::PWLFunction&, const goc::PWLFunction&>(&goc::Max),
            "Pointwise maximum of two functions", py::arg("f"), py::arg("g"));
    goc.def("max", py::overload_cast<const goc::PWLFunction&, double>(&goc::Max),
            "Pointwise maximum of function and constant", py::arg("f"), py::arg("a"));
    goc.def("max", py::overload_cast<double, const goc::PWLFunction&>(&goc::Max),
            "Pointwise maximum of constant and function", py::arg("a"), py::arg("f"));
    
    goc.def("min", py::overload_cast<const goc::PWLFunction&, const goc::PWLFunction&>(&goc::Min),
            "Pointwise minimum of two functions", py::arg("f"), py::arg("g"));
    goc.def("min", py::overload_cast<const goc::PWLFunction&, double>(&goc::Min),
            "Pointwise minimum of function and constant", py::arg("f"), py::arg("a"));
    goc.def("min", py::overload_cast<double, const goc::PWLFunction&>(&goc::Min),
            "Pointwise minimum of constant and function", py::arg("a"), py::arg("f"));

    // ==================== NYR NAMESPACE ====================
    py::module_ nyr = m.def_submodule("nyr", "NYR namespace functions and classes");

    // ==================== NDCPWLF CLASS ====================
    py::class_<nyr::NDCPWLF>(nyr, "NDCPWLF")
        .def(py::init<>(), "Create empty NDCPWLF")
        .def(py::init<const std::vector<double>, const std::vector<double>>(),
            "Create NDCPWLF from breakpoints and values",
            py::arg("breakpoints"), py::arg("values"))
        .def_static("make_identity", &nyr::NDCPWLF::make_identity,
                   "Create identity function", py::arg("domain"))
        .def("empty", &nyr::NDCPWLF::empty, "Check if function is empty")
        .def("nb_pieces", &nyr::NDCPWLF::nb_pieces, "Get number of pieces")
        .def("evaluate", &nyr::NDCPWLF::evaluate, "Evaluate function at x",
            py::arg("x"))
        .def("__call__", &nyr::NDCPWLF::operator(), "Evaluate function at x",
            py::arg("x"))
        .def("check_invariant", &nyr::NDCPWLF::check_invariant, "Check function invariant")
        .def("check_normalization", &nyr::NDCPWLF::check_normalization, "Check if function is normalized")
        .def("compose", &nyr::NDCPWLF::compose_alternative, "Compose with other NDCPWLF",
            py::arg("g"))
        .def("compose_visser", &nyr::NDCPWLF::compose_visser, "Compose with Visser's method without normalization",
            py::arg("g"))
        .def("compose", &nyr::NDCPWLF::compose,
            "Compose with Visser's method with normalization",
            py::arg("g"))
        .def("to_goc_pwl_function", &nyr::NDCPWLF::to_goc_pwl_function, "Convert to GOC PWL function")
        .def("get_min_domain", &nyr::NDCPWLF::get_min_domain, "Get minimum domain value")
        .def("get_max_domain", &nyr::NDCPWLF::get_max_domain, "Get maximum domain value")
        .def("get_min_image", &nyr::NDCPWLF::get_min_image, "Get minimum image value")
        .def("get_max_image", &nyr::NDCPWLF::get_max_image, "Get maximum image value")
        .def("get_domain", &nyr::NDCPWLF::get_domain, "Get function domain")
        .def("get_image", &nyr::NDCPWLF::get_image, "Get function image")
        .def("get_xs", &nyr::NDCPWLF::get_xs, "Get breakpoints",
            py::return_value_policy::reference_internal)
        .def("get_ys", &nyr::NDCPWLF::get_ys, "Get values",
            py::return_value_policy::reference_internal)
        .def("copy_breakpoints_and_values", &nyr::NDCPWLF::copy_breakpoints_and_values,
            "Get copy of breakpoints and values as pair")
        .def("memory_footprint_bytes", &nyr::NDCPWLF::memory_footprint_bytes,
            "Get memory footprint in bytes")
        .def("__eq__", &nyr::NDCPWLF::operator==)
        .def("__repr__", [](const nyr::NDCPWLF& f) {
            std::ostringstream oss;
            f.Print(oss);
            return oss.str();
        })
        .def("compute_area", &nyr::NDCPWLF::compute_area, "Compute area under the function");

    // ==================== ADDITIONAL UTILITY FUNCTIONS ====================
    
    // Test interval intersects
    nyr.def("test_interval_vector_intersects", &nyr::test_interval_vector_intersects, "Run tests for interval_vector_intersects");

    // Test interval includes
    nyr.def("test_interval_vector_includes", &nyr::test_interval_vector_includes, "Run tests for interval_vector_includes");

    // Print padded vectors function
    m.def("print_padded_vectors", [](const std::vector<double>& vec1, const std::vector<double>& vec2) {
        std::ostringstream oss;
        goc::print_padded_vectors(oss, vec1, vec2);
        return oss.str();
    }, "Print two vectors with aligned padding", py::arg("vec1"), py::arg("vec2"));

    // STR macro equivalent
    m.def("str", [](py::object obj) {
        return py::str(obj);
    }, "Convert object to string (equivalent to STR macro)", py::arg("obj"));

    // ==================== RouteMakespan CLASS ====================
    py::class_<nyr::RouteMakespan, std::shared_ptr<nyr::RouteMakespan>>(m, "RouteMakespan")
        .def(py::init<>(),
            "Create an empty makespan-route (path={} , value=0.0)")
        .def(py::init<const goc::GraphPath&, double>(),
            py::arg("path"), py::arg("makespan"),
            "Create a makespan-route with given path and makespan value")
        // give Python direct read/write access to the members:
        .def_readwrite("path",  &nyr::RouteMakespan::path,
            "The sequence of vertices in this route")
        .def_readwrite("value", &nyr::RouteMakespan::value,
            "The makespan value")
        // repr via the JSON-printing Print() override:
        .def("__repr__",
            [](const nyr::RouteMakespan &r) {
                std::ostringstream oss;
                r.Print(oss);
                return oss.str();
            },
            "Stringify to JSON using the built-in Print()")
        // equality:
        .def("__eq__", 
            [](const nyr::RouteMakespan &a, const nyr::RouteMakespan &b){
                return a == b;
            },
            py::arg("other"),
            "True if two makespan routes are identical")
        .def("__ne__",
            [](const nyr::RouteMakespan &a, const nyr::RouteMakespan &b){
                return a != b;
            },
            py::arg("other"),
            "True if two makespan routes differ");
    
    // ==================== RouteDuration CLASS ====================
    py::class_<nyr::RouteDuration, std::shared_ptr<nyr::RouteDuration>>(m, "RouteDuration")
        .def(py::init<>(),
            "Create an empty duration‐route (path={} , t0=0.0, duration=0.0)")
        .def(py::init<const goc::GraphPath&, double, double>(),
             py::arg("path"), py::arg("t0"), py::arg("duration"),
            "Create a duration‐route with given path, dispatch time t0, and duration")
        .def_readwrite("path", &nyr::RouteDuration::path,
            "The sequence of vertices in this route")
        .def_readwrite("t0",   &nyr::RouteDuration::t0,
            "Dispatch time (start time) of the first vertex")
        .def_readwrite("value", &nyr::RouteDuration::value,
            "The total duration value")
        .def("__repr__",
            [](const nyr::RouteDuration &r) {
                std::ostringstream oss;
                r.Print(oss);
                return oss.str();
            },
            "Stringify to JSON using the built-in Print()")
        .def("__eq__", 
            [](const nyr::RouteDuration &a, const nyr::RouteDuration &b){
                return a == b;
            },
            py::arg("other"),
            "True if two duration routes are identical")
        .def("__ne__",
            [](const nyr::RouteDuration &a, const nyr::RouteDuration &b){
                return a != b;
            },
            py::arg("other"),
            "True if two duration routes differ");

    // ==================== VRPInstance CLASS ====================
    py::class_<nyr::VRPInstance>(nyr, "VRPInstance")
        .def(py::init<>(), "Create empty VRPInstance")
        .def_readonly("o", &nyr::VRPInstance::o, "Origin vertex index (depot)")
        .def_readonly("d", &nyr::VRPInstance::d, "Destination vertex index (depot)")
        .def("nb_vertices", &nyr::VRPInstance::nb_vertices, "Get number of vertices")
        .def("nb_clients", &nyr::VRPInstance::nb_clients, "Get number of clients (excluding depots)")
        .def("__repr__", [](const nyr::VRPInstance& instance) {
            std::ostringstream oss;
            instance.Print(oss);
            return oss.str();
        });

    // ==================== ARTFs (Matrix of NDCPWLF) CLASS ====================
    py::class_<goc::Matrix<nyr::NDCPWLF>>(nyr, "ARTFs")
        .def(py::init<int, int>(), py::arg("row_count") = 0, py::arg("col_count") = 0,
             "Create empty matrix of NDCPWLF")
        .def(py::init([](const nyr::VRPInstance& instance) {
                return nyr::make_artfs(instance);
            }),
            "Create matrix of NDCPWLF from VRPInstance",
            py::arg("instance"))
        .def("row_count", &goc::Matrix<nyr::NDCPWLF>::row_count, "Get number of rows")
        .def("column_count", &goc::Matrix<nyr::NDCPWLF>::column_count, "Get number of columns")
        .def("size", &goc::Matrix<nyr::NDCPWLF>::size, "Get number of cells")
        .def("__getitem__", [](goc::Matrix<nyr::NDCPWLF>& m, int row) { return m[row]; }, py::return_value_policy::reference_internal)
        .def("__setitem__", [](goc::Matrix<nyr::NDCPWLF>& m, int row, const std::vector<nyr::NDCPWLF>& v) { m[row] = v; })
        .def("__call__", py::overload_cast<int, int>(&goc::Matrix<nyr::NDCPWLF>::operator(), py::const_), py::arg("row"), py::arg("col"),
             py::return_value_policy::reference_internal, "Get cell value (const)")
        .def("__call__", py::overload_cast<int, int>(&goc::Matrix<nyr::NDCPWLF>::operator()), py::arg("row"), py::arg("col"),
             py::return_value_policy::reference_internal, "Get cell value (mutable)")
        .def("at", &goc::Matrix<nyr::NDCPWLF>::at, py::arg("row"), py::arg("col"),
             py::return_value_policy::reference_internal, "Get cell value with bounds checking")
        .def("clear", &goc::Matrix<nyr::NDCPWLF>::clear, "Clear matrix to default values")
        .def("__repr__", [](const goc::Matrix<nyr::NDCPWLF>& m) {
            std::ostringstream oss;
            m.Print(oss);
            return oss.str();
        });
    
    nyr.def(
        "make_artfs",
        &nyr::make_artfs,
        py::arg("instance"),
        R"pbdoc(
        Create a matrix of NDCPWLF from a VRPInstance.
        This is the main function to create the ARTFs for the solver.
        )pbdoc"
    );

    nyr.def(
        "perform_tree_chain_composition",
        &nyr::perform_tree_chain_composition,
        py::arg("instance"),
        py::arg("deltas"),
        py::arg("path"),
        R"pbdoc(
        Perform the (Visser et al 2020) tree-chain composition:
        
        - `instance`   : a loaded VRPInstance
        - `deltas`     : the ARTFs matrix (Matrix<NDCPWLF>)
        - `path`       : a goc.GraphPath of vertices
        Returns an NDCPWLF = composition of the arc-ready-time functions along the path.
        )pbdoc"
    );

    nyr.def(
        "perform_sequential_chain_composition",
        &nyr::perform_sequential_chain_composition,
        py::arg("instance"),
        py::arg("deltas"),
        py::arg("path"),
        R"pbdoc(
        Perform the sequential chain composition:
        - `instance`   : a loaded VRPInstance
        - `deltas`     : the ARTFs matrix (Matrix<NDCPWLF>)
        - `path`       : a goc.GraphPath of vertices
        Returns an NDCPWLF equals to the composition of the arc-ready-time functions along the path.
        This is a less optimal version of the tree-chain composition.
        )pbdoc"
    );

    nyr.def(
        "compute_optimal_departure_time_and_duration",
        &nyr::compute_optimal_departure_time_and_duration,
        py::arg("delta_path"),
        R"pbdoc(
        Compute the optimal departure time and duration for a given path of NDCPWLFs.
        - `delta_path` : a nyr.NDCPWLF function associated with a path of vertices.
        Returns a tuple (t0, duration) where:
        - `t0`        : the optimal departure time
        - `duration`  : the total duration of the route
        )pbdoc"
    );

    nyr.def(
        "compute_RouteDuration_from_delta_path",
        py::overload_cast<const nyr::NDCPWLF&, const goc::GraphPath&>(&nyr::compute_RouteDuration),
        py::arg("delta_path"),
        py::arg("path"),
        R"pbdoc(
        Return a RouteDuration object that contains its own copy of the path, t0, and duration.
        - `delta_path`: NDCPWLF function for the path
        - `path`: goc.GraphPath of vertices
        Returns a RouteDuration object.
        )pbdoc"
    );

    nyr.def(
        "compute_RouteDuration_from_scratch",
        py::overload_cast<const nyr::VRPInstance&, const goc::Matrix<nyr::NDCPWLF>&, const goc::GraphPath&>(&nyr::compute_RouteDuration),
        py::arg("instance"),
        py::arg("deltas"),
        py::arg("path"),
        R"pbdoc(
        Returns a RouteDuration provided its path.
        - `instance`: VRPInstance
        - `deltas`: ARTFs matrix (Matrix<NDCPWLF>)
        - `path`: goc.GraphPath of vertices
        Returns a RouteDuration object.
        )pbdoc"
    );

    nyr.def(
        "compute_RouteDuration_lera",
        &nyr::compute_RouteDuration_lera,
        py::arg("instance"),
        py::arg("path"),
        R"pbdoc(
        Returns a RouteDuration provided its path using the Lera-Romero procedure (unoptimal).
        - `instance`: VRPInstance
        - `path`: goc.GraphPath of vertices
        Returns a RouteDuration object.
        )pbdoc"
    );

    // ==================== solver NAMESPACE ====================
    // py::module_ solver = m.def_submodule("solver", "Solver namespace functions and classes");

    // bind loader: Python dict → nlohmann::json → C++ VRPInstance
    m.def(
        "load_instance_from_json",
        &solver::load_instance_from_json,
        py::arg("instance"),
        R"pbdoc(
        Load a VRPInstance from a Python dict (or JSON string, list, etc.).
        Internally we run your C++ preprocessors on the parsed JSON and then
        deserialize into a nyr::VRPInstance.
        )pbdoc"
    );

}
