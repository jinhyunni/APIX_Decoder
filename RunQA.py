import os
import sys
import shutil
from pathlib import Path
import subprocess
import argparse

# 0. Parser setting
parser = argparse.ArgumentParser()
parser.add_argument(
    "--filename",
    help="filename to download(without .bin)",
)

parser.add_argument(
    "--data-taken-time",
    type=int,
    default=None,
    help="Data-taking time. ex)300",
)

parser.add_argument(
  "--only-draw-plots",
  type=bool,
  default=False,
  help="Turn on this option to skip downloading .bin from server",
)

parser.add_argument(
  "--only-store",
  type=bool,
  default=False,
  help="Turn on this option to skip showing canvas of QA plots"
)

args = parser.parse_args()

filename = args.filename
data_taken_time = args.data_taken_time
only_draw = args.only_draw_plots
only_store = args.only_store

server = "npl17inch"
remote_dir = "/home/npl/AstroPix_9chip_2026TB_CERN/TB"
local_dir = Path("../data/TB@CERN")
local_dir.mkdir(parents=True, exist_ok=True)

# .bin to be downloaded
filename = Path(args.filename).stem

remote_file = f"{server}:{remote_dir}/{filename}.bin"
local_bin_file = f"{local_dir}/{filename}.bin"
local_root_file = f"{local_dir}/{filename}.root"

# Command to draw QA plot
if data_taken_time is not None:
    run_qa_command = (
        f'Draw_RunQA.cpp("{local_root_file}", {data_taken_time})'
    )
else:
    run_qa_command = f'Draw_RunQA.cpp("{local_root_file}")'

# 1. Download .bin from server if requested
if not only_draw: 
  subprocess.run(
    ["scp", remote_file, str(local_bin_file)],
    check=True,
  )

# 2. Decode: .bin → .root
subprocess.run(
    [
      "root", "-l", "-b", "-q",
      f'APIXDecoder_TBCern.cpp("{local_bin_file}")',
    ],
    check=True,
)
# 3. Run QA
root_command = ["root", "-l"]
if only_store:
  root_command.extend(["-b", "-q"])
root_command.append(run_qa_command)

subprocess.run(root_command, check=True)
