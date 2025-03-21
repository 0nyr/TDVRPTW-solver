//
// Created by Gonzalo Lera Romero.
// Grupo de Optimizacion Combinatoria (GOC).
// Departamento de Computacion - Universidad de Buenos Aires.
//

#include "goc/vrp/route.h"

using namespace std;
using namespace nlohmann;

namespace goc
{
Route::Route() : t0(0), duration(0)
{ }

Route::Route(const GraphPath& path, double t0, double duration)
	: path(path), t0(t0), duration(duration)
{ }

void Route::Print(ostream& os) const
{
	os << json(*this);
}

void to_json(json& j, const Route& r)
{
	j["path"] = r.path;
	j["t0"] = r.t0;
	j["duration"] = r.duration;
}

void from_json(const json& j, Route& r)
{
	vector<int> p = j["path"];
	r.path = p;
	r.t0 = j["t0"];
	r.duration = j["duration"];
}

bool operator==(const Route& r1, const Route& r2)
{
	return r1.path == r2.path && r1.t0 == r2.t0 && r1.duration == r2.duration;
}
} // namespace goc