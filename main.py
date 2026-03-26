import json
import pandas as pd
import numpy as np
from io import StringIO
from pathlib import Path

# Path to THIS file (backend/app.py)
BASE_DIR = Path(__file__).resolve().parent
csv_directory = BASE_DIR
json_directory = BASE_DIR / "JSON"

def findcol(csvfile: str, query: str):
    data = pd.read_csv(csv_directory / csvfile)
    output = set()
    for row in data.itertuples(index=False):
        value = getattr(row, query)
        if isinstance(value, str):
            output.add(value.upper())
        elif pd.isna(value):
            # Skip NaN/empty values
            continue
        else:
            raise ValueError(f"{query} not a valid search option")

    return sorted(output)

def findoriginnames(csvfile:str):
    data = pd.read_csv(csv_directory / csvfile, low_memory=False)
    output = set()
    for row in data.itertuples(index=False):
        airport = getattr(row, "airport_1")
        geocode = getattr(row, "Geocoded_City1")
        if isinstance(airport, str) and pd.notna(geocode):
            # Keep geocode values even when they contain newlines like
            # 'City, ST\n(LAT, LNG)'; parsing of coordinates happens later.
            geocode_str = str(geocode).strip()
            output.add((airport.upper(), geocode_str))
        elif pd.isna(airport) or pd.isna(geocode):
            continue
    return sorted(output)

def finddestinationnames(csvfile: str):
    data = pd.read_csv(csv_directory / csvfile, low_memory=False)
    output = set()
    for row in data.itertuples(index=False):
        airport = getattr(row, "airport_2")
        geocode = getattr(row, "Geocoded_City2")
        if isinstance(airport, str) and pd.notna(geocode):
            # Preserve geocode strings; they may include a newline and
            # parentheses with coordinates which are parsed downstream.
            geocode_str = str(geocode).strip()
            output.add((airport.upper(), geocode_str))
        elif pd.isna(airport) or pd.isna(geocode):
            continue
    return sorted(output)

def findallroutes(csvfile: str):
    data = pd.read_csv(csv_directory / csvfile)
    output = dict()
    for row in data.itertuples(index=False):
        origin = getattr(row, "airport_1")
        destination = getattr(row, "airport_2")
        distance = getattr(row, "nsmiles")
        # collect only valid string airport codes
        if isinstance(origin, str) and isinstance(destination, str):
            output[(origin.upper(), destination.upper())] = distance
            output[(destination.upper(), origin.upper())] = distance
        elif pd.isna(origin) or pd.isna(destination):
            continue
        else:
            raise ValueError("Invalid data format in CSV")

    return output

def findoriginroutes(csvfile: str, origin: str):
    data = pd.read_csv(csv_directory / csvfile)
    output = dict()
    for row in data.itertuples(index=False):
        row_origin = getattr(row, "airport_1")
        destination = getattr(row, "airport_2")
        distance = getattr(row, "nsmiles")
        if isinstance(row_origin, str) and isinstance(destination, str):
            if row_origin.upper() == origin.upper():
                output[(row_origin.upper(), destination.upper())] = distance
        elif pd.isna(row_origin) or pd.isna(destination):
            continue
        else:
            raise ValueError("Invalid data format in CSV")
    
    return output


print(findallroutes("flight_data.csv"))

# if __name__ == "__main__":
#     # Debugging helper run only when executed directly.
#     print(findoriginnames("flight_data.csv"))