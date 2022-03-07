#!/usr/bin/python3
# generates the debian package (NOT THE BINARY)

from sys import argv
from os import path
import os, shutil
import control_replacings

ROOT_DIR = "../.."
DEB_DIR = path.join(ROOT_DIR, "bin", "deb")
COPIED_FILES_FILE = "copied-files.txt"
COPIED_FILES_SEP = " -> "

# ============================

def clean():
	# just rm the whole thing
	shutil.rmtree(DEB_DIR)

def prepare_files():
	os.makedirs(DEB_DIR)

	pairs = []
	# extract pairs
	with open(COPIED_FILES_FILE, "r") as f:
		for pair in [l.strip() for l in f.readlines()]:
			pair = pair.replace("ROOT_DIR", ROOT_DIR)
			pair = pair.replace("DEB_DIR", DEB_DIR)
			pair = pair.replace("|", os.sep)
			pairs.append(pair.split(COPIED_FILES_SEP))

	# do the copying
	print(f"[generate-deb] Copying files to \"{DEB_DIR}\"")
	for i in range(len(pairs)):
		src = pairs[i][0]
		dest = pairs[i][1]

		if not path.isdir(path.dirname(dest)):
			# print(f"[generate-deb]    creating directory: {path.dirname(dest)}")
			os.makedirs(path.dirname(dest))

		if path.isfile(src):
			print(f"[generate-deb]    {i + 1}/{len(pairs)}: {src} -> {dest}")
			shutil.copy(src, dest)

		elif path.isdir(src):
			print(f"[generate-deb]    {i + 1}/{len(pairs)}: {src} -> {dest} (dir)")
			shutil.copytree(src, dest)

		else:
			print(f"[generate-deb] Error: Could not copy file or directory \"{src}\"")
			exit(1)

	print(f"[generate-deb] Finished copying files.")

def prepare_control():
	os.makedirs(path.join(DEB_DIR, "DEBIAN"))
	controlfile = path.join(DEB_DIR, "DEBIAN", "control")
	print(f"[generate-deb] Preparing control file \"{controlfile}\"")



def finish():
	pass

def main():
	if len(argv) != 2:
		print("Error: Expected target as only argument.")
		exit(1)

	clean()

	prepare_files()
	prepare_control()

	finish()

# ============================

if __name__ == '__main__':
	main()