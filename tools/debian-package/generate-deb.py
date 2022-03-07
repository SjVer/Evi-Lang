#!/usr/bin/python3
# generates the debian package (NOT THE BINARY)

from sys import argv
from os import path
import os, shutil
import control_replacings

SCRIPT_DIR = path.dirname(path.realpath(__file__))
CURR_DIR = os.getcwd() + os.sep
ROOT_DIR = path.realpath(path.join(SCRIPT_DIR, "../.."))
DEB_DIR = path.join(ROOT_DIR, "bin", "deb")
COPIED_FILES_FILE = path.join(SCRIPT_DIR, "copied-files.txt")
COPIED_FILES_SEP = " -> "
CONTROL_FORMAT_FILE = path.join(SCRIPT_DIR, "control")

# ============================

def clean():
	if path.isdir(DEB_DIR):
		print(f"[generate-deb] Cleaning directory \"{DEB_DIR}\"".replace(CURR_DIR, ""))
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
	print(f"[generate-deb] Copying files to \"{DEB_DIR}\"".replace(CURR_DIR, ""))
	for i in range(len(pairs)):
		src = pairs[i][0]
		dest = pairs[i][1]

		if not path.isdir(path.dirname(dest)):
			# print(f"[generate-deb]    creating directory: {path.dirname(dest)}")
			os.makedirs(path.dirname(dest))

		if path.isfile(src):
			print(f"[generate-deb]    {i + 1}/{len(pairs)}: {src} -> {dest}".replace(CURR_DIR, ""))
			shutil.copy(src, dest)

		elif path.isdir(src):
			print(f"[generate-deb]    {i + 1}/{len(pairs)}: {src} -> {dest} (dir)".replace(CURR_DIR, ""))
			shutil.copytree(src, dest)

		else:
			print(f"[generate-deb] Error: Could not copy file or directory \"{src}\"".replace(CURR_DIR, ""))
			exit(1)

	print(f"[generate-deb] Finished copying files.")

def prepare_control():
	os.makedirs(path.join(DEB_DIR, "DEBIAN"))
	controlfile = path.join(DEB_DIR, "DEBIAN", "control")
	print(f"[generate-deb] Formatting control file \"{controlfile}\"".replace(CURR_DIR, ""))

	with open(CONTROL_FORMAT_FILE, "r") as f: text = f.read()
 
	for fmt, func in control_replacings.funcs.items():
		print(f"[generate-deb]    \"{fmt}\" -> \"{func()}\"")
		text = text.replace(f"{{{{{fmt}}}}}", func())

	with open(controlfile, 'w') as f: f.write(text)
	print("[generate-deb] Finished formatting.")

def finish():
	basename = f"{control_replacings.package_name()}_{control_replacings.version()}_{control_replacings.target}.deb"
	filename = path.join(argv[2], basename)
	print(f"[generate-deb] Generating \"{basename}\"...")

	if os.system(f"dpkg -b {DEB_DIR} {filename}") != 0:
		print(f"[generate-deb] Failed to generate deb package!")
		exit(1)

	print(f"[generate-deb] Done generating deb package at \"{filename}\".")

def main():
	if len(argv) != 3:
		print("Error: Expected target and output directory as only arguments.")
		exit(1)
  
	control_replacings.target = argv[1]
	control_replacings.root_dir = ROOT_DIR

	clean()

	prepare_files()
	prepare_control()

	finish()

# ============================

if __name__ == '__main__':
	main()