//
// Created by Gonzalo Lera Romero.
// Grupo de Optimizacion Combinatoria (GOC).
// Departamento de Computacion - Universidad de Buenos Aires.
//

#ifndef NETWORKS2019_BP_H
#define NETWORKS2019_BP_H

#include "goc/goc.h"
#include "instance/vrp_instance.h"
#include "nyr/log/timed_solutions.h"
#include "pricing_problem.h"
#include "spf.h"

namespace solver
{
typedef std::function<void(const PricingProblem& pricing_problem, int node_number, goc::Duration time_limit, goc::CGExecutionLog* cg_execution_log)> BCPPricingFunction;

// This class represents a branch cut and price algorithm. It is a one use object.
class BCP
{
public:
	goc::Duration time_limit;
	int node_limit;
	int cut_limit;
	BCPPricingFunction pricing_solver;
	
	// Initializes the Branch cut and price solver.
	// 	D: digraph that the VRP is based on.
	//	spf: set-partitioning formulation to use (it must include an initial solution).
	BCP(const goc::Digraph& D, SPF* spf);
	
	// Executes a Branch-Cut-Price algorithm on
	goc::BCPExecutionLog Run(nyr::VrpSolutionRecord& timed_solutions);
	
private:
	struct Node
	{
		int index;
		double bound;
		std::vector<goc::Arc> A; // forbidden arcs.
		goc::Valuation opt; // relaxation optimum.
		
		struct Comparator { inline bool operator() (Node* n1, Node* n2) { return n1->bound > n2->bound; } };
	};
	
	// Returns: an estimate of the node's bound.
	// If the node is infeasible or unbounded it returns INFTY.
	double EstimateBound(Node* node);
	
	// Solves the node relaxation using CG and sets its bound and opt attributes.
	// Adds it to the queue if it is feasible and fractional.
	void ProcessNode(Node* node, nyr::VrpSolutionRecord& timed_solutions);
	
	// Branches the node using strong branching.
	void BranchNode(Node* node, nyr::VrpSolutionRecord& timed_solutions);
	
	// The freeze heuristic consists in solving the SPF with the existing columns using a BC solver.
	// The best solution there is an UB to the problem.
	void FreezeHeuristic(nyr::VrpSolutionRecord& timed_solutions);
	
	// Separates subset row cuts with n = 3, k = 2.
	// Returns: if any cut was added.
	bool SeparateCuts(const goc::Valuation& z);
	
	std::priority_queue<Node*, std::vector<Node*>, Node::Comparator> q; // queue of nodes in the BB tree.
	double z_lb, z_ub; // z_lb = value of the worst open node. z_ub = value of the best int solution
	goc::Valuation ub; // Best int solution found so far.
	int node_seq; // number of nodes created.
	goc::Stopwatch rolex; // Stopwatch to measure the time spent in the algorithm.
	
	goc::Digraph D;
	SPF* spf;
	goc::LPSolver lp_solver;
	goc::CGSolver cg_solver;
	goc::BCPExecutionLog log;
};
} // namespace

#endif //NETWORKS2019_BP_H
