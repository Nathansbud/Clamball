import itertools
from enum import Enum

ESC = "\033"
DEFAULT = "\033[0m"
class Colors(Enum):
    BLACK = "0"
    RED = "1"
    GREEN = "2"
    YELLOW = "3"
    BLUE = "4"
    MAGENTA = "5"
    CYAN = "6"
    WHITE = "7"

def rgb(text: str, rgb_triple: tuple) -> str:
    return f"\033[38;2;{rgb_triple[0]};{rgb_triple[1]};{rgb_triple[2]}m{text}{DEFAULT}"

def cc(text: str, color_code: int) -> str:
    return f"\033[38;5;{color_code}m{text}{DEFAULT}"

def color(text, foreground=None, background=None):
    return f"\033[{('3' + foreground.value + ';') if foreground else ''}{('4' + background.value + ';') if background else ''}1m{text}{DEFAULT}"

def col(text, c, background):
    if not background: return color(text, c)
    else: return color(text, None, c)

def black(text, bg=False): return col(text, Colors.BLACK, bg)
def red(text, bg=False): return col(text, Colors.RED, bg)
def green(text, bg=False): return col(text, Colors.GREEN, bg)
def yellow(text, bg=False): return col(text, Colors.YELLOW, bg)
def blue(text, bg=False): return col(text, Colors.BLUE, bg)
def magenta(text, bg=False): return col(text, Colors.MAGENTA, bg)
def cyan(text, bg=False): return col(text, Colors.CYAN, bg)
def white(text, bg=False): return col(text, Colors.WHITE, bg)
def bold(text): return color(text)
def rainbow(text, bg=False): 
    return "".join([
        f"{col(l, c if not bg else None, c if bg else None)}" 
        for l, c in zip(text, itertools.cycle(list(Colors)[1:-1]))
    ])
