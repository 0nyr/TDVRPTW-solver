from typing import Optional

# Returns: text in color blue for console.
def blue(text: str, color: Optional[bool] = True) -> str:
    """
    Returns the given text wrapped in blue color codes if color is True.
    """
    return "\033[94m" + text + "\033[0m" if color else text

def red(text: str, color: Optional[bool] = True) -> str:
    """
    Returns the given text wrapped in red color codes if color is True.
    """
    return "\033[91m" + text + "\033[0m" if color else text

def green(text: str, color: Optional[bool] = True) -> str:
    """
    Returns the given text wrapped in green color codes if color is True.
    """
    return "\033[92m" + text + "\033[0m" if color else text

def purple(text: str, color: Optional[bool] = True) -> str:
    """
    Returns the given text wrapped in purple color codes if color is True.
    """
    return "\033[95m" + text + "\033[0m" if color else text
