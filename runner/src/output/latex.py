import pandas as pd
from typing import Any

import kairos_tdvrptw as ks


def generate_latex_longtable(
    df: pd.DataFrame,
    table_caption: str = "Table caption",
    table_label: str = "label",
    column_format: str = "c", # Default to center alignment for all columns
):
    """
    Converts a (long) pandas DataFrame to a LaTeX longtable as string.
    """
    # Generate LaTeX longtable header
    header = " & ".join(f"\\makecell{{{str(col).replace('_', '\\\\')}}}" for col in df.columns) + " \\\\ \n"

    # Internal function to format cell values
    def format_cell(val):
        try:
            if isinstance(val, float):
                if val >= 10e40:
                    return r"$\infty$"
                elif val < 10e6:
                    # Format small values with two decimal places
                    return f"{val:.2f}"
                else:
                    # Use scientific notation for large values
                    return f"{val:.2e}"
        except Exception as e:
            print(f"Error formatting cell value {val}: {e}")
            pass
        return str(val)
    
    def color_row(row):
        """
        Color the row based on the 'is_stored_bks_correct' column.
        """
        if "is_stored_bks_correct" in row and not row["is_stored_bks_correct"]:
            return "\\rowcolor{lightred} "
        elif "percentage_diff_stored_onyr" in row and float(row["percentage_diff_stored_onyr"]) > 1.0:
            return "\\rowcolor{lightorange} "
        else:
            return ""

    # Generate LaTeX longtable rows
    rows = df.apply(lambda row: color_row(row) + " & ".join(format_cell(cell) for cell in row), axis=1).str.cat(sep=" \\\\ \n")
    
    formatting_for_columns = column_format * len(df.columns)

    # Combine everything into a longtable
    latex_table = (
        "\\begin{longtable}{" + formatting_for_columns + "}" + "\n" +
        "\\toprule\n" +
        header +
        "\\endfirsthead\n" +
        header +
        "\\endhead\n" +
        "\\midrule\n" +
        rows + " \\\\ \n" +
        "\\bottomrule\n"
    ).replace("_", "\\_")
    latex_table += (
        "\\caption{" + table_caption + "} \\\\ \n" +
        "\\label{table:" + table_label + "}" + "\n" +
        "\\end{longtable}\n"
    )
    
    return latex_table

