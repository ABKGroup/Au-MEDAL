import subprocess as sp
import os
import sys
import time
import utils

def run(database):
	start = time.time()
	dir_name = f"LIBGEN_{database}"
	os.makedirs(dir_name, exist_ok=True)
	os.chdir(dir_name)

	sp.call(f"python ../run_sm.py {database}", shell=True)
	os.chdir("../")
	# sp.call(f"rm -rf {dir_name}", shell=True)


if __name__ == "__main__":
	database = sys.argv[1]
	
	libDir = f"LIBGEN_{database}"
	os.makedirs(libDir, exist_ok=True)
	lib_name = f"LIB_{database}"
	except_lib = utils.getExceptLib(libDir, lib_name)		

	if len(except_lib) == 8:
		print("data already exists")

	run(database)

	time.sleep(5)
