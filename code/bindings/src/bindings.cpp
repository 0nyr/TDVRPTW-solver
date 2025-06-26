#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

#include <goc/goc.h>
#include <nyr/nyr.h>

namespace py = pybind11;

PYBIND11_MODULE(kairos_tdvrptw, m) {
    m.doc() = "Piecewise Linear Function Library";

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
    goc.attr("INFTY") = goc::INFTY;

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
        .def("compose", &nyr::NDCPWLF::compose, "Compose with other NDCPWLF",
             py::arg("g"))
        .def("compose_visser", &nyr::NDCPWLF::compose_visser, "Compose with Visser's method without normalization",
             py::arg("g"))
        .def("compose_visser_normalization", &nyr::NDCPWLF::compose_visser_normalization,
            "Compose with Visser's method with normalization",
            py::arg("g"))
        .def("to_goc_pwl_function", &nyr::NDCPWLF::to_goc_pwl_function, "Convert to GOC PWL function")
        .def("get_min_domain", &nyr::NDCPWLF::get_min_domain, "Get minimum domain value")
        .def("get_max_domain", &nyr::NDCPWLF::get_max_domain, "Get maximum domain value")
        .def("get_min_image", &nyr::NDCPWLF::get_min_image, "Get minimum image value")
        .def("get_max_image", &nyr::NDCPWLF::get_max_image, "Get maximum image value")
        .def("get_domain", &nyr::NDCPWLF::get_domain, "Get function domain")
        .def("get_image", &nyr::NDCPWLF::get_image, "Get function image")
        .def("get_breakpoints", &nyr::NDCPWLF::get_breakpoints, "Get breakpoints",
             py::return_value_policy::reference_internal)
        .def("get_values", &nyr::NDCPWLF::get_values, "Get values",
             py::return_value_policy::reference_internal)
        .def("copy_breakpoints_and_values", &nyr::NDCPWLF::copy_breakpoints_and_values,
             "Get copy of breakpoints and values as pair")
        .def("memory_footprint_bytes", &nyr::NDCPWLF::memory_footprint_bytes,
             "Get memory footprint in bytes")
        .def_readonly("xs", &nyr::NDCPWLF::xs, "Sorted x-values (breakpoints)")
        .def_readonly("ys", &nyr::NDCPWLF::ys, "Corresponding y-values")
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
}