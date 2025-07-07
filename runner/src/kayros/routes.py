
from utils.terminal import green

import kairos_tdvrptw as ks

def legacy_test_route_duration_calculation(
    tdvrptw_instance: ks.nyr.VRPInstance,
    artfs: ks.nyr.ARTFs
):
    """
    legacy manual test function.
    Test the route duration calculation using ONYR and LERA methods.
    You need to ensure the route selected is valid and feasible for the instance.
    """

    # Create some random routes for testing.
    random_routes: list[list[int]] = []
    # for i in range(5):
    #     random_route_length = 10
    #     #random_route_length = random.randint(1, tdvrptw_instance.nb_clients())
    #     random_clients = random.sample(range(1, tdvrptw_instance.nb_clients() + 1), random_route_length)
    #     random_route = [tdvrptw_instance.o] + random_clients + [tdvrptw_instance.d]
    #     random_routes.append(random_route)
    random_routes.append([
        0,
        37,
        14,
        44,
        86,
        6,
        101
    ])
    
    for route in random_routes:
        print(green(f"Random route: {route}"))
        # Evaluate the route.
        delta_route: ks.nyr.NDCPWLF = ks.nyr.perform_tree_chain_composition(
            tdvrptw_instance, 
            artfs, 
            route
        )
        print(green(f"RRTF (tree-chain): {delta_route}"))
        print(green(f"RRTF (tree-chain) duration: {ks.nyr.compute_optimal_departure_time_and_duration(delta_route)}"))
        delta_route_sequential: ks.nyr.NDCPWLF = ks.nyr.perform_sequential_chain_composition(
            artfs, 
            route
        )
        print(green(f"RRTF (sequential): {delta_route_sequential}"))
        print(green(f"RRTF (sequential) duration: {ks.nyr.compute_optimal_departure_time_and_duration(delta_route_sequential)}"))

        route_duration_onyr = ks.nyr.compute_RouteDuration_from_delta_path(
            delta_route,
            route
        )
        print(green(f"Route duration (ONYR): {route_duration_onyr}"))
        route_duration_lera = ks.nyr.compute_RouteDuration_lera(
            tdvrptw_instance,
            route
        )
        print(green(f"Route duration (LERA): {route_duration_lera}"))
        print("is equal:", route_duration_onyr == route_duration_lera)
