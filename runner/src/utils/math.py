

def percentage_difference(a: float, b: float) -> float:
    """
    Calculate the percentage difference between two numbers.
    """
    if a == 0 and b == 0:
        return 0.0
    if a == 0:
        return float('inf')  # Avoid division by zero
    return abs((b - a) / a) * 100.0
