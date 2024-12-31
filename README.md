# TempStat

**TempStat** is a high-performance C program designed to process and analyze large-scale weather station temperature data efficiently. It handles massive datasets, calculating essential statistical metrics such as the minimum, maximum, and mean temperature for each weather station.

## Features

- **Dual Implementation Approach**:
  - **Memory-mapped I/O (mmap)**: Leverages memory mapping for fast file access and processing.
  - **Standard File I/O (fread)**: Uses traditional file reading for compatibility and comparison.
- **Efficient Data Handling**:
  - Optimized for datasets with over 50 million temperature records.
  - Processes large files without excessive memory usage.
- **Statistical Analysis**:
  - Calculates per-weather-station metrics: minimum, maximum, and mean temperatures.

## How It Works

TempStat processes a text file containing rows of temperature measurements from various weather stations. Each row in the input file typically includes:
- Weather station ID
- Recorded temperature
- Timestamp (optional, depending on the dataset)

The program computes the following metrics for each station:
- **Minimum Temperature**: The lowest recorded temperature.
- **Maximum Temperature**: The highest recorded temperature.
- **Mean Temperature**: The average temperature across all records.

## Prerequisites

- A C compiler .
- A dataset containing temperature records in a plain text format.



