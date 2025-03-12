import datetime

from params.constants import FILEPATH_DATE_FORMAT

def format_date_for_filepath(
        datetime: datetime.datetime,
        format_str: str = FILEPATH_DATE_FORMAT
    ) -> str:
    """
    Format a datetime object to a string to be used in a file path.
    """
    return datetime.strftime(format_str)
