//
// Created by Gonzalo Lera Romero.
// Grupo de Optimizacion Combinatoria (GOC).
// Departamento de Computacion - Universidad de Buenos Aires.
//

#include "goc/math/pwl_function.h"

#include "goc/print/print_utils.h"
#include "goc/exception/exception_utils.h"
#include "goc/string/string_utils.h"
#include "goc/math/number_utils.h"

using namespace std;
using namespace nlohmann;

namespace goc
{

PWLFunction PWLFunction::ConstantFunction(double a, Interval domain)
{
	PWLFunction f;
	f.AddPiece(LinearFunction(Point2D(domain.left, a), Point2D(domain.right, a)));
	return f;
}

PWLFunction PWLFunction::IdentityFunction(Interval domain)
{
	PWLFunction f;
	f.AddPiece(LinearFunction(Point2D(domain.left, domain.left), Point2D(domain.right, domain.right)));
	return f;
}

PWLFunction::PWLFunction()
{
	domain_ = image_ = {INFTY, -INFTY};
}

PWLFunction::PWLFunction(const std::vector<LinearFunction>& pieces) : PWLFunction()
{
	for (auto& p: pieces) AddPiece(p);
}

PWLFunction::PWLFunction(
	const vector<double>& breakpoints,
    const vector<double>& values
) {
	if (breakpoints.empty() || values.empty() || breakpoints.size() != values.size()) {
        throw invalid_argument("Breakpoints and values must be non-empty and of the same size.");
    }
    
    #ifndef NDEBUG
    // Check if breakpoints are sorted and unique
    for (size_t i = 1; i < breakpoints.size(); ++i) {
        if (epsilon_smaller_equal(breakpoints[i], breakpoints[i-1])) {
            throw invalid_argument("Breakpoints must be sorted and unique.");
        }
    }
    #endif

	pieces_.reserve(breakpoints.size() - 1);
	for (size_t i = 0; i < breakpoints.size() - 1; ++i)
	{
		double x_left = breakpoints[i];
		double x_right = breakpoints[i + 1];
		double y_left = values[i];
		double y_right = values[i + 1];
		
		AddPiece(LinearFunction(
			Point2D(x_left, y_left),
			Point2D(x_right, y_right)
		));
	}

	#ifndef NDEBUG
	// Check if the function is normalized.
	if (!check_invariant()) {
		throw runtime_error("PWLFunction invariant violated after construction.");
	}
	#endif
}

void PWLFunction::AddPiece(const LinearFunction& piece)
{
	// If this piece is a continuation of the last piece, then we need to merge them into one piece to have the
	// function normalized.
	bool is_continuation_of_last_piece = false;
	if (!pieces_.empty())
	{
		if (epsilon_equal(pieces_.back().domain.right, piece.domain.left))
		{
			double right_val = pieces_.back().Value(max(dom(pieces_.back())));
			double left_val = piece.Value(min(dom(piece)));
			is_continuation_of_last_piece = epsilon_equal(left_val, right_val) && (pieces_.back().domain.IsPoint() || epsilon_equal(pieces_.back().slope, piece.slope));
		}
	}
	
	if (!is_continuation_of_last_piece)
	{
		// Add the new piece if it is not a continuation.
		pieces_.push_back(piece);
	}
	else
	{
		// Merge pieces to have the function normalized.
		pieces_.back().domain.right = piece.domain.right;
		pieces_.back().image.left = min(pieces_.back().image.left, piece.image.left);
		pieces_.back().image.right = max(pieces_.back().image.right, piece.image.right);
		pieces_.back().slope = piece.slope;
		pieces_.back().intercept = piece.intercept;
	}
	
	// Update domain and image.
	if (domain_.left == INFTY) domain_.left = piece.domain.left;
	domain_.right = piece.domain.right;
	image_.left = min(image_.left, piece.image.left);
	image_.right = max(image_.right, piece.image.right);
}

void PWLFunction::PopPiece()
{
	pieces_.pop_back();
	domain_ = Empty() ? Interval(INFTY, -INFTY) : Interval(domain_.left, pieces_.back().domain.right);
	UpdateImage();
}

bool PWLFunction::Empty() const
{
	return pieces_.empty();
}

int PWLFunction::PieceCount() const
{
	return pieces_.size();
}

const vector<LinearFunction>& PWLFunction::Pieces() const
{
	return pieces_;
}

const LinearFunction& PWLFunction::Piece(int i) const
{
	return pieces_[i];
}

const LinearFunction& PWLFunction::operator[](int i) const
{
	return pieces_[i];
}

const LinearFunction& PWLFunction::FirstPiece() const
{
	return pieces_.front();
}

const LinearFunction& PWLFunction::LastPiece() const
{
	return pieces_.back();
}

int PWLFunction::PieceIncluding(double x) const
{
	// if x is outside the Domain(), throw exception.
	if (epsilon_bigger(domain_.left, x) || epsilon_smaller(domain_.right, x))
	{
		fail("PWLFunction::Value(" + STR(x) +") failed, because domain is " + STR(Domain()));
		return -1;
	}
	
	// Look for a piece that include x in their domain.
	for (int i = pieces_.size()-1; i >= 0; --i)
		if (pieces_[i].domain.Includes(x))
			return i;
	
	// The function is not continuous and x is not in the domain of any piece.
	fail("PWLFunction::Value(" + STR(x) +") failed, because x is not inside the domain of its pieces.");
	return -1;
}

Interval PWLFunction::Domain() const
{
	return domain_;
}

Interval PWLFunction::Image() const
{
	return image_;
}

double PWLFunction::Value(double x) const
{
	// if x is outside the Domain(), throw exception.
	if (epsilon_bigger(domain_.left, x) || epsilon_smaller(domain_.right, x))
	{
		fail("PWLFunction::Value(" + STR(x) +") failed, because domain is " + STR(Domain()));
		return -1;
	}
	
	// Look for a piece that include x in their domain.
	for (int i = pieces_.size()-1; i >= 0; --i)
		if (pieces_[i].domain.Includes(x))
			return pieces_[i].Value(x);
	
	// The function is not continuous and x is not in the domain of any piece.
	fail("PWLFunction::Value(" + STR(x) +") failed, because x is not inside the domain of its pieces.");
	return -1;
}

double PWLFunction::operator()(double x) const
{
	return Value(x);
}

double PWLFunction::PreValue(double y) const
{
	if (epsilon_bigger(image_.left, y) || epsilon_smaller(image_.right, y))
	{
		fail("PWLFunction::PreValue(" + STR(y) +") failed, because image is " + STR(Image()));
		return -1;
	}
	
	// Look for a piece that include x in their domain.
	for (int i = (int)pieces_.size()-1; i >= 0; --i)
		if (pieces_[i].image.Includes(y))
			return pieces_[i].PreValue(y);
	
	// The function is not continuous and x is not in the domain of any piece.
	fail("PWLFunction::PreValue(" + STR(y) +") failed, because y is not inside the domain of its pieces.");
	return -1;
}

PWLFunction PWLFunction::Compose(const PWLFunction& g) const
{
	PWLFunction fog;
	
	auto& f = *this;
	int i = 0;
	for (int j = 0; j < g.PieceCount(); ++j)
	{
		// Put i inside bounds.
		i = max(0, min(f.PieceCount()-1, i));
		
		// If g[j] is constant, image is a single y = min(img(g[j])) = max(img(g[j])).
		if (epsilon_equal(g[j].slope, 0.0))
		{
			double y = min(img(g[j]));
			
			// Find (unique) piece f[i] with img(g[j]) \subseteq dom(f[i]) if exists.
			while (i < f.PieceCount() && epsilon_smaller(max(dom(f[i])), y)) ++i;
			while (i >= 0 && epsilon_bigger(min(dom(f[i])), y)) --i;
			
			// If found f[i] such that dom(f[i]) \cap img(g[j]) \neq \emptyset, add the piece to fog.
			if (i >= 0 && i < f.PieceCount() && dom(f[i]).Includes(y))
				fog.AddPiece(LinearFunction({min(dom(g[j])), f[i](y)}, {max(dom(g[j])), f[i](y)}));
		}
		// If g[j] is increasing.
		else if (epsilon_bigger(g[j].slope, 0.0))
		{
			// Find first piece f[i] such that max(dom(f[i])) >= min(img(g[j])).
			while (i > 0 && epsilon_bigger_equal(max(dom(f[i-1])), min(img(g[j])))) --i;
			while (i < f.PieceCount() && epsilon_smaller(max(dom(f[i])), min(img(g[j])))) ++i;
			
			// For each piece f[i] such that dom(f[i]) \cap img(g[j]) \neq \emptyset
			for (; i < PieceCount() && dom(f[i]).Intersects(img(g[j])); ++i)
			{
				// Find intersection.
				Interval inter = dom(f[i]).Intersection(img(g[j]));
				double left = g[j].PreValue(inter.left), right = g[j].PreValue(inter.right);
				fog.AddPiece(LinearFunction({left, f[i](inter.left)}, {right, f[i](inter.right)}));
			}
		}
		// If g[j] is decreasing.
		else if (epsilon_smaller(g[j].slope, 0.0))
		{
			// Find last piece f[i] such that min(dom(f[i])) <= max(img(g[j])).
			while (i < f.PieceCount()-1 && epsilon_smaller_equal(min(dom(f[i+1])), max(img(g[j])))) ++i;
			while (i >= 0 && epsilon_bigger(min(dom(f[i])), max(img(g[j])))) --i;
			
			// For each piece f[i] such that dom(f[i]) \cap img(g[j]) \neq \emptyset
			for (; i >= 0 && dom(f[i]).Intersects(img(g[j])); --i)
			{
				// Find intersection.
				Interval inter = dom(f[i]).Intersection(img(g[j]));
				double left = g[j].PreValue(inter.right), right = g[j].PreValue(inter.left);
				fog.AddPiece(LinearFunction({left, f[i](inter.right)}, {right, f[i](inter.left)}));
			}
		}
	}
	return fog;
}

PWLFunction PWLFunction::Inverse() const
{
	PWLFunction g;
	for (auto& p: Pieces()) g = Max(g, PWLFunction({p.Inverse()}));
	return g;
}

PWLFunction PWLFunction::RestrictDomain(const Interval& domain) const
{
	PWLFunction f;
	for (auto& p: Pieces())
	{
		if (!domain.Intersects(p.domain)) continue;
		f.AddPiece(p.RestrictDomain(domain));
	}
	return f;
}

PWLFunction PWLFunction::RestrictImage(const Interval& image) const
{
	PWLFunction f;
	for (auto& p: Pieces())
	{
		if (!image.Intersects(p.image)) continue;
		f.AddPiece(p.RestrictImage(image));
	}
	return f;
}

// TODO: @0nyr - could be optimized if sure that the 
// function has continuous subdomains.
PartitionedInterval PWLFunction::DomainBreakpoints() const
{
	PartitionedInterval pi;
	for (auto& p: pieces_) pi.add(p.domain);
	return pi;
}

PartitionedInterval PWLFunction::ImageBreakpoints() const
{
	PartitionedInterval pi;
	for (auto& p: pieces_) pi.add(p.image);
	return pi;
}

bool PWLFunction::check_invariant() const
{
	if (pieces_.empty()) return domain_ == Interval(INFTY, -INFTY) && image_ == Interval(INFTY, -INFTY);
	
	// Check that the domain is the union of the pieces domains.
	Interval domain = pieces_.front().domain;
	for (auto& p: pieces_) domain = domain.Union(p.domain);
	if (domain != domain_) return false;
	
	// Check that the image is the union of the pieces images.
	Interval image = pieces_.front().image;
	for (auto& p: pieces_) image = image.Union(p.image);
	if (image != image_) return false;
	
	return true;
}

void PWLFunction::Print(std::ostream& os) const
{
	os << "[";
	for (int i = 0; i < PieceCount(); ++i)
	{
		if (i > 0) os << ", ";
		os << Piece(i);
	}
	os << "]";
}

bool PWLFunction::operator==(const PWLFunction& f) const
{
	if (PieceCount() != f.PieceCount()) return false;
	for (int i = 0; i < PieceCount(); ++i) if (Piece(i) != f.Piece(i)) return false;
	return true;
}

bool PWLFunction::operator!=(const PWLFunction& f) const
{
	return !(*this == f);
}

void PWLFunction::UpdateImage()
{
	image_ = {INFTY, -INFTY};
	for (auto& p: pieces_)
	{
		image_.left = min(image_.left, p.image.left);
		image_.right = max(image_.right, p.image.right);
	}
}

void from_json(const json& j, PWLFunction& f)
{
	for (auto& p: j) f.AddPiece(p);
}

void to_json(json& j, const PWLFunction& f)
{
	j = vector<LinearFunction>();
	for (auto& p: f.Pieces()) j.push_back(p);
}

string to_string(const PWLFunction& f)
{
	ostringstream msg;
	msg << "PWLFunction" << endl;

	// print domain and image intervals
	msg << setw(14) << f.Domain() << setw(14) << f.Image() << endl;
	
	vector<double> xs;
	vector<double> ys;
	for (size_t i = 0; i < f.Pieces().size(); ++i)
	{
		auto&p = f.Pieces()[i];
		xs.push_back(p.domain.left);
		xs.push_back(p.domain.right);
		ys.push_back(p.image.left);
		ys.push_back(p.image.right);
		
		if (i >= f.Pieces().size() - 1) // if last piece
		{
			msg << setw(26) << setprecision(6) << fixed << p.domain.left << setw(26) << setprecision(6) << fixed << p.image.left << endl;
			msg << setw(26) << setprecision(6) << fixed << p.domain.right << setw(26) << setprecision(6) << fixed << p.image.right << endl; 
		}
		else
		{
			msg << setw(26) << setprecision(6) << fixed << p.domain.left << setw(26) << setprecision(6) << fixed << p.image.left << endl;
		}
	}
	// print vectors, using goc/print/print_utils.h '<<'
	print_padded_vectors(msg, xs, ys);
	// msg << f.DomainBreakpoints();
	// msg << f.ImageBreakpoints();
	return msg.str();
}

PWLFunction operator+(const PWLFunction& f, const PWLFunction& g)
{
	PWLFunction h;
	int i = 0, j = 0;
	while (i < f.PieceCount() && j < g.PieceCount())
	{
		auto& pf = f.Piece(i), &pg = g.Piece(j);
		if (pf.domain.Intersects(pg.domain))
		{
			double left = max(pf.domain.left, pg.domain.left), right = min(pf.domain.right, pg.domain.right);
			h.AddPiece(LinearFunction({left, pf.Value(left)+pg.Value(left)}, {right, pf.Value(right)+pg.Value(right)}));
		}
		if (epsilon_equal(pf.domain.right, pg.domain.right)) { ++i; ++j; }
		else if (epsilon_smaller(pf.domain.right, pg.domain.right)) { ++i; }
		else { ++j; }
	}
	return h;
}

PWLFunction operator-(const PWLFunction& f, const PWLFunction& g)
{
	return f + g * -1.0;
}

PWLFunction operator*(const PWLFunction& f, const PWLFunction& g)
{
	PWLFunction h;
	int i = 0, j = 0;
	while (i < f.PieceCount() && j < g.PieceCount())
	{
		auto& pf = f.Piece(i), &pg = g.Piece(j);
		if (pf.domain.Intersects(pg.domain))
		{
			double left = max(pf.domain.left, pg.domain.left), right = min(pf.domain.right, pg.domain.right);
			h.AddPiece(LinearFunction({left, pf.Value(left)*pg.Value(left)}, {right, pf.Value(right)*pg.Value(right)}));
		}
		if (epsilon_equal(pf.domain.right, pg.domain.right)) { ++i; ++j; }
		else if (epsilon_smaller(pf.domain.right, pg.domain.right)) { ++i; }
		else { ++j; }
	}
	return h;
}

PWLFunction operator+(const PWLFunction& f, double a)
{
	return f + PWLFunction::ConstantFunction(a, f.Domain());
}

PWLFunction operator+(double a, const PWLFunction& f)
{
	return f + a;
}

PWLFunction operator-(const PWLFunction& f, double a)
{
	return f + (-1 * a);
}

PWLFunction operator-(double a, const PWLFunction& f)
{
	return f * -1.0 + a;
}

PWLFunction operator*(const PWLFunction& f, double a)
{
	return f * PWLFunction::ConstantFunction(a, f.Domain());
}

PWLFunction operator*(double a, const PWLFunction& f)
{
	return f * a;
}

PWLFunction Max(const PWLFunction& f_orig, const PWLFunction& g_orig)
{
	PWLFunction f = f_orig, g = g_orig;
	PWLFunction h;
	int i = 0, j = 0;
	while (i < f.PieceCount() && j < g.PieceCount())
	{
		auto& pf = (LinearFunction&)f.Piece(i);
		auto& pg = (LinearFunction&)g.Piece(j);
		// If pf has a part before pg.
		if (epsilon_smaller(pf.domain.left, pg.domain.left))
		{
			double l = pf.domain.left, r = min(pf.domain.right, pg.domain.left);
			h.AddPiece(LinearFunction(Point2D(l, pf.Value(l)), Point2D(r, pf.Value(r))));
			pf.domain.left = r;
			pf.image.left = pf.Value(r);
			if (epsilon_equal(r, pf.domain.right)) ++i;
		}
		else if (epsilon_smaller(pg.domain.left, pf.domain.left))
		{
			double l = pg.domain.left, r = min(pg.domain.right, pf.domain.left);
			h.AddPiece(LinearFunction(Point2D(l, pg.Value(l)), Point2D(r, pg.Value(r))));
			pg.domain.left = r;
			pg.image.left = pg.Value(r);
			if (epsilon_equal(r, pg.domain.right)) ++j;
		}
		else if (epsilon_equal(pf.domain.left, pg.domain.left))
		{
			double inter = pf.Intersection(pg);
			double l = pf.domain.left;
			double r = min(pf.domain.right, pg.domain.right);
			if (epsilon_bigger(inter, l) && epsilon_smaller(inter, r)) r = inter;
			h.AddPiece(LinearFunction(Point2D(l, max(pf.Value(l), pg.Value(l))), Point2D(r, max(pf.Value(r), pg.Value(r)))));
			pf.domain.left = r;
			pg.domain.left = r;
			if (epsilon_equal(r, pf.domain.right)) ++i;
			if (epsilon_equal(r, pg.domain.right)) ++j;
		}
	}
	for (; i < f.PieceCount(); ++i) h.AddPiece(f.Piece(i));
	for (; j < g.PieceCount(); ++j) h.AddPiece(g.Piece(j));
	return h;
}

PWLFunction Max(const PWLFunction& f, double a)
{
	return Max(f, PWLFunction::ConstantFunction(a, f.Domain()));
}

PWLFunction Max(double a, const PWLFunction& f)
{
	return Max(f, a);
}

PWLFunction Min(const PWLFunction& f, const PWLFunction& g)
{
	return Max(f*-1.0, g*-1.0)*-1.0;
}

PWLFunction Min(const PWLFunction& f, double a)
{
	return Min(f, PWLFunction::ConstantFunction(a, f.Domain()));
}

PWLFunction Min(double a, const PWLFunction& f)
{
	return Min(f, a);
}
} // namespace goc